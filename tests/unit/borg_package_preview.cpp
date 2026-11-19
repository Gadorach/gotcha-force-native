#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/native_animation_mosaic.hpp"

namespace {
void check(bool ok,const char* why,const std::string& detail={}){
    if(!ok){std::fprintf(stderr,"Borg package preview: %s (%s)\n",why,detail.c_str());std::abort();}
}
std::vector<gf::u8> read(const std::filesystem::path& path){
    std::error_code error;const auto size=std::filesystem::file_size(path,error);
    if(error||!size||size>(32u<<20))return {};
    std::ifstream file(path,std::ios::binary);return {std::istreambuf_iterator<char>(file),{}};
}
gf::ByteView view(const std::vector<gf::u8>& bytes){return {bytes.data(),bytes.size()};}
}
int main(){
    using namespace gf;
    const std::string profile="GFBORG-PREVIEW 1\nmodel-format=hsd-body-v1\nvariant=normal\n"
        "rig-prefix=visual/borg.prefix\npose-profile=visual/borg.pose\nidle=0:0\n";
    BorgPreviewProfile parsed;
    check(borg_read_preview_profile(profile,&parsed)&&parsed.variant=="normal"&&parsed.idle_group==0&&parsed.idle_choice==0,
        "explicit default-variant mapping accepted");
    for(auto bad:{"idle=0:0","variant=normal","rig-prefix=other","format=glb"})
        check(!borg_read_preview_profile(profile+bad,&parsed)&&parsed.variant=="normal","duplicate or unknown profile data refuses");
    auto outside=profile;outside.replace(outside.find("idle=0:0"),8,"idle=0:128");
    check(!borg_read_preview_profile(outside,&parsed),"out-of-domain clip fails rather than selecting idle");
    const std::filesystem::path base="local/content/native-battle-mvp-v1";
    const auto model=read(base/"g-red.hsd"),prefix=read(base/"g-red.prefix"),pose=read(base/"g-red.pose"),motions=read(base/"g-red.motions");
    if(model.empty()||prefix.empty()||pose.empty()||motions.empty()){
        std::puts("Borg package preview profile PASS; optional original model data absent");return 0;
    }
    headless::Render device;RenderEngine render(device);std::string error;
    check(render.initialize({640,480,false,true,"package preview"},nullptr,&error),"headless shared renderer",error);
    NativeAnimationMosaic preview;
    const BorgIdentity identity{"example:preview","G-Red model preview"};
    auto load=[&](const BorgPreviewProfile& p,ByteView pose_data){
        return preview.load_package_preview(identity,p,view(model),view(prefix),pose_data,view(motions),render,&error);
    };
    check(load(parsed,view(pose)), "load explicit idle through shared model and clip data",error);
    const auto shown=preview.page_previews();
    check(shown.size()==1&&shown[0].supported&&!shown[0].source_program&&shown[0].end_frame>0,"one raw in-place clip, no actor programs");
    GamePadState pad;pad.dpad_down=true;
    check(preview.step(pad,1.f/60,render,&error)&&preview.borg()==0,"package preview cannot switch to the classifier's other Borg",error);
    render.begin_frame({1,.055f,.065f,.085f});render.reset_arena();
    const auto draws=preview.draw(render,640,480);render.end_frame();
    check(draws>0,"real preview draw path uses existing headless renderer");
    CharacterMotionBank bank;
    check(character_motion_bank_load(view(motions),&bank,&error),"find actual absent source group",error);
    for(unsigned i=0;i<bank.groups.size();++i)if(bank.groups[i].empty()){
        auto missing=parsed;missing.idle_group=u8(i);
        check(!load(missing,view(pose))&&error.find("fallback is not allowed")!=std::string::npos,
            "explicit unmapped idle never falls back to group zero",error);
        break;
    }
    auto bad=pose;bad[0]^=1;
    check(!load(parsed,view(bad)),"malformed pose data blocks preview",error);
    std::puts("Borg package preview: exact idle, rig validation and shared renderer PASS");
}
