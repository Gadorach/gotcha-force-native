#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/force_spec_scene.hpp"
#include "engine/core/headless_backends.hpp"
#include "tests/unit/force_name_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"spec art: %s\n",why);std::abort();}}
std::vector<u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
std::vector<u8> fixture(){std::vector<u8> b;auto word=[&](u32 v){auto at=b.size();b.resize(at+4);store_be32(b.data()+at,v);};auto half=[&](u16 v){auto at=b.size();b.resize(at+2);store_be16(b.data()+at,v);};auto real=[&](f32 v){word(f32_to_bits(v));};
    word(0x47465341);word(1);word(1);word(2);for(float v:{0.f,0.f,5.f,0.f,0.f,0.f,0.f,1.f,0.f,30.f,.1f,100.f,640.f/480})real(v);half(1);half(0);word(0);half(2);half(0);real(0);real(0);real(-5);
    for(float v:{10.f,-6000.f,2000.f,-60.f,-50.f,-29.f,275.f,bits_to_f32(0x38c90fdb)})real(v);
    for(unsigned i=0;i<3;++i){word(0xe0e0e000);word(0);}
    for(unsigned i=0;i<30;++i){half(2);half(0x8260);}return b;
}
}
int main(int argc,char** argv){
    using namespace gf;auto font=force_name_fixture::font();auto b=fixture();ForceSpecArt art;std::string error;auto view=[](const auto& x){return ByteView{x.data(),x.size()};};check(art.load(view(b),font,&error)&&art.widgets.size()==1&&art.tribes[19]==std::string("\x82\x60",2),"owned art profile");
    for(unsigned offset:{0u,4u,8u,12u,16u,64u,68u,70u,72u,76u,78u}){auto bad=b;if(offset==68||offset==70||offset==76||offset==78)store_be16(bad.data()+offset,0x7fff);else store_be32(bad.data()+offset,0x7fc00000);if(offset==8||offset==12)store_be32(bad.data()+offset,0);check(!art.load(view(bad),font,&error)&&art.widgets.empty(),"invalid header/camera/widget clears partial state");}
    check(!art.load({b.data(),b.size()-1},font,&error),"short label refuses");
    if(argc==3){const std::string root=argv[1];auto fp=read(root+"/font-inputs-v1/native-v1/editor-font.gffn"),tpl=read(root+"/font-inputs-v1/native-v1/editor-font.tpl"),ap=read(root+"/spec-art-export-v1/spec.gfsa"),mp=read(root+"/spec-art-export-v1/spec-text.gftx"),cb=read(root+"/catalog-inputs-v1/catalog.gfbc"),db=read(root+"/spec-data-export-v2/spec.gfsd"),bank=read(root+"/spec-art-inputs-v1/spec_mdl.arc");
        SourceFont actual_font;ForceSpecArt actual;SourceMessageBank messages;ForceCatalog catalog;ForceSpecificationCatalog data;check(actual_font.load(view(fp),view(tpl),&error)&&actual.load(view(ap),actual_font,&error)&&messages.load(view(mp),actual_font,&error)&&catalog.load(view(cb),&error)&&data.load(view(db),catalog,&error),"actual source assets");
        std::ofstream journal(argv[2]);for(unsigned group=0;group<2;++group)for(unsigned index=0;const auto* lines=messages.message(group,index);++index)for(unsigned line=0;line<lines->size();++line){journal<<"text "<<group<<' '<<index<<' '<<line;for(unsigned char c:lines->at(line))journal<<' '<<unsigned(c);journal<<'\n';}
        headless::Render backend;RenderEngine render(backend);ForceSpecScene scene;check(scene.load(view(bank),actual,messages,actual_font,render,&error),"actual shared UI component preflight");
        for(unsigned case_id=0;case_id<6;++case_id){ForceSpecWidgetInput input;const u16 model=case_id<3?0x615:0xa;check(data.authored_view(catalog,model,case_id<3?0:5,case_id<3?0:9,ForceSpecMoveAccess::Available,&input.borg),"actual model readout");input.callbacks=67+case_id*137;input.blink=u8((case_id*13)%71);input.page=ForceSpecPage(case_id%3);input.experience=case_id<3?0:12345;
            char name[16];std::snprintf(name,sizeof(name),"%04x.tpl",model);auto plate=read(root+"/spec-nameplate-inputs-v1/"+name);check(scene.bind_nameplate(model,view(plate),render,&error),"actual package nameplate");
            render.begin_frame({});render.reset_arena();check(scene.draw_background(render,input,&error)&&scene.draw_foreground(render,input,&error)&&render.end_frame(),"actual source widget render submission");
            for(unsigned row=0;row<actual.widgets.size();++row){ForceSpecWidgetPose pose;check(force_spec_widget_pose(actual,input,row,&pose),"actual widget callback");journal<<"widget "<<case_id<<' '<<row<<' '<<pose.record<<' '<<pose.visible<<' '<<unsigned(pose.layer)<<' '<<f32_to_bits(pose.frame);for(unsigned i=0;i<12;++i)journal<<' '<<f32_to_bits(pose.world.m[i]);journal<<'\n';}
        }
    }else check(argc==1,"arguments");
    std::printf("specification art/actual UI source: %u checks PASS\n",checks);
}
