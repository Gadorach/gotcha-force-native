#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
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
class TrackingRender final:public gf::IRenderBackend {
public:
    std::set<gf::u32> textures;
    unsigned uploads=0,body_draws=0;
    bool initialize(const gf::RenderConfig&,gf::IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{textures.clear();}
    bool upload_vertices(const gf::Vertex*,std::size_t)override{return true;}
    bool upload_texture(gf::u32 slot,const gf::TextureDesc&)override{textures.insert(slot);++uploads;return true;}
    bool release_texture(gf::u32 slot)override{textures.erase(slot);return true;}
    bool upload_matrix(gf::u32,const gf::f32*)override{return true;}
    void begin_frame(const gf::FrameDesc&)override{body_draws=0;}
    void submit(const gf::DrawCall& call)override{if(call.depth_test&&call.vertex_count)++body_draws;}
    void end_frame()override{}
    void present()override{}
};
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
    auto multi_text=profile;multi_text[multi_text.find(" 1\n")+1]='2';
    multi_text+="clip=walk|0:1\nclip=takeoff|0:6\n";
    BorgPreviewProfile multi;
    check(borg_read_preview_profile(multi_text,&multi)&&multi.clips.size()==2&&multi.clips[0].id=="walk",
          "version two binds named clips independently of idle");
    for(auto row:{"clip=walk|0:2","clip=idle|0:1","clip=other|16:0","clip=other|0:128"})
        check(!borg_read_preview_profile(multi_text+row,&multi)&&multi.clips.size()==2,"invalid extra clip refuses atomically");
    check(!borg_read_preview_profile(profile+"clip=walk|0:1",&multi),"version one does not accept new rows");
    const std::string whole_text="GFBORG-PREVIEW 2\nmodel-format=hsd-whole-body-v1\nvariant=normal\nrig-prefix=visual/prefix\n";
    BorgPreviewProfile resting,whole;
    check(borg_read_preview_profile(whole_text+"idle=none\n",&resting)&&resting.static_body&&resting.whole_body,
          "explicit static body needs neither a clip nor authored lane profile");
    check(!borg_read_preview_profile(whole_text+"idle=none\nclip=walk|0:1\n",&whole),"static and animated declarations cannot mix");
    check(!borg_read_preview_profile(whole_text+"idle=none\npose-profile=visual/bad.pose\n",&whole),"whole body does not consume authored lane data");
    check(borg_read_preview_profile(whole_text+"idle=0:0\n",&whole)&&!whole.static_body&&whole.whole_body,
          "whole-body explicit clip");
    const std::filesystem::path base="local/content/native-battle-mvp-v1";
    const auto model=read(base/"g-red.hsd"),prefix=read(base/"g-red.prefix"),pose=read(base/"g-red.pose"),motions=read(base/"g-red.motions");
    if(model.empty()||prefix.empty()||pose.empty()||motions.empty()){
        std::puts("Borg package preview profile PASS; optional original model data absent");return 0;
    }
    TrackingRender device;RenderEngine render(device);std::string error;
    check(render.initialize({640,480,false,true,"package preview"},nullptr,&error),"headless shared renderer",error);
    const u8 pixel[]{255,255,255,255};
    check(render.upload_texture_shared(42,{1,1,pixel})&&!render.release_texture(42),"preview release cannot evict shared assets");
    NativeAnimationMosaic preview;
    const BorgIdentity identity{"example:preview","G-Red model preview"};
    auto load=[&](const BorgPreviewProfile& p,ByteView pose_data){
        return preview.load_package_preview(identity,p,view(model),view(prefix),pose_data,view(motions),render,&error);
    };
    check(load(parsed,view(pose)), "load explicit idle through shared model and clip data",error);
    const auto shown=preview.page_previews();
    check(shown.size()==1&&shown[0].supported&&!shown[0].source_program&&shown[0].end_frame>0,"one raw in-place clip, no actor programs");
    check(device.textures.size()>1&&render.shared_texture_count()==1,"preview owns replaceable device textures separately from shared art");
    GamePadState pad;pad.dpad_down=true;
    check(preview.step(pad,1.f/60,render,&error)&&preview.borg()==0,"package preview cannot switch to the classifier's other Borg",error);
    render.begin_frame({1,.055f,.065f,.085f});render.reset_arena();
    check(!preview.close_package_preview(render),"a preview cannot retire submitted textures in an active frame");
    const auto draws=preview.draw(render,640,480);render.end_frame();
    check(draws>0&&device.body_draws>0,"real preview submits model geometry, not just overlay");
    headless::Render other_device;RenderEngine other(other_device);
    check(!preview.close_package_preview(other)&&preview.close_package_preview(render)&&device.textures==std::set<u32>{42},
        "the owning renderer retires only the preview textures and leaves shared art alive");
    check(load(parsed,view(pose))&&preview.close_package_preview(render)&&device.textures==std::set<u32>{42},
         "close and reopen reuses the same bounded slots");
    check(load(multi,view(pose))&&preview.page_count()==3,"all named clips bind on the exact shared rig",error);
    const auto uploads=device.uploads;const auto textures=device.textures;
    check(preview.select(0,1,render,&error)&&preview.page_previews()[0].entry.choice==1,"select the declared walk clip",error);
    preview.sync_input({});GamePadState next;next.dpad_right=true;
    check(preview.step(next,1.f/60,render,&error)&&preview.page()==2&&preview.page_previews()[0].entry.choice==6,
         "next clip uses its own authored mapping",error);
    check(device.uploads==uploads&&device.textures==textures,"switching clips retains the decoded texture owner");
    check(!preview.select(0,3,render,&error)&&preview.page()==2,"out of range refuses without idle fallback");
    check(preview.close_package_preview(render)&&device.textures==std::set<u32>{42},"retire once after clip switches");
    CharacterMotionBank bank;
    check(character_motion_bank_load(view(motions),&bank,&error),"find actual absent source group",error);
    for(unsigned i=0;i<bank.groups.size();++i)if(bank.groups[i].empty()){
        auto missing=parsed;missing.idle_group=u8(i);
        check(!load(missing,view(pose))&&error.find("fallback is not allowed")!=std::string::npos,
             "explicit unmapped idle never falls back to group zero",error);
        missing=multi;missing.clips.back().group=u8(i);
        check(!load(missing,view(pose))&&error.find("takeoff")!=std::string::npos&&device.textures==std::set<u32>{42},
             "missing non-idle clip refuses at load before uploads",error);
        break;
    }
    auto bad=pose;bad[0]^=1;
    check(!load(parsed,view(bad)),"malformed pose data blocks preview",error);
    check(!load(resting,{}),"static declaration refuses a supplied motion bank",error);
    check(preview.load_package_preview(identity,resting,view(model),view(prefix),{},{},render,&error)&&
          preview.page_count()==1&&preview.page_previews()[0].end_frame==0,
          "static body draws with exact wrapper topology, without invented motion",error);
    render.begin_frame({2,.055f,.065f,.085f});render.reset_arena();
    const auto static_draws=preview.draw(render,640,480);render.end_frame();
    check(static_draws>0&&device.body_draws>0&&preview.close_package_preview(render)&&device.textures==std::set<u32>{42},
        "static body uses and releases shared renderer resources");
    check(load(whole,{})&&preview.page_previews()[0].end_frame>0&&preview.close_package_preview(render),
        "whole-body declared clip samples without guessed source lane layout",error);
    std::puts("Borg package preview: exact clips, static body, rig validation and shared renderer PASS");
}
