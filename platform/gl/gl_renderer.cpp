// OpenGL 2.1 / GLSL 1.20 renderer backend.
//
// Target rationale (see docs/rewrite-architecture.md): GL 2.0 is the hard floor
// because GX TEV needs programmable colour *and* alpha stages, which fixed
// function cannot express. Render-to-texture is required for EFB copies and is
// core only from 3.0, so on 2.1 it comes from EXT_framebuffer_object.
//
// Deliberately avoids anything that needs a core profile: no VAOs, no
// `layout(location=)`, no `in`/`out`, so it runs on old hardware, VMs and
// software Mesa. Entry points beyond GL 1.1 are loaded through SDL, so no GL
// loader library is required.
#include "platform/gl/gl_renderer.hpp"
#include "engine/graphics/fog.hpp"

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace gf::gl {
namespace {

#define GF_GL_1X(X)                                                              \
    X(PFNGLACTIVETEXTUREPROC, ActiveTexture)                                     \
    X(PFNGLBLENDFUNCSEPARATEPROC, BlendFuncSeparate)                             \
    X(PFNGLGENBUFFERSPROC, GenBuffers)                                           \
    X(PFNGLBINDBUFFERPROC, BindBuffer)                                           \
    X(PFNGLBUFFERDATAPROC, BufferData)                                           \
    X(PFNGLDELETEBUFFERSPROC, DeleteBuffers)                                     \
    X(PFNGLVERTEXATTRIBPOINTERPROC, VertexAttribPointer)                         \
    X(PFNGLENABLEVERTEXATTRIBARRAYPROC, EnableVertexAttribArray)

#define GF_GL_2X(X)                                                              \
    X(PFNGLCREATESHADERPROC, CreateShader)                                       \
    X(PFNGLSHADERSOURCEPROC, ShaderSource)                                       \
    X(PFNGLCOMPILESHADERPROC, CompileShader)                                     \
    X(PFNGLGETSHADERIVPROC, GetShaderiv)                                         \
    X(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)                               \
    X(PFNGLCREATEPROGRAMPROC, CreateProgram)                                     \
    X(PFNGLATTACHSHADERPROC, AttachShader)                                       \
    X(PFNGLLINKPROGRAMPROC, LinkProgram)                                         \
    X(PFNGLBINDATTRIBLOCATIONPROC, BindAttribLocation)                           \
    X(PFNGLGETPROGRAMIVPROC, GetProgramiv)                                       \
    X(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog)                             \
    X(PFNGLDELETESHADERPROC, DeleteShader)                                       \
    X(PFNGLDELETEPROGRAMPROC, DeleteProgram)                                     \
    X(PFNGLUSEPROGRAMPROC, UseProgram)                                           \
    X(PFNGLGETUNIFORMLOCATIONPROC, GetUniformLocation)                           \
    X(PFNGLUNIFORM1IPROC, Uniform1i)                                             \
    X(PFNGLUNIFORM1FPROC, Uniform1f)                                             \
    X(PFNGLUNIFORM2FPROC, Uniform2f)                                             \
    X(PFNGLUNIFORM4FPROC, Uniform4f)                                             \
    X(PFNGLUNIFORMMATRIX4FVPROC, UniformMatrix4fv)

// EXT_framebuffer_object: required for EFB copies on GL 2.x.
#define GF_GL_EXT(X)                                                             \
    X(PFNGLGENFRAMEBUFFERSEXTPROC, GenFramebuffersEXT)                           \
    X(PFNGLBINDFRAMEBUFFEREXTPROC, BindFramebufferEXT)                           \
    X(PFNGLDELETEFRAMEBUFFERSEXTPROC, DeleteFramebuffersEXT)                     \
    X(PFNGLFRAMEBUFFERTEXTURE2DEXTPROC, FramebufferTexture2DEXT)                 \
    X(PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC, CheckFramebufferStatusEXT)             \
    X(PFNGLGENRENDERBUFFERSEXTPROC, GenRenderbuffersEXT)                         \
    X(PFNGLBINDRENDERBUFFEREXTPROC, BindRenderbufferEXT)                         \
    X(PFNGLRENDERBUFFERSTORAGEEXTPROC, RenderbufferStorageEXT)                   \
    X(PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC, FramebufferRenderbufferEXT)           \
    X(PFNGLDELETERENDERBUFFERSEXTPROC, DeleteRenderbuffersEXT)

#define GF_GL_FUNCTIONS(X) GF_GL_1X(X) GF_GL_2X(X) GF_GL_EXT(X)

struct GlFunctions {
#define GF_DECLARE(type, name) type name = nullptr;
    GF_GL_FUNCTIONS(GF_DECLARE)
#undef GF_DECLARE
};

GlFunctions functions;

// Names are looked up once; the GLSL/EXT suffix in the function name is not part
// of the exported symbol, so the lookup uses the plain glXxx spelling.
bool load_functions(std::string* error) {
#define GF_LOAD(type, name)                                                       \
    functions.name = reinterpret_cast<type>(SDL_GL_GetProcAddress("gl" #name));    \
    if (!functions.name) {                                                        \
        if (error) *error = "missing GL entry point: gl" #name;                    \
        return false;                                                             \
    }
    GF_GL_FUNCTIONS(GF_LOAD)
#undef GF_LOAD
    return true;
}

// GLSL 1.20: two texture stages is enough for the TEV stages the game shows in
// the battle path; additional stages are added as the port needs them.
const char* kVertexShader = R"(#version 120
attribute vec3 a_position;
attribute vec4 a_color;
attribute vec2 a_texcoord;
attribute float a_inv_w;
attribute vec4 a_light;
attribute vec4 a_specular;
varying vec4 v_color;
varying vec2 v_texcoord;
varying vec4 v_light;
varying vec3 v_specular;
uniform vec2 u_scale;
uniform mat4 u_model;
void main() {
    // Game space is 0..width x 0..height with the origin at the top-left.
    vec2 ndc = vec2(a_position.x * u_scale.x - 1.0, 1.0 - a_position.y * u_scale.y);
    // Reconstruct clip W after portable CPU projection. UI has inv_w=1.
    gl_Position = (u_model * vec4(ndc, 2.0 * a_position.z - 1.0, 1.0)) / a_inv_w;
    v_color = a_color;
    v_texcoord = a_texcoord;
    v_light = a_light;
    v_specular = a_specular.rgb;
}
)";

const char* kFragmentShader = R"(#version 120
varying vec4 v_color;
varying vec2 v_texcoord;
varying vec4 v_light;
varying vec3 v_specular;
uniform sampler2D u_texture0;
uniform sampler2D u_texture1;
uniform int u_texture_count;
uniform int u_alpha_test;
uniform float u_alpha_ref;
uniform int u_alpha_compare0, u_alpha_compare1, u_alpha_op;
uniform float u_alpha_ref1;
uniform int u_colour_mode, u_alpha_mode;
uniform float u_texture_weight;
uniform int u_colour_lerp;
uniform vec4 u_colour_low,u_colour_high;
uniform int u_texture_after_lighting;
uniform vec4 u_post_rgb_add;
uniform vec4 u_post_rgb_multiply;
uniform int u_fog_enabled;
uniform vec2 u_fog_range;
uniform vec4 u_fog_colour;
vec4 illuminate(vec4 base) {
    base *= v_light;
    base.rgb += v_specular;
    return clamp(base, 0.0, 1.0);
}
bool compare_alpha(float a, float b, int op) {
    if (op == 0) return false;
    if (op == 1) return a < b;
    if (op == 2) return a == b;
    if (op == 3) return a <= b;
    if (op == 4) return a > b;
    if (op == 5) return a != b;
    if (op == 6) return a >= b;
    return true;
}
void main() {
    vec4 base = v_color;
    if (u_texture_after_lighting != 0) base = illuminate(base);
    if (u_texture_count > 0) {
        vec4 tex = texture2D(u_texture0, v_texcoord);
        if(u_colour_lerp != 0) tex.rgb=mix(u_colour_low.rgb,u_colour_high.rgb,tex.rgb);
        if (u_colour_mode == 1) base.rgb = mix(base.rgb, tex.rgb, tex.a);
        else if (u_colour_mode == 2) base.rgb = mix(base.rgb, tex.rgb, tex.rgb);
        else if (u_colour_mode == 3) base.rgb = mix(base.rgb, tex.rgb, u_texture_weight);
        else if (u_colour_mode == 4) base.rgb *= tex.rgb;
        else if (u_colour_mode == 5) base.rgb = tex.rgb;
        else if (u_colour_mode == 7) base.rgb += tex.rgb;
        else if (u_colour_mode == 8) base.rgb -= tex.rgb;
        if (u_alpha_mode == 1) base.a = mix(base.a, tex.a, tex.a);
        else if (u_alpha_mode == 2) base.a = mix(base.a, tex.a, u_texture_weight);
        else if (u_alpha_mode == 3) base.a *= tex.a;
        else if (u_alpha_mode == 4) base.a = tex.a;
        else if (u_alpha_mode == 6) base.a += tex.a;
        else if (u_alpha_mode == 7) base.a -= tex.a;
    }
    // TEV-style modulation of a second stage; more stages land here as ported.
    if (u_texture_count > 1) base *= texture2D(u_texture1, v_texcoord);
    if (u_texture_after_lighting == 0) base = illuminate(base);
    // GX-style cut-out: discard fully transparent texels so they do not write
    // depth and hide the geometry behind them.
    base = clamp(base, 0.0, 1.0);
    base.rgb = clamp(base.rgb * u_post_rgb_multiply.rgb + u_post_rgb_add.rgb, 0.0, 1.0);
    if (u_alpha_test != 0) {
        float a = floor(base.a * 255.0 + 0.5);
        bool x = compare_alpha(a, u_alpha_ref, u_alpha_compare0);
        bool y = compare_alpha(a, u_alpha_ref1, u_alpha_compare1);
        bool pass = x && y;
        if (u_alpha_op == 1) pass = x || y;
        else if (u_alpha_op == 2) pass = x != y;
        else if (u_alpha_op == 3) pass = x == y;
        if (!pass) discard;
    }
    // CPU projection carries reciprocal eye depth in clip W. Its reciprocal
    // reconstructs per-fragment eye depth, rather than fitting a screen gradient.
    if (u_fog_enabled != 0) {
        float amount = clamp((1.0 / gl_FragCoord.w - u_fog_range.x) /
                             (u_fog_range.y - u_fog_range.x), 0.0, 1.0);
        base.rgb = mix(base.rgb, u_fog_colour.rgb, amount);
    }
    gl_FragColor = base;
}
)";

// EFB copy resampling stays on the GPU. Destination texture row zero represents
// the portable top row; GL's source framebuffer row zero is the bottom row.
// Sample explicit pixel centres so capture filtering is independent of the
// sampler used later to display the feedback texture.
const char* kCopyVertexShader = R"(#version 120
void main() { gl_Position = gl_Vertex; }
)";
const char* kCopyFragmentShader = R"(#version 120
uniform sampler2D u_source;
uniform vec2 u_source_size;
uniform vec2 u_destination_size;
void main() {
    // Integer division keeps exact half-way samples on the upper pixel. A
    // floating reciprocal can turn an exact integer into n-epsilon (97->128).
    ivec2 p = ivec2(gl_FragCoord.xy);
    ivec2 source = ivec2(u_source_size), destination = ivec2(u_destination_size);
    vec2 pixel = vec2(((p.x * 2 + 1) * source.x) / (destination.x * 2),
                      ((p.y * 2 + 1) * source.y) / (destination.y * 2));
    vec2 uv = vec2(pixel.x + 0.5, u_source_size.y - pixel.y - 0.5) / u_source_size;
    gl_FragColor = texture2D(u_source, uv);
}
)";

