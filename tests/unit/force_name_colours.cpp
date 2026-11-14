#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_name_scene.hpp"
#include "tests/unit/force_name_fixture.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
namespace {
using namespace gf;
struct Backend final:IRenderBackend {
    std::vector<DrawCall> draws;bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}void shutdown()override{}RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const Vertex*,std::size_t)override{return true;}bool upload_texture(u32,const TextureDesc&)override{return true;}bool upload_matrix(u32,const f32*)override{return true;}
    void begin_frame(const FrameDesc&)override{draws.clear();}void submit(const DrawCall& d)override{draws.push_back(d);}void end_frame()override{}void present()override{}
};
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"name colours: %s\n",why);std::abort();}}
}
int main(){
    using namespace gf;auto font=force_name_fixture::font();auto art=force_name_fixture::art();auto fixture=hsd_record_animation_fixture::Fixture{};auto bytes=fixture.archive();HsdRecordAnimationBank bank;std::string error;check(bank.load({bytes.data(),bytes.size()},&error),"owned bank");art.bank_hash=bank.hash();art.colours={{{0xe6e6e600,0x00004d00},{0xe6e6e600,0x4d000000}}};
    AttachmentAlternateProfile style;style.words={0x20080000,0x50140000,4,10,0x60002001,0x400a,0x20,7};style.alpha=.4f;style.loaded=true;Backend backend;RenderEngine render(backend);Camera camera;ForceNameScene scene;
    check(scene.load({bytes.data(),bytes.size()},art,font,style,camera,render,{},&error),"owned scene");ForceNameKeyboard keyboard;check(keyboard.open(art,font,std::string("\x82\x60",2)),"owned name");
    render.begin_frame({});render.reset_arena();check(scene.draw(render,keyboard,&error)&&render.end_frame(),"draw actual name scene entry");std::vector<DrawCall> text;for(const auto& d:backend.draws)if(d.texture==12000)text.push_back(d);
    check(text.size()==3,"selected key, text and cursor");check(text[0].texture_low_colour==0x4d000000&&text[0].texture_high_colour==0xe6e6e600,"preview source TEV0 then KONST");
    for(unsigned i=1;i<3;++i)check(text[i].texture_low_colour==0x00004d00&&text[i].texture_high_colour==0xe6e6e600,"white fill and blue glow for name/cursor");
    check(art.colours[0][0]==0xe6e6e600&&art.colours[0][1]==0x00004d00,"raw source style order retained");std::printf("name source colour binding: %u owned checks PASS\n",checks);
}
