#include <cstdio>
#include <cstdlib>
#include "engine/graphics/screen_fade.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"fade quad: %s\n",why);std::abort();}}
struct Sink final:gf::IRenderBackend {
    bool matrix_ok=true;unsigned matrices=0;std::vector<gf::Vertex> vertices;std::vector<gf::DrawCall> calls;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}
    bool upload_vertices(const gf::Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return true;}
    bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{++matrices;return matrix_ok;}
    void begin_frame(const gf::FrameDesc&)override{calls.clear();vertices.clear();}void submit(const gf::DrawCall& call)override{calls.push_back(call);}void end_frame()override{}void present()override{}
};
}
int main(){
    using namespace gf;Sink sink;RenderEngine render(sink);RenderConfig config;std::string error;check(render.initialize(config,nullptr,&error),"owned renderer");
    render.state().texture=42;render.state().position_matrix=7;render.state().depth_write=true;render.state().post_rgb_add={1,2,3};ScreenFade fade;
    render.begin_frame({});check(draw_screen_fade(render,fade,640,480)&&render.arena_size()==0&&sink.matrices==0,"inactive draws nothing");check(!draw_screen_fade(render,fade,0,480)&&!draw_screen_fade(render,fade,640,16385),"bounded dimensions");
    check(fade.request(0x12345600,4,ScreenFadeDirection::Reveal),"reveal");fade.advance();const auto remaining=fade.remaining();check(draw_screen_fade(render,fade,640,480)&&fade.remaining()==remaining,"draw cannot advance fade");
    check(render.state().texture==42&&render.state().position_matrix==7&&render.state().depth_write&&render.state().post_rgb_add==std::array<u8,3>{1,2,3},"all borrowed render state restored");
    check(render.end_frame()&&sink.calls.size()==1,"one ordinary quad");const auto& call=sink.calls[0];check(!call.texture&&!call.depth_test&&!call.depth_write&&!call.cull_backface&&call.blend&&call.blend_src==5&&call.blend_dst==6,"portable blend/state flags");
    check(sink.vertices.size()==6,"shared primitive topology");for(const auto& vertex:sink.vertices)check(vertex.r==0x12&&vertex.g==0x34&&vertex.b==0x56&&vertex.a==191,"source fade colour/alpha, no second weighting");
    fade.clear();fade.advance();render.reset_arena();render.begin_frame({});check(draw_screen_fade(render,fade,640,480)&&render.end_frame()&&sink.calls.empty(),"clear removes draw on next callback advance");
    check(fade.request(0,32767,ScreenFadeDirection::Cover),"long bounded source cover");fade.advance();check(fade.draw()&&u8(fade.colour())==0,"cover can publish alpha0 without being inactive");
    render.reset_arena();render.begin_frame({});check(draw_screen_fade(render,fade,640,480)&&render.end_frame()&&sink.calls.size()==1,"alpha0 source draw retained");
    sink.matrix_ok=false;render.reset_arena();render.begin_frame({});check(!draw_screen_fade(render,fade,640,480)&&render.state().texture==42&&render.state().position_matrix==7&&render.state().post_rgb_add==std::array<u8,3>{1,2,3},"matrix failure refuses and restores prior state");
    std::printf("{\"screen_fade_draw_owned_checks\":%u,\"all_passed\":true}\n",checks);
}
