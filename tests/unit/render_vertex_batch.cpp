// Owned stream/barrier tests: observe the vertices consumed by each draw, not
// merely the upload count. No original assets or graphics device required.
#include <cstdio>
#include <cstdlib>
#include "engine/graphics/render_engine.hpp"
namespace {
using namespace gf;
void check(bool ok){if(!ok)std::abort();}
struct Sink final:IRenderBackend {
    std::vector<Vertex> vertices;std::vector<int> seen;unsigned uploads=0,presents=0,copies=0;bool reject=false;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    RenderCapabilities capabilities()const override{RenderCapabilities c;c.supports_framebuffer_copy=true;return c;}
    bool upload_vertices(const Vertex* p,std::size_t n)override{++uploads;if(reject)return false;vertices.assign(p,p+n);return true;}
    bool upload_texture(u32,const TextureDesc&)override{return true;}
    bool upload_matrix(u32,const f32*)override{return true;}
    bool copy_framebuffer(const FramebufferCopy&)override{++copies;seen.push_back(-1);return true;}
    void begin_frame(const FrameDesc&)override{}
    void submit(const DrawCall& d)override{check(d.vertex_first+d.vertex_count<=vertices.size());seen.push_back(int(vertices[d.vertex_first].x));}
    void end_frame()override{}
    void present()override{++presents;}
};
void triangle(RenderEngine& r,int id,bool model=false,float depth=0){Vertex v[3];for(auto& x:v)x.x=float(id);auto at=r.append_vertices(v,3);check(model?r.draw_model(Primitive::Triangles,at,3,depth):r.draw(Primitive::Triangles,at,3));}
}
int main(){
    Sink s;RenderEngine r(s);FramebufferCopy c;c.texture=1;c.width=c.height=c.reference_width=c.reference_height=c.source_width=c.source_height=4;
    r.begin_frame({});r.reset_arena();for(int i=0;i<100;++i)triangle(r,i);check(s.uploads==0&&s.seen.empty());check(r.end_frame());check(s.uploads==1&&s.seen.size()==100);for(int i=0;i<100;++i)check(s.seen[i]==i);
    s.seen.clear();r.begin_frame({});r.reset_arena();triangle(r,101);check(r.copy_framebuffer(c));triangle(r,102);check(r.end_frame());check((s.seen==std::vector<int>{101,-1,102}));check(s.uploads==3);
    s.seen.clear();r.begin_frame({});r.reset_arena();triangle(r,103);r.reset_arena();triangle(r,104);check(r.end_frame());check((s.seen==std::vector<int>{103,104}));
    s.seen.clear();r.begin_frame({});r.reset_arena();check(r.begin_model_pass(2));triangle(r,105,true,1);triangle(r,106,true,3);triangle(r,107,true,3);check(r.end_model_pass());check(r.end_frame());check((s.seen==std::vector<int>{106,107,105}));
    // Upload failure is latched for the frame: no stale draw, capture or retry.
    s.seen.clear();r.begin_frame({});r.reset_arena();triangle(r,108);s.reject=true;auto attempts=s.uploads;check(!r.copy_framebuffer(c));check(s.uploads==attempts+1&&s.seen.empty());s.reject=false;check(!r.end_frame());r.present();check(s.presents==0&&s.uploads==attempts+1);
    r.begin_frame({});r.reset_arena();triangle(r,109);check(r.end_frame());r.present();check(s.seen==std::vector<int>{109}&&s.presents==1);
    // Replacing a pending stream publishes its prefix before changing indices.
    s.seen.clear();r.begin_frame({});r.reset_arena();triangle(r,110);Vertex replacement[3];for(auto& v:replacement)v.x=111;check(r.upload_vertices(replacement,3));check(r.draw(Primitive::Triangles,0,3));check(r.end_frame());check((s.seen==std::vector<int>{110,111}));
    // A replacement cannot split a live, globally sorted model pass.
    r.begin_frame({});r.reset_arena();check(r.begin_model_pass(2));triangle(r,112,true,1);check(!r.upload_vertices(replacement,3));check(!r.end_model_pass());check(!r.end_frame());
    std::puts("render vertex batches: ordered streams, barriers, sorting, failure isolation PASS");
}
