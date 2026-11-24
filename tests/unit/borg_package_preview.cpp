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
    auto full_profile=multi_text;
    for(unsigned i=2;i<kBorgPreviewExtraClips;++i)
        full_profile+="candidate=extra-"+std::to_string(i)+"|0:1\n";
    BorgPreviewProfile full;
    check(borg_read_preview_profile(full_profile,&full)&&full.clips.size()==kBorgPreviewExtraClips&&
          !borg_read_preview_profile(full_profile+"candidate=extra-last|0:1\n",&full)&&full.clips.size()==kBorgPreviewExtraClips,
          "movement and action candidates share a bounded clip table");
    auto candidate_text=profile;candidate_text[candidate_text.find(" 1\n")+1]='2';
    candidate_text+="candidate=ground-turn|0:1\n";
    BorgPreviewProfile candidate;
    check(borg_read_preview_profile(candidate_text,&candidate)&&candidate.clips.size()==1&&candidate.clips[0].candidate,
          "movement candidate is explicit optional inspection, not a strict clip");
    const std::string whole_text="GFBORG-PREVIEW 2\nmodel-format=hsd-whole-body-v1\nvariant=normal\nrig-prefix=visual/prefix\n";
    BorgPreviewProfile resting,whole;
    check(borg_read_preview_profile(whole_text+"idle=none\n",&resting)&&resting.static_body&&resting.whole_body,
          "explicit static body needs neither a clip nor authored lane profile");
    check(!borg_read_preview_profile(whole_text+"idle=none\nclip=walk|0:1\n",&whole),"static and animated declarations cannot mix");
    check(!borg_read_preview_profile(whole_text+"idle=none\npose-profile=visual/bad.pose\n",&whole),"whole body does not consume authored lane data");
    check(borg_read_preview_profile(whole_text+"idle=0:0\n",&whole)&&!whole.static_body&&whole.whole_body,
          "whole-body explicit clip");
    auto composite_text=whole_text;composite_text[composite_text.find(" 2\n")+1]='3';
    composite_text+="idle=0:0\n";
    BorgPreviewProfile mounted;
    check(borg_read_preview_profile(composite_text+"mount=17|7|-15|-5|0\nmount=2|7|0|0|0\nmount=14|17|0|0|0\n",&mounted)&&
          mounted.mounts.size()==3&&mounted.mounts[0].offset[0]==-15&&mounted.mounts[1].record==2,
          "version three binds selected records and permits shared mounts and repeated records");
    for(auto bad:{"","mount=0|7|0|0|0","mount=1|32|0|0|0","mount=1|7|-4097|0|0",
                  "mount=1|7|0|0|0|0","mount=1|7|0|0|0\nmount=1|7|1|0|0"})
        check(!borg_read_preview_profile(composite_text+bad,&mounted)&&mounted.mounts.size()==3,
              "bad assembly refuses atomically");
    check(!borg_read_preview_profile(whole_text+"idle=0:0\nmount=1|7|0|0|0\n",&mounted),
          "legacy profile cannot implicitly request a composite");
    auto audio_text=whole_text;audio_text[audio_text.find(" 2\n")+1]='4';
    constexpr std::string_view audio_row="lab-landing-pcm=audio/landing.pcm\n";
    BorgPreviewProfile sounds;
    check(borg_read_preview_profile(audio_text+"idle=0:0\n"+std::string(audio_row),&sounds)&&
          sounds.lab_landing_pcm=="audio/landing.pcm","explicit per-look lab audio is parsed");
    for(auto invalid:{"","lab-landing-pcm=../other.pcm\n","lab-landing-pcm=/tmp/other.pcm\n",
                      "lab-landing-pcm=a.pcm\nlab-landing-pcm=b.pcm\n"})
        check(!borg_read_preview_profile(audio_text+"idle=0:0\n"+invalid,&sounds)&&
              sounds.lab_landing_pcm=="audio/landing.pcm","bad audio declaration refuses atomically");
    check(!borg_read_preview_profile(whole_text+"idle=0:0\n"+std::string(audio_row),&sounds)&&
          !borg_read_preview_profile(audio_text+"idle=none\n"+std::string(audio_row),&sounds),
          "old formats and static bodies do not admit lab audio");
    check(borg_read_preview_profile(audio_text+"idle=0:0\nmount=1|7|0|0|0\n"+std::string(audio_row),&sounds)&&
          sounds.mounts.size()==1,"audio profile retains fixed mounts");
    auto marked_text=whole_text;marked_text[marked_text.find(" 2\n")+1]='5';
    marked_text+="idle=none\n";
    BorgPreviewProfile marked;
    check(borg_read_preview_profile(marked_text+"origin=first-point|0|150|0|-2.5e-1\n",&marked)&&
          marked.origins.size()==1&&marked.origins[0].point[2]==-.25f,"bounded exact-look inspection point");
    auto cruise_text=whole_text;cruise_text[cruise_text.find(" 2\n")+1]='6';
    BorgPreviewProfile cruiser;
    constexpr std::string_view cruise_row="lab-cruise=120|240|12|12|8|4\n";
    check(borg_read_preview_profile(cruise_text+"idle=none\n"+std::string(cruise_row),&cruiser)&&cruiser.lab_cruise&&
          cruiser.lab_cruise->capacity==240&&cruiser.static_body,"static flight shares authored boost tuple validation");
    check(!borg_read_preview_profile(cruise_text+"idle=0:0\n"+std::string(cruise_row),&cruiser)&&
          !borg_read_preview_profile(whole_text+"idle=none\n"+std::string(cruise_row),&cruiser)&&
          !borg_read_preview_profile(cruise_text+"idle=none\nlab-cruise=120|240|241|12|8|4\n",&cruiser)&&
           cruiser.lab_cruise->capacity==240,"cruise data neither enters the animated path nor bypasses fuel bounds");
    auto jet_text=whole_text;jet_text[jet_text.find(" 2\n")+1]='8';
    jet_text+="idle=0:0\n"+std::string(cruise_row);
    check(borg_read_preview_profile(jet_text+"candidate=cruise-entry|4:6\norigin=sample|0|0|0|5\n",&cruiser)&&
          !cruiser.static_body&&cruiser.clips.size()==1&&cruiser.clips[0].group==4&&cruiser.clips[0].choice==6,
          "animated cruise demands an explicit exact entry clip without changing raw preview idle");
    for(auto row:{"","clip=cruise-entry|4:6\n","candidate=cruise-entry|4:6\ncandidate=cruise-entry|2:0\n"})
        check(!borg_read_preview_profile(jet_text+row,&cruiser)&&cruiser.clips[0].group==4,
              "animated cruise rejects missing, eager or duplicate entry clips atomically");
    auto pace_header=whole_text;pace_header[pace_header.find(" 2\n")+1]='7';
    const auto pace_text=pace_header+"idle=0:0\ncandidate=ground-travel|0:1\n";
    BorgPreviewProfile paced;
    check(borg_read_preview_profile(pace_text+"lab-ground-pace=750\n",&paced)&&paced.lab_ground_pace==750,
          "exact animated preview declares fixed-point lab pace");
    for(auto bad:{"","lab-ground-pace=41\n","lab-ground-pace=1043\n","lab-ground-pace=750\nlab-ground-pace=600\n"})
        check(!borg_read_preview_profile(pace_text+bad,&paced)&&paced.lab_ground_pace==750,
              "missing, out-of-domain and duplicate pace declarations refuse atomically");
    check(!borg_read_preview_profile(pace_header+"idle=0:0\nlab-ground-pace=750\n",&paced)&&
          !borg_read_preview_profile(pace_header+"idle=none\nlab-ground-pace=750\n",&paced)&&
          !borg_read_preview_profile(whole_text+"idle=0:0\ncandidate=ground-travel|0:1\nlab-ground-pace=750\n",&paced),
          "pace requires version seven and an animated ground-travel binding");
    auto air_text=whole_text;air_text[air_text.find(" 2\n")+1]='9';
    air_text+="idle=0:0\ncandidate=ground-travel|0:1\ncandidate=ground-takeoff|0:6\n"
              "candidate=air-continuation|0:24\ncandidate=landing|0:9\ncandidate=air-dash-wind-up|0:14\n"
              "candidate=air-dash-active|0:23\ncandidate=air-dash-recovery|0:23\nlab-ground-pace=750\n";
    BorgPreviewProfile with_starts;
    check(borg_read_preview_profile(air_text+"lab-air-dash-starts=0|6|20\n",&with_starts)&&
          with_starts.lab_air_dash_starts==std::array<u16,3>{0,6,20}&&with_starts.lab_ground_pace==750,
          "air-dash phases can bind distinct start frames on a shared exact clip");
    for(auto row:{"","lab-air-dash-starts=0|0|0\n","lab-air-dash-starts=0|601|20\n",
                  "lab-air-dash-starts=0|6|20|30\n","lab-air-dash-starts=0|6|20\nlab-air-dash-starts=0|6|20\n"})
        check(!borg_read_preview_profile(air_text+row,&with_starts)&&with_starts.lab_air_dash_starts[2]==20,
              "absent, malformed and duplicate start declarations refuse atomically");
    for(auto bad:{"","origin=first-point|32|0|0|0\n","origin=bad|0|nan|0|0\n","origin=bad|0|0|0|1e6\n",
                  "origin=bad|0|0|0|0\norigin=bad|1|1|1|1\n","origin=bad|0|0|0\n"})
        check(!borg_read_preview_profile(marked_text+bad,&marked)&&marked.origins[0].id=="first-point",
              "malformed or duplicated preview origin refuses atomically");
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
    unsigned body_submissions=0;
    const auto draws=preview.draw(render,640,480,&body_submissions);render.end_frame();
    check(draws>body_submissions&&body_submissions>0&&device.body_draws>0,"real preview submits model geometry, not just overlay");
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
    check(load(candidate,view(pose))&&preview.select(0,1,render,&error)&&preview.page_previews()[0].supported,
          "candidate validates and samples when selected",error);
    auto bad_candidate_bank=motions;
    const auto bytes=view(motions);
    const auto archive=bytes.be32(bytes.be32(0)+4);
    const auto joint_symbols=archive+32+bytes.be32(archive+4)+bytes.be32(archive+8)*4;
    bool modified=false;
    for(unsigned i=0;i<bytes.be32(archive+12);++i){
        const auto joint=bytes.be32(joint_symbols+i*8);
        const auto aobj=bytes.be32(archive+32+joint+8);
        if(aobj){store_be32(bad_candidate_bank.data()+archive+32+aobj,1);modified=true;break;}
    }
    check(modified&&preview.load_package_preview(identity,candidate,view(model),view(prefix),view(pose),
          view(bad_candidate_bank),render,&error),"unsupported optional candidate cannot block the default",error);
    check(!preview.select(0,1,render,&error)&&preview.page()==0&&preview.page_previews()[0].supported&&
          error.find("unsupported candidate ground-turn")!=std::string::npos,
          "choosing an unsupported candidate refuses and retains the current clip",error);
    check(preview.close_package_preview(render),"optional clip keeps the same texture owner");
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
    check(preview.load_package_preview(identity,marked,view(model),view(prefix),{},{},render,&error)&&
          !preview.package_origin_world(),"declared point is hidden until explicitly selected",error);
    check(preview.select_package_origin("first-point",render,&error)&&!preview.select_package_origin("missing",render,&error),
          "unknown point refuses without replacing the active point");
    const auto first=preview.package_origin_world();
    check(first.has_value(),"selected logical joint has a valid world point");
    render.begin_frame({3,.055f,.065f,.085f});render.reset_arena();
    check(!preview.select_package_origin("first-point",render,&error),"selection refuses during frame submission");
    check(preview.draw(render,640,480)>0&&preview.package_origin_submitted(),"point draws on the shared body preview");
    render.end_frame();
    GamePadState rotate;rotate.z=true;
    check(preview.step(rotate,0.f,render,&error)&&!preview.package_origin_submitted()&&
          preview.package_origin_world()&&preview.package_origin_world()!=first,"rotation publishes a newly transformed point",error);
    render.begin_frame({4,.055f,.065f,.085f});render.reset_arena();
    check(preview.draw(render,300,480)>0&&preview.package_origin_submitted(),"point stays in the fitted portrait viewport");
    render.end_frame();
    check(preview.close_package_preview(render)&&!preview.package_origin_world()&&!preview.package_origin_submitted(),
          "closing a preview retires its old point");
    auto absent=marked;absent.origins[0].joint=31;
    check(!preview.load_package_preview(identity,absent,view(model),view(prefix),{},{},render,&error)&&
          error.find("origin joint missing")!=std::string::npos,"absent exact joint does not use the root",error);
    std::puts("Borg package preview: exact clips, static body, rig validation and shared renderer PASS");
}
