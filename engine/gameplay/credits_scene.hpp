#pragma once
#include "engine/assets/credits.hpp"
#include "engine/assets/hsd_leaf_material.hpp"
#include "engine/gameplay/menu_scene.hpp"
namespace gf {
class CreditsScene {
public:
    bool load(ByteView archive,ByteView content,std::string* error){
        loaded_=false;timeline_.cancel();pose_={};
        HsdScene source;if(!hsd_load(archive,&source,error)||!credits_profile_load(content,archive,source,&profile_,error)||!hsd_share_texture_images(source))return false;
        const auto& c=profile_.config;std::vector<unsigned> mesh_for_model(c.model_count);
        std::vector<std::array<HsdAnimPose::Material,4>> styles(c.model_count);
        for(unsigned i=0;i<c.model_count;++i){
            unsigned m=0;while(m<source.meshes.size()&&source.mesh_joint[m]!=i)++m;if(m==source.meshes.size())return false;
            mesh_for_model[i]=m;const auto& mesh=source.meshes[m];
            for(auto& style:styles[i]){style.present=true;style.alpha=mesh.material_alpha;
                for(unsigned k=0;k<3;++k){style.ambient[k]=mesh.material_ambient[k]/255.0f;style.diffuse[k]=mesh.material_colour[k]/255.0f;}}
            bool animated=false;for(const auto& binding:source.material_animation_roots)if(binding.second==profile_.roots[i])animated=true;
            if(animated){HsdLeafMaterialClip clip;if(!hsd_leaf_material_clip_load(archive,source,profile_.roots[i],&clip,error))return false;
                for(unsigned time=0;time<=3;++time)if(!hsd_leaf_material_sample(clip,time,&styles[i][time],error))return false;
            }
            // Text styles 1..3 must have an explicit material clip. Special
            // image6 has no MOBJ AObj in this asset, so its request is a no-op.
            for(const auto& row:c.rows)if(row.kind<=3)for(unsigned g=0;g<row.count;++g)
                if(c.glyphs[row.first+g].model==i&&!animated){if(error)*error="credits scene: missing text style binding";return false;}
        }
        HsdScene instances;instances.textures=source.textures;
        auto append=[&](unsigned model){const auto m=mesh_for_model[model];instances.mesh_joint.push_back(u32(instances.joints.size()));
            instances.joints.push_back(source.joints[model]);instances.joints.back().parent=kHsdNoJoint;instances.meshes.push_back(source.meshes[m]);};
        append(0);pose_.joints.resize(c.glyphs.size()+1);pose_.materials.resize(c.glyphs.size()+1);
        const auto& background=source.joints[0];
        for(unsigned k=0;k<3;++k){pose_.joints[0].rotation[k]=background.rotation[k];pose_.joints[0].scale[k]=background.scale[k];pose_.joints[0].translation[k]=background.translation[k];}
        std::size_t vertices=instances.meshes[0].vertices.size();
        for(const auto& row:c.rows)for(unsigned k=0;k<row.count;++k){const auto index=row.first+k;const auto& glyph=c.glyphs[index];append(glyph.model);
            vertices+=instances.meshes.back().vertices.size();if(vertices>200000){if(error)*error="credits scene: vertex budget exceeded";return false;}
            auto& p=pose_.joints[index+1];p.hidden=true;p.matrix_override=true;
            const f32 x=row.kind<=3?std::fma(c.grid_factor,c.x_step*f32(glyph.column),c.x_base):c.image_x;
            const f32 z=row.kind<=3?std::fma(c.grid_factor,c.z_step*f32(glyph.column),c.z_base):c.image_z;
            const f32 rotation[3]={0,c.rotation_y,0},scale[3]={c.scale,c.scale,c.scale},position[3]={x,row.spawn_y,z};
            const auto matrix=mat4_from_srt(rotation,scale,position);for(unsigned j=0;j<12;++j)p.matrix[j]=matrix.m[j];
            pose_.materials[index+1]=styles[glyph.model][row.kind<=3?row.kind:0];
        }
        if(!renderer_.load_scene(std::move(instances),error))return false;
        renderer_.set_interaction(false);loaded_=true;return true;
    }
    bool begin(){if(!loaded_||!timeline_.begin(profile_.config))return false;return publish();}
    bool step(){if(!loaded_||!timeline_.running())return false;if(!timeline_.step())return false;return publish();}
    void cancel(){timeline_.cancel();}
    bool running()const{return timeline_.running();}
    bool loaded()const{return loaded_;}
    const CreditsConfig& config()const{return profile_.config;}
    const CreditsTimeline& timeline()const{return timeline_;}
    const HsdAnimPose& pose()const{return pose_;}
    unsigned upload_textures(RenderEngine& render){return renderer_.upload_textures(render,0x30000);}
    unsigned draw(RenderEngine& render,const Camera& camera){return loaded_&&timeline_.running()?renderer_.draw(render,camera):0;}
private:
    bool publish(){
        HsdAnimPose next=pose_;next.joints[0].hidden=!timeline_.running();
        for(std::size_t row=0;row<profile_.config.rows.size();++row){const auto& record=profile_.config.rows[row];const auto& state=timeline_.rows()[row];
            for(unsigned g=0;g<record.count;++g){auto& joint=next.joints[record.first+g+1];joint.hidden=!state.alive||!timeline_.running();joint.matrix[7]=state.y;}}
        if(!renderer_.set_pose(&next))return false;
        pose_=std::move(next);return true;
    }
    bool loaded_=false;CreditsProfile profile_;CreditsTimeline timeline_;HsdAnimPose pose_;MenuScene renderer_;
};
} // namespace gf
