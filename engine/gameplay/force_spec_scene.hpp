#pragma once
#include <map>
#include "engine/gameplay/force_spec_widgets.hpp"
#include "engine/gameplay/animated_model_scene.hpp"
#include "engine/assets/source_messages.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/graphics/depth_clear.hpp"
namespace gf {
// Original specification UI component. The full owner draws its separately
// qualified actor between background and foreground; no placeholder actor here.
class ForceSpecScene {
public:
    const SceneLighting* lighting()const{return ready_?bank_.lighting():nullptr;}
    bool load(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,RenderEngine& render,std::string* error){
        ready_=false;models_.clear();empty_.clear();name_texture_=0;art_=art;messages_=messages;horizon_fog_={};marker_={};
        if(art.widgets.empty()||!messages.hash()||messages.hash()!=art.messages_hash||!bank_.load(bank,error,true)||bank_.hash()!=art.bank_hash||!text_.load(font,render,6000))return false;
        for(const auto& widget:art.widgets){const u16 last=u16(widget.record+(widget.type==8&&widget.index>1?1:widget.type==18?1:0));for(u16 id=widget.record;id<=last;++id){if(models_.contains(id)||empty_.contains(id))continue;if(bank_.is_empty_static_record(id)){empty_[id]=true;continue;}AnimatedModelScene model;if(!model.load_compact_textures(bank_,id,2,0,0,error)||!model.request(0,0,error))return false;model.set_enabled(true);model.set_untextured_billboards(true);model.upload_textures_shared(render,20000);models_.emplace(id,std::move(model));}}
        if(render.shared_texture_upload_failed())return false;
        ready_=true;return true;
    }
    // Source type0/type1 objects clear byte11 so the current scene FogDesc is
    // bound by09838. This is explicit; other widgets/actors remain unchanged.
    bool load_with_horizon_fog(ByteView bank,const ForceSpecArt& art,const SourceMessageBank& messages,const SourceFont& font,RenderEngine& render,std::string* error){
        if(!load(bank,art,messages,font,render,error))return false;
        LinearFog fog;if(!bank_.read_linear_fog(&fog,error)||!fog.enabled){ready_=false;return false;}
        std::vector<u16> floor;
        for(const auto& widget:art_.widgets)if(widget.type<=1){if(!models_.contains(widget.record)){ready_=false;return false;}if(std::find(floor.begin(),floor.end(),widget.record)==floor.end())floor.push_back(widget.record);}
        if(floor.empty()){ready_=false;return false;}
        for(const auto& widget:art_.widgets)if(widget.type>1&&std::find(floor.begin(),floor.end(),widget.record)!=floor.end()){ready_=false;return false;}
        for(auto record:floor)if(!models_.at(record).set_linear_fog(fog)){ready_=false;return false;}
        horizon_fog_=fog;return true;
    }
    const LinearFog* horizon_fog()const{return ready_&&horizon_fog_.enabled?&horizon_fog_:nullptr;}
    bool bind_nameplate(u16 model,ByteView tpl,RenderEngine& render,std::string* error){
        name_texture_=0;TplImage image;if(!ready_||model>=0x8000||!tpl_decode(tpl,0,&image,error)||image.width!=224||image.height!=32)return false;
        const u32 id=30000+model;if(!render.upload_texture_shared(id,{image.width,image.height,image.rgba.data(),0,0,true}))return false;name_model_=model;name_texture_=id;return true;
    }
    // Policy9/8008D5D0: UI camera1/group2, clear, actor camera2/group0,
    // clear, UI camera1/group3, clear. The actor owner fills the middle stage.
    bool draw_background(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error){return draw_layer(render,input,2,error)&&draw_depth_clear(render,640,480);}
    bool draw_foreground(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error){
        return draw_foreground_impl(render,input,error,false);
    }
    //Native actor-free edit mode retains original statistics but has no page or
    //actor controls. Default source widgets and their pose kernel stay unchanged.
    bool draw_borg_edit_foreground(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error){
        return input.page==ForceSpecPage::Statistics&&draw_foreground_impl(render,input,error,true);
    }
    //Original pages without a qualified actor. Keep page arrows; do not advertise
    //the source actor rotation/reset/demonstration footer on this explicit path.
    bool draw_stats_ui_foreground(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error){return draw_foreground_impl(render,input,error,false,true);}
    static bool draw_backdrop(RenderEngine& render,Tint colour){
        const auto saved=render.state();render.state()={};render.state().blend=colour.a!=255;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;
        if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
        std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,0,0,640,480,0,0,0,1,1,colour);const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;return ok;
    }
    //Native edit highlight follows actual source label/bar/digit geometry. No
    //camera fit or second stats layout; addon-only fields use explicit text slots.
    bool borg_edit_bounds(const ForceSpecWidgetInput& input,unsigned field,std::array<f32,4>* out){
        if(!out||field<1||field>7)return false;
        *out={1e6f,1e6f,-1e6f,-1e6f};bool found=false;f32 row_y=0;
        if(field>=3){const u16 record=u16(field+11);bool label=false;for(const auto& w:art_.widgets)if(w.record==record&&w.type==10){row_y=w.position[1];label=true;break;}if(!label)return false;}
        for(unsigned i=0;i<art_.widgets.size();++i){const auto& w=art_.widgets[i];const bool wanted=field==1?w.type==5:field==2?w.type==6:(w.type==10&&w.position[1]==row_y)||(field==3?w.type==12:w.type==14&&w.index==s8(field-4));if(!wanted)continue;ForceSpecWidgetPose pose;if(!force_spec_widget_pose(art_,input,i,&pose))return false;if(!pose.visible)continue;std::array<f32,4> bounds;if(!widget_bounds(pose,&bounds))return false;for(unsigned k=0;k<2;++k){(*out)[k]=std::min((*out)[k],bounds[k]);(*out)[k+2]=std::max((*out)[k+2],bounds[k+2]);}found=true;}
        return found;
    }
private:
    bool draw_foreground_impl(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error,bool editing,bool ui_only=false){
        if(input.component||!name_texture_||name_model_!=input.borg.model||!validate_text(input))return false;
        if(!draw_depth_clear(render,640,480)||!draw_layer(render,input,3,error,editing,ui_only)||!draw_depth_clear(render,640,480)||!nameplate(render))return false;
        const auto& b=input.borg;
        if(input.page==ForceSpecPage::Statistics){
            const auto label=[&](const std::string& s,f32 y){return text_.draw_screen(render,s,512-f32(s.size())*2,y,24*.8f,8,1,480.f/448,art_.colours[0][1],art_.colours[0][0]);};
            if(!label(art_.tribes[b.tribe],96)||!label(art_.jumps[b.jump_type],129))return false;
            if(b.feature>=0&&!label(art_.features[unsigned(b.feature)],397))return false;
        }else if(input.page==ForceSpecPage::Description){if(!lines(render,*messages_.message(0,b.number),375,135,1,10,1))return false;}
        else if(input.page==ForceSpecPage::Moves){for(unsigned i=0;i<b.move_count;++i)if(!lines(render,*messages_.message(1,b.moves[i].text_index),416,121+67.f*i,.85f,8.5f,0))return false;}
        if(input.edited&&!edited_marker(render,input,error))return false;
        return true;
    }
private:
    struct EditedMarker {bool ready=false;u32 texture=0;std::array<std::array<f32,3>,4> local{};};
    bool prepare_edited_marker(RenderEngine& render,std::string* error){
        if(marker_.ready)return true;
        //Original LEVEL word: GFSA type6 label record8. Its first E is the
        //source atlas crop [74,181..84,196), not the unrelated tall message font.
        HsdRecordAnimationBinding binding;if(!bank_.select_compact_textures(8,2,0,0,&binding,error)||!binding.scene())return false;const auto& source=*binding.scene();
        if(source.joints.size()!=2||source.meshes.size()!=2||source.mesh_joint.size()!=2||source.joints[0].parent!=kHsdNoJoint||source.joints[1].parent!=0||source.mesh_joint[0]!=0||source.mesh_joint[1]!=1)return false;
        //The baked word is on the original identity child of the plate root.
        //Only the source root is replaced by the GFSA widget world matrix.
        const auto& child=source.joints[1];for(unsigned i=0;i<3;++i)if(child.translation[i]!=0||child.rotation[i]!=0||child.scale[i]!=1)return false;
        const auto& word=source.meshes[1];if(word.texture>=source.textures.size()||word.vertices.size()!=4)return false;const auto& image=source.textures[word.texture];const auto& pixels=image.pixels();
        if(image.width!=512||image.height!=256||pixels.size()!=512*256*4)return false;
        constexpr unsigned x0=74,y0=181,width=10,height=15;std::vector<u8> mask(width*height*4);bool ink=false;
        for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){const auto at=((y0+y)*image.width+x0+x)*4,to=(y*width+x)*4;if(pixels[at]!=pixels[at+1]||pixels[at]!=pixels[at+2])return false;mask[to]=mask[to+1]=mask[to+2]=255;mask[to+3]=u8((unsigned(pixels[at])*pixels[at+3]+127)/255);ink|=mask[to+3]!=0;}
        if(!ink)return false;
        //Derive glyph dimensions from the SAME original word plane/UV mapping.
        f32 u0=1e6f,u1=-1e6f,v0=1e6f,v1=-1e6f,left=1e6f,right=-1e6f,top=-1e6f,bottom=1e6f,z=word.vertices[0].position[2];
        for(const auto& v:word.vertices){u0=std::min(u0,v.uv[0]);u1=std::max(u1,v.uv[0]);v0=std::min(v0,v.uv[1]);v1=std::max(v1,v.uv[1]);left=std::min(left,v.position[0]);right=std::max(right,v.position[0]);top=std::max(top,v.position[1]);bottom=std::min(bottom,v.position[1]);if(std::abs(v.position[2]-z)>.001f)return false;}
        if(!(u0<u1&&v0<v1&&left<right&&bottom<top)||x0<image.width*u0||x0+width>image.width*u1||y0<image.height*v0||y0+height>image.height*v1)return false;
        const auto point=[&](f32 u,f32 v){return std::array<f32,3>{left+(u-u0)/(u1-u0)*(right-left),top-(v-v0)/(v1-v0)*(top-bottom),z};};
        for(const auto& v:word.vertices){const auto p=point(v.uv[0],v.uv[1]);if(std::abs(p[0]-v.position[0])>.001f||std::abs(p[1]-v.position[1])>.001f)return false;}
        EditedMarker next;next.local={point(f32(x0)/image.width,f32(y0)/image.height),point(f32(x0+width)/image.width,f32(y0)/image.height),point(f32(x0+width)/image.width,f32(y0+height)/image.height),point(f32(x0)/image.width,f32(y0+height)/image.height)};next.texture=0x24000;
        if(!render.upload_texture_shared(next.texture,{width,height,mask.data(),0,0,true}))return false;
        next.ready=true;marker_=next;return true;
    }
    bool edited_marker(RenderEngine& render,const ForceSpecWidgetInput& input,std::string* error){
        if(!prepare_edited_marker(render,error))return false;
        ForceSpecWidgetPose label;bool found=false;
        for(unsigned i=0;i<art_.widgets.size();++i)if(art_.widgets[i].type==6&&art_.widgets[i].index==-1&&art_.widgets[i].record==8){if(!force_spec_widget_pose(art_,input,i,&label))return false;found=true;break;}
        if(!found)return false;
        std::array<f32,4> glyph={1e6f,1e6f,-1e6f,-1e6f};
        for(const auto& local:marker_.local){f32 world[3],x=0,y=0;mat4_transform_point(label.world,local.data(),world);if(!art_.camera.project(world,&x,&y,nullptr))return false;glyph[0]=std::min(glyph[0],x);glyph[1]=std::min(glyph[1],y);glyph[2]=std::max(glyph[2],x);glyph[3]=std::max(glyph[3],y);}
        std::array<f32,4> cost;if(!borg_edit_bounds(input,1,&cost))return false;const f32 width=glyph[2]-glyph[0],height=glyph[3]-glyph[1],x=cost[0]-7-width,y=glyph[1];if(width<=0||height<=0||width>64||height>64||x<3||y<3)return false;
        const auto saved=render.state();render.state()={};render.state().texture=marker_.texture;render.state().texture_colour_lerp=true;render.state().texture_low_colour=render.state().texture_high_colour=0x000000ff;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;bool ok=render.upload_matrix(0,identity.m);std::vector<Vertex> outline;
        //Exactly a three-reference-pixel black outline, independent of source
        //atlas texel size. Same shared textured-quad submission as the menu.
        for(int dy=-3;dy<=3;++dy)for(int dx=-3;dx<=3;++dx)if(dx||dy)ScreenDraw::textured_quad(outline,x+dx,y+dy,x+width+dx,y+height+dy,0,0,0,1,1,{255,255,255,255});
        if(ok){const auto at=render.append_vertices(outline.data(),outline.size());ok=render.draw(Primitive::Triangles,at,u32(outline.size()));}
        if(ok){render.state().texture_high_colour=0xff8000ff;std::vector<Vertex> fill;ScreenDraw::textured_quad(fill,x,y,x+width,y+height,0,0,0,1,1,{255,255,255,255});const auto at=render.append_vertices(fill.data(),fill.size());ok=render.draw(Primitive::Triangles,at,u32(fill.size()));}render.state()=saved;return ok;
    }
    bool widget_bounds(const ForceSpecWidgetPose& pose,std::array<f32,4>* out){
        const auto it=models_.find(pose.record);if(it==models_.end())return false;auto& model=it->second;f32 lo[3],hi[3];if(!model.request_again(pose.frame,0,nullptr)||!model.set_root_world(pose.world,nullptr)||!model.bounds(lo,hi))return false;*out={1e6f,1e6f,-1e6f,-1e6f};
        //The shared bounds accessor is already evaluated WORLD geometry.
        for(unsigned i=0;i<8;++i){const f32 world[3]={i&1?hi[0]:lo[0],i&2?hi[1]:lo[1],i&4?hi[2]:lo[2]};f32 x=0,y=0;if(!art_.camera.project(world,&x,&y,nullptr))return false;(*out)[0]=std::min((*out)[0],x);(*out)[1]=std::min((*out)[1],y);(*out)[2]=std::max((*out)[2],x);(*out)[3]=std::max((*out)[3],y);}
        return true;
    }
    bool validate_text(const ForceSpecWidgetInput& in)const{
        if(!ready_||in.component||in.borg.tribe>=art_.tribes.size()||in.borg.jump_type>=art_.jumps.size()||in.borg.feature<-1||in.borg.feature>=s8(art_.features.size())||in.borg.move_count>5||!messages_.message(0,in.borg.number))return false;
        for(unsigned i=0;i<in.borg.move_count;++i)if(!messages_.message(1,in.borg.moves[i].text_index))return false;
        return true;
    }
    bool draw_layer(RenderEngine& render,const ForceSpecWidgetInput& in,u8 layer,std::string* error,bool editing=false,bool ui_only=false){
        if(!ready_||render.model_pass())return false;
        std::vector<ForceSpecWidgetPose> poses;for(unsigned i=0;i<art_.widgets.size();++i){ForceSpecWidgetPose pose;if(!force_spec_widget_pose(art_,in,i,&pose))return false;if((editing&&(art_.widgets[i].type==7||art_.widgets[i].type==8))||(ui_only&&art_.widgets[i].type==8))continue;if(pose.visible&&pose.layer==layer&&!empty_.contains(pose.record)){if(!models_.contains(pose.record))return false;poses.push_back(pose);}}
        for(u8 pass:{u8(1),u8(4),u8(2)}){if(!render.begin_model_pass(pass))return false;bool ok=true;for(const auto& pose:poses){auto& model=models_.at(pose.record);if(!model.request_again(pose.frame,0,error)||!model.set_root_world(pose.world,error)){ok=false;break;}model.draw(render,art_.camera);}if(!ok)render.reject_model_pass();const bool ended=render.end_model_pass();if(!ok||!ended)return false;}return true;
    }
    bool lines(RenderEngine& render,const SourceMessageBank::Lines& lines,f32 x,f32 y,f32 scale,f32 advance,unsigned style){for(const auto& line:lines){if(!text_.draw_screen(render,line,x,y,24*scale,advance,1,480.f/448,art_.colours[style][1],art_.colours[style][0]))return false;y+=24*scale;}return true;}
    bool nameplate(RenderEngine& render){
        const auto saved=render.state();render.state()={};render.state().texture=name_texture_;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;render.state().texture_colour_lerp=true;render.state().texture_low_colour=art_.colours[2][1];render.state().texture_high_colour=art_.colours[2][0];Mat4 identity;if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
        std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,140,32*480.f/448,364,64*480.f/448,0,0,0,1,1,{255,255,255,255});const auto first=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,first,u32(vertices.size()));render.state()=saved;return ok;
    }
    bool ready_=false;u16 name_model_=0;u32 name_texture_=0;HsdRecordAnimationBank bank_;ForceSpecArt art_;SourceMessageBank messages_;SourceTextRenderer text_;std::map<u16,AnimatedModelScene> models_;std::map<u16,bool> empty_;LinearFog horizon_fog_;EditedMarker marker_;
};
} // namespace gf
