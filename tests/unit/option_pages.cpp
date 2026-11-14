#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <iterator>
#include "engine/gameplay/option_scene.hpp"
#include "engine/core/headless_backends.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"option pages: %s\n",why);std::abort();}}std::vector<u8> read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}}
int main(int argc,char** argv){using namespace gf;OptionScene scene;std::string error;headless::Render backend;RenderEngine render(backend);check(!scene.configure_pages({},render,&error)&&!scene.update_page(OptionPresentationPage::More,0,{}, {},false,&error),"unconfigured pages refuse");
    if(argc==2){const std::filesystem::path root=argv[1];const auto r=root/"local/recomp-evidence/native-force-editor-20261111";auto bank=read(root/"local/analysis/arc/optn00_mdl.arc"),profile=read(root/"local/content/option-widgets.gfow"),fp=read(r/"option-extension-font-v2/font.gffn"),ft=read(r/"option-extension-font-v2/font.tpl");SourceFont font;auto v=[](const auto& x){return ByteView{x.data(),x.size()};};check(scene.load(v(bank),v(profile),&error)&&font.load(v(fp),v(ft),&error)&&scene.configure_pages(font,render,&error),"source bank/font pages");
        std::vector<std::pair<unsigned,unsigned>> ids;for(const auto& w:scene.profile().widgets)for(unsigned part=0;part<w.count;++part)ids.push_back({w.kind,w.id});check(ids.size()==scene.pose().joints.size(),"source instance order");unsigned staff=0;for(unsigned i=0;i<ids.size();++i)if(ids[i]==std::pair<unsigned,unsigned>{1,3})staff=i;
        check(scene.update(8,{},false,&error)&&!scene.pose().joints[staff].hidden,"legacy Staff Credits retained");check(scene.update_page(OptionPresentationPage::Settings,8,{}, {},false,&error)&&scene.pose().joints[staff].hidden,"More replaces only Staff Credits word art");
        check(scene.update_page(OptionPresentationPage::More,1,{}, {},false,&error)&&!scene.pose().joints[staff].hidden&&scene.pose().joints[staff].matrix[7]==scene.profile().row_y[1],"More reuses Staff Credits artwork on second source row");
        for(unsigned mask=0;mask<4;++mask){check(scene.update_page(OptionPresentationPage::Cheats,mask%2,{}, {bool(mask&1),bool(mask&2)},false,&error),"cheat choices");unsigned booleans=0;for(unsigned i=0;i<ids.size();++i){const auto [kind,id]=ids[i];const auto& p=scene.pose().joints[i];if(kind==5&&id>=1&&id<=2){check(!p.hidden&&p.matrix[7]==scene.profile().row_y[id-1],"original ON/OFF pair at actual cheat row");++booleans;}else if(kind!=0&&kind!=2)check(p.hidden,"unrelated option widgets hidden");}check(booleans==4,"exactly two complete ON/OFF pairs");}
        check(!scene.update_page(OptionPresentationPage::More,2,{}, {},false,&error)&&!scene.update_page(OptionPresentationPage::Cheats,7,{}, {},false,&error),"page row guards");check(scene.update(8,{},false,&error)&&!scene.pose().joints[staff].hidden,"legacy update restores source presentation");
        check(kCheatDescriptions[2].label=="INF HP"&&kCheatDescriptions[3].label=="INF AMMO"&&kCheatDescriptions[4].label=="INF JMP/FLT"&&kCheatDescriptions[5].label=="INF GF ENR"&&kCheatDescriptions[6].label=="INF SP ENR","owner short labels");
        for(unsigned row=0;row<7;++row){render.begin_frame({});render.reset_arena();check(scene.draw_details(render,row,&error)&&render.end_frame()&&render.arena_size()>6,"each source-font details popup draws");check(kCheatDescriptions[row].implemented==(row<2),"implemented tools and explicit battle placeholders");}
        check(!scene.draw_details(render,7,&error),"unknown details row refuses");
    }else check(argc==1,"arguments");std::printf("option source-style pages: %u checks PASS\n",checks);return 0;
}