GLuint compile(GLenum type, const char* source, std::string* error) {
    const GLuint shader = functions.CreateShader(type);
    functions.ShaderSource(shader, 1, &source, nullptr);
    functions.CompileShader(shader);
    GLint ok = 0;
    functions.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char buffer[1024] = {};
        functions.GetShaderInfoLog(shader, sizeof(buffer) - 1, nullptr, buffer);
        if (error) *error = std::string("shader compile failed: ") + buffer;
        functions.DeleteShader(shader);
        return 0;
    }
    return shader;
}

class GlRenderer final : public IRenderBackend {
public:
    ~GlRenderer() override { shutdown(); }
    bool initialize(const RenderConfig& config, IWindowBackend* window, std::string* error) override {
        if (context_) { if (error) *error = "OpenGL renderer already initialized"; return false; }
        if (!window || !window->native_handle()) {
            if (error) *error = "OpenGL backend requires a window with a native handle";
            return false;
        }
        window_ = window;
        context_ = SDL_GL_CreateContext(static_cast<SDL_Window*>(window->native_handle()));
        if (!context_) {
            if (error) *error = std::string("SDL_GL_CreateContext: ") + SDL_GetError();
            return false;
        }
        if (!load_functions(error)) { shutdown(); return false; }
        SDL_GL_SetSwapInterval(config.vsync ? 1 : 0);

        // Fail loudly rather than silently losing EFB copies.
        if (!functions.GenFramebuffersEXT || !functions.BindFramebufferEXT) {
            if (error) *error = "OpenGL 2.1 target requires EXT_framebuffer_object for EFB copies";
            shutdown();
            return false;
        }

        const GLuint vs = compile(GL_VERTEX_SHADER, kVertexShader, error);
        if (!vs) { shutdown(); return false; }
        const GLuint fs = compile(GL_FRAGMENT_SHADER, kFragmentShader, error);
        if (!fs) { functions.DeleteShader(vs); shutdown(); return false; }
        program_ = functions.CreateProgram();
        functions.AttachShader(program_, vs);
        functions.AttachShader(program_, fs);
        functions.BindAttribLocation(program_, 0, "a_position");
        functions.BindAttribLocation(program_, 1, "a_color");
        functions.BindAttribLocation(program_, 2, "a_texcoord");
        functions.BindAttribLocation(program_, 3, "a_inv_w");
        functions.BindAttribLocation(program_, 4, "a_light");
        functions.BindAttribLocation(program_, 5, "a_specular");
        functions.LinkProgram(program_);
        GLint linked = 0;
        functions.GetProgramiv(program_, GL_LINK_STATUS, &linked);
        functions.DeleteShader(vs);
        functions.DeleteShader(fs);
        if (!linked) {
            char buffer[1024] = {};
            functions.GetProgramInfoLog(program_, sizeof(buffer) - 1, nullptr, buffer);
            if (error) *error = std::string("program link failed: ") + buffer;
            shutdown();
            return false;
        }
        scale_location_ = functions.GetUniformLocation(program_, "u_scale");
        texture_count_location_ = functions.GetUniformLocation(program_, "u_texture_count");
        alpha_test_location_ = functions.GetUniformLocation(program_, "u_alpha_test");
        alpha_ref_location_ = functions.GetUniformLocation(program_, "u_alpha_ref");
        texture0_location_ = functions.GetUniformLocation(program_, "u_texture0");
        texture1_location_ = functions.GetUniformLocation(program_, "u_texture1");
        model_location_ = functions.GetUniformLocation(program_, "u_model");
        const char* names[] = {"u_alpha_compare0", "u_alpha_compare1", "u_alpha_op",
                               "u_alpha_ref1", "u_colour_mode", "u_alpha_mode", "u_texture_weight"};
        for (unsigned i = 0; i < 7; ++i) material_locations_[i] = functions.GetUniformLocation(program_, names[i]);
        tint_locations_[0]=functions.GetUniformLocation(program_,"u_colour_lerp");
        tint_locations_[1]=functions.GetUniformLocation(program_,"u_colour_low");
        tint_locations_[2]=functions.GetUniformLocation(program_,"u_colour_high");
        lighting_order_location_=functions.GetUniformLocation(program_,"u_texture_after_lighting");
        post_rgb_add_location_=functions.GetUniformLocation(program_,"u_post_rgb_add");
        post_rgb_multiply_location_=functions.GetUniformLocation(program_,"u_post_rgb_multiply");
        fog_enabled_location_=functions.GetUniformLocation(program_,"u_fog_enabled");
        fog_range_location_=functions.GetUniformLocation(program_,"u_fog_range");
        fog_colour_location_=functions.GetUniformLocation(program_,"u_fog_colour");

        const GLuint copy_vs=compile(GL_VERTEX_SHADER,kCopyVertexShader,error);
        if(!copy_vs){shutdown();return false;}
        const GLuint copy_fs=compile(GL_FRAGMENT_SHADER,kCopyFragmentShader,error);
        if(!copy_fs){functions.DeleteShader(copy_vs);shutdown();return false;}
        copy_program_=functions.CreateProgram();
        functions.AttachShader(copy_program_,copy_vs);functions.AttachShader(copy_program_,copy_fs);
        functions.LinkProgram(copy_program_);
        functions.DeleteShader(copy_vs);functions.DeleteShader(copy_fs);
        functions.GetProgramiv(copy_program_,GL_LINK_STATUS,&linked);
        if(!linked){if(error)*error="framebuffer copy shader link failed";shutdown();return false;}
        copy_source_location_=functions.GetUniformLocation(copy_program_,"u_source");
        copy_size_location_=functions.GetUniformLocation(copy_program_,"u_source_size");
        copy_destination_location_=functions.GetUniformLocation(copy_program_,"u_destination_size");
        functions.GenFramebuffersEXT(1,&copy_fbo_);
        glGenTextures(1,&copy_source_);

        functions.GenBuffers(1, &vbo_);
        functions.BindBuffer(GL_ARRAY_BUFFER, vbo_);
        // Attribute slots are bound explicitly because 2.1 has no VAO.
        functions.EnableVertexAttribArray(0);
        functions.VertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, x)));
        functions.EnableVertexAttribArray(1);
        functions.VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, r)));
        functions.EnableVertexAttribArray(2);
        functions.VertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, u)));
        functions.EnableVertexAttribArray(3);
        functions.VertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, inv_w)));
        functions.EnableVertexAttribArray(4);
        functions.VertexAttribPointer(4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, light)));
        functions.EnableVertexAttribArray(5);
        functions.VertexAttribPointer(5, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex),
                                      reinterpret_cast<void*>(offsetof(Vertex, specular)));

        width_ = config.width ? config.width : 640;
        height_ = config.height ? config.height : 480;
        glEnable(GL_DEPTH_TEST);
        // The scene seam emits z = near/depth, so a larger value is nearer.
        // Clear depth to 0 and keep fragments with z >= the stored value.
        glClearDepth(0.0);
        glDepthFunc(GL_GEQUAL);
        // Initialize the screen-quad convention. Each draw explicitly supplies
        // its front face; don't infer world winding from just one Y inversion
        // (CPU projection and this vertex shader both change Y conventions).
        glFrontFace(GL_CW);
        glDisable(GL_CULL_FACE);
        return true;
    }

    void shutdown() override {
        if(copy_fbo_){functions.DeleteFramebuffersEXT(1,&copy_fbo_);copy_fbo_=0;}
        if(copy_source_){glDeleteTextures(1,&copy_source_);copy_source_=0;}
        if(copy_program_){functions.DeleteProgram(copy_program_);copy_program_=0;}
        copy_sizes_.clear();copy_source_width_=copy_source_height_=0;
        if (vbo_) { functions.DeleteBuffers(1, &vbo_); vbo_ = 0; }
        if (program_) { functions.DeleteProgram(program_); program_ = 0; }
        for (auto& entry : textures_) glDeleteTextures(1, &entry.second);
        textures_.clear();
        if (context_) { SDL_GL_DeleteContext(context_); context_ = nullptr; }
    }

    RenderCapabilities capabilities() const override {
        RenderCapabilities caps;
        caps.name = "opengl-2.1-glsl120";
        GLint value = 0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
        caps.max_texture_size = static_cast<u32>(value);
        caps.supports_depth_texture = true;
        caps.supports_framebuffer_copy = context_!=nullptr;
        return caps;
    }

    bool upload_vertices(const Vertex* vertices, std::size_t count) override {
        if (!vbo_) return false;
        vertex_count_ = static_cast<u32>(count);
        functions.BindBuffer(GL_ARRAY_BUFFER, vbo_);
        functions.BufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * sizeof(Vertex)),
                             vertices, GL_DYNAMIC_DRAW);
        return true;
    }

    bool upload_texture(u32 handle, const TextureDesc& desc) override {
        if (!handle || !desc.pixels || !desc.width || !desc.height) return false;
        copy_sizes_.erase(handle);
        GLuint name = 0;
        auto it = textures_.find(handle);
        if (it != textures_.end()) name = it->second;
        else {
            glGenTextures(1, &name);
            textures_[handle] = name;
        }
        functions.ActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, name);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.linear ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc.linear ? GL_LINEAR : GL_NEAREST);
        const auto wrap = [](u8 mode) { return mode == 0 ? GL_CLAMP_TO_EDGE : mode == 2 ? GL_MIRRORED_REPEAT : GL_REPEAT; };
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap(desc.wrap_s));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap(desc.wrap_t));
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(desc.width),
                     static_cast<GLsizei>(desc.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, desc.pixels);
        return true;
    }
    bool release_texture(u32 handle) override {
        if(!handle||!context_||frame_open_||SDL_GL_GetCurrentContext()!=context_)return false;
        const auto it=textures_.find(handle);
        if(it!=textures_.end()){
            glDeleteTextures(1,&it->second);
            textures_.erase(it);
        }
        copy_sizes_.erase(handle);
        return true;
    }

    bool copy_framebuffer(const FramebufferCopy& copy) override {
        if(!context_||SDL_GL_GetCurrentContext()!=context_||!frame_open_||!framebuffer_copy_valid(copy))return false;
        unsigned width=0,height=0;window_->size(&width,&height);
        if(!width||!height||width!=drawable_width_||height!=drawable_height_||width>16384||height>16384)return false;
        const u32 x0=u32(u64(copy.x)*width/copy.reference_width),x1=u32(u64(copy.x+copy.source_width)*width/copy.reference_width);
        const u32 y0=u32(u64(copy.y)*height/copy.reference_height),y1=u32(u64(copy.y+copy.source_height)*height/copy.reference_height);
        if(x0>=x1||y0>=y1||x1>width||y1>height||u64(x1-x0)*(y1-y0)>16777216)return false;
        const u32 sw=x1-x0,sh=y1-y0;
        GLint maximum=0,pack_buffer=0,unpack_buffer=0,framebuffer=0;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maximum);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack_buffer);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&unpack_buffer);glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT,&framebuffer);
        if(copy.width>u32(maximum)||copy.height>u32(maximum)||sw>u32(maximum)||sh>u32(maximum)||pack_buffer||unpack_buffer||framebuffer||glGetError()!=GL_NO_ERROR)return false;
        GLint program=0,active=0,binding=0,read_buffer=0;
        glGetIntegerv(GL_CURRENT_PROGRAM,&program);glGetIntegerv(GL_ACTIVE_TEXTURE,&active);glGetIntegerv(GL_READ_BUFFER,&read_buffer);
        functions.ActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D,&binding);
        // GL2.1 compatibility state for this internal copy only. Pack state is
        // never changed, and the model VBO/attribute pointers remain intact.
        glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_VIEWPORT_BIT);
        glReadBuffer(GL_BACK);glBindTexture(GL_TEXTURE_2D,copy_source_);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        if(copy_source_width_!=sw||copy_source_height_!=sh){
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,GLsizei(sw),GLsizei(sh),0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
            copy_source_width_=sw;copy_source_height_=sh;
        }
        glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,GLint(x0),GLint(height-y1),GLsizei(sw),GLsizei(sh));
        auto& target=textures_[copy.texture];if(!target)glGenTextures(1,&target);
        glBindTexture(GL_TEXTURE_2D,target);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,copy.linear?GL_LINEAR:GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,copy.linear?GL_LINEAR:GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        auto& size=copy_sizes_[copy.texture];
        if(size!=std::array<u32,2>{copy.width,copy.height}){
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,GLsizei(copy.width),GLsizei(copy.height),0,GL_RGBA,GL_UNSIGNED_BYTE,nullptr);
            size={copy.width,copy.height};
        }
        functions.BindFramebufferEXT(GL_FRAMEBUFFER_EXT,copy_fbo_);
        functions.FramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,GL_COLOR_ATTACHMENT0_EXT,GL_TEXTURE_2D,target,0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
        bool ok=functions.CheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT&&glGetError()==GL_NO_ERROR;
        if(ok){
            glViewport(0,0,GLsizei(copy.width),GLsizei(copy.height));
            glDisable(GL_DEPTH_TEST);glDisable(GL_STENCIL_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glDisable(GL_SCISSOR_TEST);glDisable(GL_ALPHA_TEST);glDisable(GL_DITHER);
            glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_FALSE);
            functions.UseProgram(copy_program_);functions.Uniform1i(copy_source_location_,0);
            functions.Uniform2f(copy_size_location_,float(sw),float(sh));functions.Uniform2f(copy_destination_location_,float(copy.width),float(copy.height));
            glBindTexture(GL_TEXTURE_2D,copy_source_);
            glBegin(GL_TRIANGLE_STRIP);glVertex2f(-1,-1);glVertex2f(1,-1);glVertex2f(-1,1);glVertex2f(1,1);glEnd();
            ok=glGetError()==GL_NO_ERROR;
        }
        functions.BindFramebufferEXT(GL_FRAMEBUFFER_EXT,GLuint(framebuffer));
        glPopAttrib();glReadBuffer(GLenum(read_buffer));
        functions.UseProgram(GLuint(program));glBindTexture(GL_TEXTURE_2D,GLuint(binding));functions.ActiveTexture(GLenum(active));
        ok=ok&&glGetError()==GL_NO_ERROR;
        if(!ok){copy_source_width_=copy_source_height_=0;copy_sizes_.erase(copy.texture);}
        return ok;
    }

    // Palette entries are stored as a 4x4 column-major matrix because that is
    // what glUniformMatrix4fv expects; the source is a row-major affine 3x4.
    bool upload_matrix(u32 slot, const f32* matrix) override {
        if (!matrix) return false;
        if (slot >= kMaxPaletteSlots) return false;
        auto& entry = palette_[slot];
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned column = 0; column < 4; ++column) {
                entry[column * 4 + row] = matrix[row * 4 + column];
            }
        }
        entry[3] = entry[7] = entry[11] = 0.0f;
        entry[15] = 1.0f;
        palette_valid_[slot] = true;
        return true;
    }

    void begin_frame(const FrameDesc& frame) override {
        unsigned w = width_, h = height_;
        if (window_) window_->size(&w, &h);
        drawable_width_=w;drawable_height_=h;frame_open_=true;
        glViewport(0, 0, static_cast<GLsizei>(w), static_cast<GLsizei>(h));
        // Clear is affected by write masks and scissor, unlike portable state.
        glDisable(GL_SCISSOR_TEST);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDepthMask(GL_TRUE);
        glClearColor(frame.clear_r, frame.clear_g, frame.clear_b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        functions.UseProgram(program_);
        functions.Uniform2f(scale_location_, 2.0f / static_cast<float>(width_),
                            2.0f / static_cast<float>(height_));
        functions.BindBuffer(GL_ARRAY_BUFFER, vbo_);
    }

    void submit(const DrawCall& call) override {
        if (!call.vertex_count) return;
        if (call.vertex_first + call.vertex_count > vertex_count_) return;
        if(!linear_fog_valid(call.fog))return;

        // Per-draw depth state is part of the seam: UI passes submit depth_test
        // off, and leaving the test on would silently discard them wherever the
        // 3D set already wrote a nearer fragment.
        if (call.depth_test) glEnable(GL_DEPTH_TEST);
        else glDisable(GL_DEPTH_TEST);
        glDepthMask(call.depth_write ? GL_TRUE : GL_FALSE);
        glColorMask(call.colour_write?GL_TRUE:GL_FALSE,call.colour_write?GL_TRUE:GL_FALSE,
                    call.colour_write?GL_TRUE:GL_FALSE,call.alpha_write?GL_TRUE:GL_FALSE);
        static const GLenum compare[] = {GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL,
                                         GL_GREATER, GL_NOTEQUAL, GL_GEQUAL, GL_ALWAYS};
        glDepthFunc(compare[call.depth_compare & 7]);
        glFrontFace(call.front_ccw ? GL_CCW : GL_CW);
        if (call.blend) {
            glEnable(GL_BLEND);
            functions.BlendFuncSeparate(blend_factor(call.blend_src), blend_factor(call.blend_dst),
                                        GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }
        if (call.cull_backface || call.cull_frontface) {
            glEnable(GL_CULL_FACE);
            glCullFace(call.cull_backface && call.cull_frontface ? GL_FRONT_AND_BACK : call.cull_frontface ? GL_FRONT : GL_BACK);
        }
        else glDisable(GL_CULL_FACE);

        // Single texture per draw in this stage of the port; the shader supports
        // two so a TEV pass can bind a second once base code provides it.
        unsigned bound = 0;
        auto it = textures_.find(call.texture);
        if (call.texture && it != textures_.end()) {
            functions.ActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, it->second);
            functions.Uniform1i(texture0_location_, 0);
            bound = 1;
        }
        functions.Uniform1i(texture_count_location_, static_cast<GLint>(bound));
        functions.Uniform1i(alpha_test_location_, call.alpha_test ? 1 : 0);
        functions.Uniform1f(alpha_ref_location_, static_cast<float>(call.alpha_ref));
        functions.Uniform1i(material_locations_[0], call.alpha_compare0);
        functions.Uniform1i(material_locations_[1], call.alpha_compare1);
        functions.Uniform1i(material_locations_[2], call.alpha_op);
        functions.Uniform1f(material_locations_[3], call.alpha_ref1);
        functions.Uniform1i(material_locations_[4], call.texture_colour_mode);
        functions.Uniform1i(material_locations_[5], call.texture_alpha_mode);
        functions.Uniform1f(material_locations_[6], call.texture_weight);
        functions.Uniform1i(tint_locations_[0],call.texture_colour_lerp?1:0);
        functions.Uniform1i(lighting_order_location_,call.texture_after_lighting?1:0);
        functions.Uniform4f(post_rgb_add_location_,call.post_rgb_add[0]/255.0f,call.post_rgb_add[1]/255.0f,call.post_rgb_add[2]/255.0f,0);
        functions.Uniform4f(post_rgb_multiply_location_,call.post_rgb_multiply[0]/255.0f,call.post_rgb_multiply[1]/255.0f,call.post_rgb_multiply[2]/255.0f,1);
        functions.Uniform1i(fog_enabled_location_,call.fog.enabled?1:0);
        functions.Uniform2f(fog_range_location_,call.fog.enabled?call.fog.start:0,call.fog.enabled?call.fog.end:1);
        functions.Uniform4f(fog_colour_location_,call.fog.colour[0]/255.0f,call.fog.colour[1]/255.0f,call.fog.colour[2]/255.0f,0);
        const auto colour=[&](GLint location,u32 rgba){functions.Uniform4f(location,
            float((rgba>>24)&255)/255,float((rgba>>16)&255)/255,float((rgba>>8)&255)/255,float(rgba&255)/255);};
        colour(tint_locations_[1],call.texture_low_colour);colour(tint_locations_[2],call.texture_high_colour);

        if (call.position_matrix < kMaxPaletteSlots && palette_valid_[call.position_matrix]) {
            functions.UniformMatrix4fv(model_location_, 1, GL_FALSE, palette_[call.position_matrix].data());
        } else {
            functions.UniformMatrix4fv(model_location_, 1, GL_FALSE, identity_.data());
        }

        GLenum mode = GL_TRIANGLES;
        switch (call.primitive) {
        case Primitive::Triangles: mode = GL_TRIANGLES; break;
        case Primitive::TriangleStrip: mode = GL_TRIANGLE_STRIP; break;
        case Primitive::Lines: mode = GL_LINES; break;
        case Primitive::Points: mode = GL_POINTS; break;
        }
        glDrawArrays(mode, static_cast<GLint>(call.vertex_first), static_cast<GLsizei>(call.vertex_count));
        ++draws_;
    }

    void end_frame() override {frame_open_=false;}
    void present() override { if (context_) SDL_GL_SwapWindow(static_cast<SDL_Window*>(window_->native_handle())); }

private:
    static GLenum blend_factor(u8 value) {
        switch (value) {
        case 1: return GL_ZERO;
        case 2: return GL_ONE;
        case 3: return GL_SRC_COLOR;
        case 4: return GL_ONE_MINUS_SRC_COLOR;
        case 5: return GL_SRC_ALPHA;
        case 6: return GL_ONE_MINUS_SRC_ALPHA;
        case 7: return GL_DST_ALPHA;
        default: return GL_ONE_MINUS_DST_ALPHA;
        }
    }

    IWindowBackend* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    GLuint program_ = 0, vbo_ = 0;
    GLuint copy_program_=0,copy_fbo_=0,copy_source_=0;
    GLint copy_source_location_=-1,copy_size_location_=-1,copy_destination_location_=-1;
    u32 copy_source_width_=0,copy_source_height_=0;
    std::unordered_map<u32,std::array<u32,2>> copy_sizes_;
    GLint scale_location_ = -1, texture_count_location_ = -1;
    GLint texture0_location_ = -1, texture1_location_ = -1;
    GLint alpha_test_location_ = -1;
    GLint alpha_ref_location_ = -1;
    GLint model_location_ = -1;
    std::array<GLint, 7> material_locations_{};
    std::array<GLint, 3> tint_locations_{};
    GLint lighting_order_location_=-1;
    GLint post_rgb_add_location_=-1;
    GLint post_rgb_multiply_location_=-1;
    GLint fog_enabled_location_=-1,fog_range_location_=-1,fog_colour_location_=-1;
    static constexpr u32 kMaxPaletteSlots = 64;
    std::array<std::array<f32, 16>, kMaxPaletteSlots> palette_{};
    std::array<bool, kMaxPaletteSlots> palette_valid_{};
    std::array<f32, 16> identity_{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    u32 vertex_count_ = 0;
    unsigned width_ = 640, height_ = 480;
    unsigned drawable_width_=0,drawable_height_=0;bool frame_open_=false;
    u64 draws_ = 0;
    std::unordered_map<u32, GLuint> textures_;
};

} // namespace

