#include <cstdio>
#include <cstdlib>
#include <limits>
#include "tests/unit/startup_feedback_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"startup feedback: %s\n",why);std::abort();}}
class Backend final:public gf::IRenderBackend {
public:
    std::vector<char> events;std::vector<gf::DrawCall> draws;std::vector<gf::Vertex> vertices;std::vector<gf::FramebufferCopy> copies;
    bool capable=true,copy_ok=true,vertices_ok=true;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}void shutdown()override{}
    gf::RenderCapabilities capabilities()const override{gf::RenderCapabilities c;c.supports_framebuffer_copy=capable;return c;}
    bool upload_vertices(const gf::Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return vertices_ok;}bool upload_texture(gf::u32,const gf::TextureDesc&)override{return true;}bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{events.clear();draws.clear();copies.clear();}void submit(const gf::DrawCall& d)override{events.push_back('D');draws.push_back(d);}void end_frame()override{}void present()override{}
    bool copy_framebuffer(const gf::FramebufferCopy& c)override{events.push_back('C');copies.push_back(c);return copy_ok;}
};
void world(gf::RenderEngine& r){std::vector<gf::Vertex> vertices;gf::ScreenDraw::textured_quad(vertices,0,0,16,16,0,0,0,1,1);auto at=r.append_vertices(vertices.data(),vertices.size());check(r.draw(gf::Primitive::Triangles,at,vertices.size()),"owned preceding geometry queued");}
}
int main(){
    using namespace gf;using namespace startup_feedback_fixture;std::string error;Backend backend;RenderEngine render(backend);check(render.initialize({16,16,false,true,"owned feedback"},nullptr,&error),"backend init");
    for(unsigned mode:{1u,2u}){
        auto raw=bytes(mode);StartupFeedbackProfile p;check(startup_feedback_load({raw.data(),raw.size()},&p,&error),"owned source-shaped profile");StartupFeedback feedback;check(feedback.load(p,999,&error),"explicit native resource load");StartupDirectorState d;d.config_words=p.source_words;d.config_value=.5f;
        for(unsigned frame=1;frame<=4;++frame){d.updates=frame;std::vector<StartupEffect> e;if(frame==1)e.push_back(enabled(true));if(frame==3)e.push_back(alpha(0));if(frame==4)e.push_back(enabled(false));check(feedback.step(d,e,&error),"consecutive source update");render.begin_frame({});render.reset_arena();world(render);const auto saved=render.state();StartupFeedbackRender out;
            check(feedback.render(render,frame,16,16,&out,&error)&&render.state().texture==saved.texture&&render.state().depth_write==saved.depth_write&&render.state().position_matrix==saved.position_matrix,"shared state restored");render.end_frame();
            if(frame==4){check(!out.captured&&!out.drawn&&backend.events==std::vector<char>{'D'},"disabled calls neither backend capture nor feedback draw");continue;}
            check(out.captured&&out.drawn==(mode==1||frame>1)&&out.alpha==(out.drawn?(frame==3?0:127):0),"first copy readiness and trunc255*.5=127, not128");
            const std::vector<char> expected=mode==1?std::vector<char>{'D','C','D'}:frame==1?std::vector<char>{'D','C'}:std::vector<char>{'D','D','C'};
            check(backend.events==expected&&backend.copies.size()==1,"capture/draw order and no duplicate end-frame submissions");
            if(out.drawn){const auto& draw=backend.draws.back();check(draw.texture==999&&draw.depth_test&&draw.depth_compare==7&&!draw.depth_write&&draw.blend&&draw.blend_src==5&&draw.blend_dst==6&&draw.position_matrix==kIdentityPositionMatrix,"source blend/depth/identity selection");check(backend.vertices[draw.vertex_first].x==1&&backend.vertices[draw.vertex_first].y==1&&backend.vertices[draw.vertex_first+2].x==15&&backend.vertices[draw.vertex_first+2].y==15,"source inset rectangle is not a fitted camera zoom");}
        }
        check(feedback.state().captures==3&&feedback.state().draws==(mode==1?3u:2u),"capture counters are real accepted operations");
    }
    auto raw=bytes();StartupFeedbackProfile p;check(startup_feedback_load({raw.data(),raw.size()},&p,&error),"guard profile");StartupFeedback feedback;StartupDirectorState d;d.updates=1;d.config_words=p.source_words;d.config_value=.5f;std::vector<StartupEffect> enable={enabled(true)};StartupFeedbackRender out;
    check(feedback.load(p,123,&error)&&feedback.step(d,enable,&error),"failure setup");backend.capable=false;render.begin_frame({});render.reset_arena();world(render);check(!feedback.render(render,1,16,16,&out,&error)&&!feedback.ready()&&!out.captured&&feedback.state().captures==0&&backend.events.empty(),"unknown backend refuses before flushing, no fabricated capture");render.end_frame();backend.capable=true;
    check(feedback.load(p,123,&error)&&feedback.step(d,enable,&error),"checked copy failure setup");backend.copy_ok=false;render.begin_frame({});render.reset_arena();world(render);check(!feedback.render(render,1,16,16,&out,&error)&&!feedback.ready()&&feedback.state().captures==0&&backend.events==std::vector<char>{'D','C'},"late capture failure acknowledges no image; preceding draws are not atomically undone");render.end_frame();backend.copy_ok=true;
    for(unsigned size=0;size<raw.size();++size){StartupFeedbackProfile rejected;check(!startup_feedback_load({raw.data(),size},&rejected,&error),"truncated profile refuses");}
    for(unsigned at:{0u,4u,6u,8u,12u,16u,28u,32u,36u,40u,44u,48u,52u}){auto bad=raw;bad[at]^=0x80;StartupFeedbackProfile rejected;check(!startup_feedback_load({bad.data(),bad.size()},&rejected,&error),"invalid header/format/rectangle/operand refuses");}
    check(feedback.load(p,123,&error)&&!feedback.step(StartupDirectorState{},enable,&error),"frame0 duplicate refuses");
    check(feedback.load(p,123,&error)&&feedback.step(d,enable,&error),"duplicate render setup");render.begin_frame({});render.reset_arena();check(feedback.render(render,1,16,16,&out,&error)&&!feedback.render(render,1,16,16,&out,&error)&&!feedback.ready(),"one render per accepted frame");render.end_frame();
    auto mismatch=d;mismatch.config_words[0]=1;check(feedback.load(p,123,&error)&&!feedback.step(mismatch,enable,&error),"configuration binding refuses neighbor profile");
    auto bad_alpha=enable;bad_alpha.push_back(alpha(std::numeric_limits<float>::quiet_NaN()));check(feedback.load(p,123,&error)&&!feedback.step(d,bad_alpha,&error),"nonfinite live alpha refuses");
    FramebufferCopy c;c.texture=9;c.width=c.height=c.source_width=c.source_height=c.reference_width=c.reference_height=16;check(framebuffer_copy_valid(c)&&!render.copy_framebuffer(c),"copy outside frame refuses");
    for(unsigned mode=0;mode<7;++mode){auto bad=c;if(mode==0)bad.texture=0;if(mode==1)bad.width=0;if(mode==2)bad.height=4097;if(mode==3)bad.x=0xffffffff;if(mode==4)bad.source_width=17;if(mode==5)bad.reference_height=0;if(mode==6)bad.y=17;check(!framebuffer_copy_valid(bad),"copy extent/overflow budget controls");}
    std::printf("{\"startup_feedback_owned_checks\":%u,\"real_framebuffer_pixels\":false,\"all_passed\":true}\n",checks);
}
