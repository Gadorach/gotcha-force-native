// Offline native geometry smoke for the local retail roster export.
#include <filesystem>
#include <fstream>
#include <iostream>
#include "engine/assets/tpl.hpp"
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/gameplay/character_model_scene.hpp"
#include "engine/assets/borg_gameplay_profile.hpp"
#include "engine/assets/character_motion.hpp"

namespace {
std::vector<gf::u8> load(const std::filesystem::path& path) {
    std::error_code error;
    if(!std::filesystem::is_regular_file(path,error)||error||std::filesystem::is_symlink(path,error)||error)return {};
    const auto size=std::filesystem::file_size(path,error);
    if(error||!size||size>(16u<<20))return {};
    std::vector<gf::u8> bytes(static_cast<std::size_t>(size));
    std::ifstream src(path,std::ios::binary);
    if(!src.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||src.peek()!=EOF)return {};
    return bytes;
}
gf::ByteView view(const std::vector<gf::u8>& bytes){return {bytes.data(),bytes.size()};}
bool check_feet(const std::filesystem::path& folder,bool small){
    using namespace gf;
    const auto model=load(folder/"visual/body.hsd"),prefix=load(folder/"visual/body.prefix"),
        pose=load(folder/"visual/body.pose"),motion=load(folder/"visual/body.motions"),rules=load(folder/"gameplay.txt");
    CharacterModelScene body;CharacterMotionBank bank;BorgGameplayProfile profile;std::string error;
    const auto fail=[&](const char* what){std::cerr<<folder<<": "<<what<<": "<<error<<'\n';return false;};
    if(rules.empty()||!borg_read_gameplay_profile({reinterpret_cast<const char*>(rules.data()),rules.size()},&profile,&error)||
       profile.animated_translation_percent!=(small?50:100)||!body.load_record_zero(view(model),view(prefix),view(pose),&error)||
       !character_motion_bank_load(view(motion),&bank,&error))return fail("exact foot-test package");
    const auto& rig=body.rig();const auto& asset=body.asset();
    for(const auto [choice,tick]:{std::array{0u,0u},std::array{1u,20u},std::array{5u,0u}}){
        CharacterMotionSelection selected;HsdBoneMotionClip clip;HsdBoneMotionSample sample;
        if(!character_motion_select(bank,0,choice,&selected,&error)||selected.fallback||
           !hsd_bone_motion_load(selected.archive,rig.binding,&clip,&error)||
           !hsd_bone_motion_sample(clip,tick,&sample,&error)||
           !character_pose_scale_translations(sample,profile.animated_translation_percent))return fail("exact grounded motion");
        std::array<CharacterPoseLane,4> lanes;for(auto& lane:lanes)lane.current=&sample;
        CharacterLocalPose local;CharacterPoseFrame frame;CharacterWorldPose world;
        if(!character_pose_sample(rig,lanes,&local,&error)||!character_pose_initial_frame(rig,Mat4{},&frame,&error)||
           !character_pose_direct_world(rig,local,frame,&world,&error)||!body.set_world_pose(world,{},&error))return fail("skinned foot pose");
        // Measure the two feet by their own envelope influences, not min-Y
        // of the body/tail or the actor's grounded flag.
        std::array<f32,2> soles{1e6f,1e6f};
        for(unsigned m=0;m<asset.meshes.size();++m)for(unsigned i=0;i<asset.scene.meshes[m].vertices.size();++i){
            const auto palette=asset.scene.meshes[m].vertices[i].matrix_index/3;
            for(const auto& bone:asset.meshes[m].palettes[palette].influences)if(bone.weight>=.5f&&(bone.joint==23||bone.joint==26)){
                auto& sole=soles[bone.joint==26];
                sole=std::min(sole,body.world_geometry().world_vertices[m][i].position[1]);
            }
        }
        const auto support=std::min(soles[0],soles[1]);
        if(*std::max_element(soles.begin(),soles.end())>=1e6f||support>5||support< (choice==5?-45.f:-10.f))
            return fail("sole is not at the local floor");
        std::cout<<folder.filename()<<" exact pose 0:"<<choice<<" frame "<<tick<<", sole height "<<support<<'\n';
    }
    return true;
}
bool check(const std::filesystem::path& path, bool common, bool empty_trail, gf::RenderEngine& render) {
    const auto bytes=load(path);
    std::string error;
    gf::HsdRecordAnimationBank bank;
    if(!bank.load(view(bytes),&error,true,gf::HsdStaticMaterialPolicy::BodyInspection)||bank.records()!=(common?157u:14u)){
        std::cerr<<path<<": "<<error<<'\n';
        return false;
    }
    for(const unsigned record:common?std::vector<unsigned>{22,83}:std::vector<unsigned>{8,9}){
        gf::AnimatedModelScene scene;
        const bool bound=common?scene.load_compact_textures(bank,record,2,0,0,&error):
            scene.load_compact_static(bank,record,&error);
        if(empty_trail&&record==9){
            if(!bank.is_empty_static_record(record)||bound||scene.ready()||
               error!="hsd record selection: empty selected geometry"){
                std::cerr<<path<<" record 9 is not an exact empty placeholder: "<<error<<'\n';return false;
            }
            std::cout<<path<<" record 9: authored empty; no substitute\n";
            continue;
        }
        if(!bound||!scene.request(0,0,&error)){
            std::cerr<<path<<" record "<<record<<": "<<error<<'\n';
            return false;
        }
        gf::f32 low[3]{},high[3]{};
        if(!scene.bounds(low,high)){
            std::cerr<<path<<" record "<<record<<": no geometry bounds\n";return false;
        }
        auto span=std::max({high[0]-low[0],high[1]-low[1],high[2]-low[2],1.f});
        gf::Camera camera;
        for(unsigned i=0;i<3;++i)camera.target[i]=(low[i]+high[i])*.5f;
        camera.eye[0]=camera.target[0]+span;camera.eye[1]=camera.target[1]+span*.5f;
        camera.eye[2]=camera.target[2]+span*2;
        camera.near_plane=span*.01f;camera.far_plane=span*10;
        scene.set_enabled(true);
        scene.set_untextured_billboards(true);
        scene.upload_textures_shared(render,1);
        render.begin_frame({});
        const auto draws=scene.draw(render,camera);
        render.end_frame();
        if(!draws||render.shared_texture_upload_failed()){
            std::cerr<<path<<" record "<<record<<": no accepted draw\n";return false;
        }
        std::cout<<path<<" record "<<record<<": "<<draws<<" draw(s)\n";
    }
    return true;
}
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const std::filesystem::path root=argv[1];
    gf::TplImage atlas;
    std::string error;
    if(!gf::tpl_decode(view(load(root/"effects/common-atlas.tpl")),0,&atlas,&error)||atlas.width!=256||atlas.height!=64){
        std::cerr<<"effect atlas: "<<error<<'\n';return 2;
    }
    gf::headless::Render backend;
    gf::RenderEngine render(backend);
    if(!render.initialize({},nullptr,&error)){
        std::cerr<<error<<'\n';return 2;
    }
    bool good=true;
    for(const std::string_view id:{"0500","0509","050a","050c","0515","0516"}){
        const auto folder=root/(std::string("gg4e08-")+std::string(id));
        good=check(folder/"visual/body.hsd",false,id!="0500"&&id!="050c",render)&&good;
        good=check_feet(folder,id=="050c"||id=="0515"||id=="0516")&&good;
    }
    good=check(root/"effects/common-single.hsd",true,false,render)&&good;
    good=check(root/"effects/common-split.hsd",true,false,render)&&good;
    render.shutdown();
    return good?0:2;
}
