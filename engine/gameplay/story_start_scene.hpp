#pragma once
#include <map>
#include "engine/gameplay/story_start_visual.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/graphics/source_fixed_text_metrics.hpp"
#include "engine/graphics/convex_text_layout.hpp"
#include "engine/graphics/alpha_text_region.hpp"
#include "engine/graphics/projected_triangle.hpp"
#include "engine/graphics/source_fixed_paragraph.hpp"
namespace gf {
//Original Story starting widgets through the existing record/model/text renderer.
//No input/storage/resource/fade clock is advanced by draw; no new rasterizer.
class StoryStartScene {
public:
    bool load(ByteView archive,const StoryStartArt& art,const SourceFont& font,RenderEngine& render,std::string* error){
        *this={};if(!art.valid(font)||!bank_.load(archive,error,true)||bank_.hash()!=art.bank_hash||bank_.records()!=11)return fail(error,"art/font/published bank identity");
        art_=art;
        if(!text_.load(font,render,90000)||!metrics_.load(font))return fail(error,"source font upload/ink measurement");
        //All eleven source records are single-root rigid instances. Preparing
        //the immutable banks does not manufacture any source widget callbacks.
        for(unsigned record=0;record<11;++record){
            const bool animated=record==0||(record>=3&&record<=5)||(record>=7&&record<=9);
            Cached model;model.animated=animated;HsdRecordAnimationBinding binding;
            if(!bank_.select_compact_retained_tev1_textures(u16(record),animated?3:0,0,0,&binding,error)||
               !binding.scene()||binding.scene()->joints.size()!=1||binding.scene()->meshes.size()!=1||
               !model.sampler.bind(binding,error)||!model.scene.load_compact_retained_tev1_textures(bank_,u16(record),animated?3:0,0,0,error))return fail(error,"complete record binding/one-root source policy");
            model.scene.set_enabled(true);model.scene.upload_textures_shared(render,50000);
            models_.emplace(u16(record),std::move(model));
        }
        if(render.shared_texture_upload_failed())return fail(error,"source texture upload");
        ready_=true;if(error)error->clear();return true;
    }
    bool draw(RenderEngine& render,const StoryStartVisual& visual,std::string* error){
        if(!ready_||!visual.ready()||visual.bank_hash()!=art_.bank_hash||render.model_pass())return fail(error,"live visual/bank/pass required");
        auto poses=visual.poses();if(poses.size()>24)return fail(error,"source packet budget");
        //Prepare each packet outside global passes; source record10 has two
        //independent arrow worlds, so its world is applied again at submission.
        std::vector<Mat4> worlds;std::vector<bool> visible;worlds.reserve(poses.size());visible.reserve(poses.size());
        for(const auto& pose:poses){
            const auto found=models_.find(pose.record);if(found==models_.end()||pose.animated!=found->second.animated||!std::isfinite(pose.frame)||pose.frame<0||pose.frame>20)return fail(error,"packet record/selector");
            auto& m=found->second;
            if(!m.sampled){if(!m.sampler.request(pose.frame,0,error))return fail(error,"first source callback sample");m.sampled=true;m.frame=pose.frame;}
            else if(m.frame!=pose.frame){if(!m.sampler.request_again(pose.frame,0,error))return fail(error,"source callback re-request");m.frame=pose.frame;}
            const auto* sample=m.sampler.pose();if(!sample||sample->joints.size()!=1)return fail(error,"source root pose");
            const auto& j=sample->joints[0];
            const bool zero=j.scale[0]==0||j.scale[1]==0||j.scale[2]==0;
            //At requested0 the two source panels have exactly zero height and
            //therefore no triangles. Omit that exact instance, not an arbitrary
            //mesh/failed pose, and do not weaken AnimatedModelScene's guard.
            if(zero&&!(pose.frame==0&&(pose.record==0||pose.record==7)&&j.scale[0]==1&&j.scale[1]==0&&j.scale[2]==1))return fail(error,"unrepresented singular source scale");
            visible.push_back(!zero);Mat4 world;
            if(!zero){
                for(f32 value:pose.position)if(!std::isfinite(value)||std::abs(value)>10000)return fail(error,"source position");
                const f32 angle=std::bit_cast<s16>(pose.angle_z)*art_.angle_unit;
                world=mat4_multiply(mat4_translation(pose.position.data()),mat4_multiply(mat4_rotation_z(angle),mat4_scale(j.scale)));
                if(!m.requested){if(!m.scene.request(pose.frame,0,error))return fail(error,"first drawable source pose");m.requested=true;m.draw_frame=pose.frame;}
                else if(m.draw_frame!=pose.frame){if(!m.scene.request_again(pose.frame,0,error))return fail(error,"drawable source re-request");m.draw_frame=pose.frame;}
            }
            worlds.push_back(world);
        }
        //Owner-requested native layout supersedes fixed source footer anchors:
        //actual projected rounded geometry, centered ink group and centered
        //individual lines. One glyph-cell line step and one-third-cell inset
        //are explicit native text-layout policy, not recovered game constants.
        FixedParagraphLayout paragraph;ConvexTextRegion region;
        const f32 sx=art_.camera.viewport_width/art_.text_reference[0],sy=art_.camera.viewport_height/art_.text_reference[1];
        if(visual.footer_visible()){
            const auto selection=visual.selection();if(selection<0||selection>=3)return fail(error,"source footer selection");
            std::size_t footer=poses.size();for(unsigned i=0;i<poses.size();++i)if(poses[i].record==6){if(footer!=poses.size())return fail(error,"unique textbox required");footer=i;}
            if(footer==poses.size()||!visible[footer])return fail(error,"visible projected textbox required");
            const auto* asset=models_.at(6).scene.asset();if(!asset||asset->meshes.size()!=1)return fail(error,"source textbox geometry");
            const auto& mesh=asset->meshes[0];
            if(!coverage_ready_||!std::equal(worlds[footer].m,worlds[footer].m+16,coverage_world_.m)){
                if(mesh.texture>=asset->textures.size()||mesh.texture_alpha_mode!=3||mesh.material_alpha!=1||mesh.texture_uv1||mesh.texture_stage_inactive||mesh.texture_coordinate_mode||mesh.indices.size()%3)return fail(error,"single static source-alpha textbox required");
                std::vector<Vertex> projected;
                for(unsigned at=0;at<mesh.indices.size();at+=3){Vertex triangle[3];
                    for(unsigned k=0;k<3;++k){const auto index=mesh.indices[at+k];if(index>=mesh.vertices.size())return fail(error,"textbox vertex identity");const auto& v=mesh.vertices[index];if(v.colour[3]!=255)return fail(error,"textbox vertex-alpha owner");f32 world[3];mat4_transform_point(worlds[footer],v.position,world);triangle[k].x=world[0];triangle[k].y=world[1];triangle[k].z=world[2];triangle[k].u=v.uv[0];triangle[k].v=v.uv[1];}
                    project_triangle(art_.camera,triangle,projected);
                }
                std::vector<Vertex> submitted;
                for(unsigned at=0;at<projected.size();at+=3){const auto& a=projected[at];const auto& b=projected[at+1];const auto& c=projected[at+2];const f64 area=(f64(b.x)-a.x)*(f64(c.y)-a.y)-(f64(b.y)-a.y)*(f64(c.x)-a.x);if(!area||(mesh.cull_backface&&area<0)||(mesh.cull_frontface&&area>0))continue;submitted.insert(submitted.end(),projected.begin()+at,projected.begin()+at+3);}
                const auto& texture=asset->textures[mesh.texture];const auto& pixels=texture.pixels();
                if(!coverage_.load(submitted,{texture.width,texture.height,pixels.data(),texture.wrap_s,texture.wrap_t,texture.linear},{pixels.data(),pixels.size()}))return fail(error,"actual texture-alpha textbox coverage");
                if(coverage_.hull().hull().size()<=4)return fail(error,"rounded textbox alpha must not collapse to rectangular bounds");
                coverage_world_=worlds[footer];coverage_ready_=true;
            }
            region=coverage_.hull();
            const f32 extent=text_.font().cell()*art_.text_scale;
            if(!layout_fixed_paragraph(art_.descriptions[unsigned(selection)],text_.font().blank(),metrics_,coverage_,extent,art_.text_advance,extent,sx,sy,(extent/3)*std::min(sx,sy),&paragraph))return fail(error,"word-preserving description does not fit rounded textbox");
        }
        for(u8 pass:{u8(1),u8(4),u8(2)}){
            if(!render.begin_model_pass(pass))return fail(error,"global source pass");
            bool ok=true;
            //097B4 prepends submissions; preserve source queue order.
            for(std::size_t at=poses.size();at>0;--at){const auto i=at-1;if(!visible[i])continue;auto& model=models_.at(poses[i].record).scene;
                if(!model.set_root_world(worlds[i],error)){ok=false;break;}model.draw(render,art_.camera);}
            if(!ok)render.reject_model_pass();
            const bool ended=render.end_model_pass();if(!ok||!ended)return fail(error,"shared model submission");
        }
        for(const auto& line:paragraph.lines)if(!text_.draw_screen(render,paragraph.text[line.line],line.x/sx,line.y/sy,text_.font().cell()*art_.text_scale,art_.text_advance,sx,sy,art_.text_colours[1],art_.text_colours[0]))return fail(error,"source description glyph submission");
        description_=std::move(paragraph.lines);description_text_=std::move(paragraph.text);description_reflowed_=paragraph.reflowed;region_=std::move(region);
        if(error)error->clear();
        return true;
    }
    bool ready()const{return ready_;}std::size_t prepared_records()const{return models_.size();}
    const std::vector<CenteredTextLine>& description_layout()const{return description_;}
    const ConvexTextRegion& description_region()const{return region_;}
    const AlphaTextRegion& description_coverage()const{return coverage_;}
    const std::vector<std::string>& description_text()const{return description_text_;}
    bool description_reflowed()const{return description_reflowed_;}
private:
    struct Cached {AnimatedModelScene scene;HsdRecordAnimationPlayer sampler;f32 frame=0,draw_frame=0;bool animated=false,sampled=false,requested=false;};
    bool fail(std::string* error,const char* why){ready_=false;description_.clear();description_text_.clear();description_reflowed_=false;region_={};if(error&&error->empty())*error=std::string("Story start scene: ")+why;return false;}
    HsdRecordAnimationBank bank_;StoryStartArt art_;SourceTextRenderer text_;SourceFixedTextMetrics metrics_;ConvexTextRegion region_;AlphaTextRegion coverage_;Mat4 coverage_world_;std::vector<CenteredTextLine> description_;std::vector<std::string> description_text_;std::map<u16,Cached> models_;bool ready_=false,coverage_ready_=false,description_reflowed_=false;
};
} //namespace gf