bool capture_framebuffer(std::vector<u8>& rgba, s32* width, s32* height) {
    if (!SDL_GL_GetCurrentContext()) return false;
    GLint viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    if (viewport[2] <= 0 || viewport[3] <= 0) return false;
    rgba.resize(static_cast<std::size_t>(viewport[2]) * static_cast<std::size_t>(viewport[3]) * 4);
    GLint pack=4,read=GL_BACK;
    glGetIntegerv(GL_PACK_ALIGNMENT,&pack);glGetIntegerv(GL_READ_BUFFER,&read);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    // Call BEFORE swap; the back buffer is undefined afterwards.
    glReadBuffer(GL_BACK);
    glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    const GLenum status=glGetError();
    glReadBuffer(static_cast<GLenum>(read));glPixelStorei(GL_PACK_ALIGNMENT,pack);
    if(status!=GL_NO_ERROR){rgba.clear();return false;}
    if (width) *width = viewport[2];
    if (height) *height = viewport[3];
    return true;
}

bool write_ppm(const char* path, const std::vector<u8>& rgba, s32 width, s32 height) {
    if (!path || width <= 0 || height <= 0) return false;
    if (rgba.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4) return false;
    std::FILE* file = std::fopen(path, "wb");
    if (!file) return false;
    std::fprintf(file, "P6\n%d %d\n255\n", width, height);
    // GL returns the bottom row first; PPM expects top-down.
    for (s32 row = height - 1; row >= 0; --row) {
        const u8* line = rgba.data() + static_cast<std::size_t>(row) * static_cast<std::size_t>(width) * 4;
        for (s32 column = 0; column < width; ++column) {
            const u8 pixel[3] = {line[column * 4 + 0], line[column * 4 + 1], line[column * 4 + 2]};
            std::fwrite(pixel, 1, 3, file);
        }
    }
    std::fclose(file);
    return true;
}

std::unique_ptr<IRenderBackend> create_renderer(ILogBackend* log) {
    (void)log;
    return std::make_unique<GlRenderer>();
}

} // namespace gf::gl
