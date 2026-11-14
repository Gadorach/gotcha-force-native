#pragma once
#include "engine/assets/option_widgets.hpp"
#include "engine/assets/hsd_leaf_material.hpp"
#include "engine/gameplay/option_settings.hpp"
#include "engine/gameplay/menu_scene.hpp"
#include "engine/graphics/source_text.hpp"
#include "engine/graphics/affine.hpp"
#include "engine/gameplay/cheat_descriptions.hpp"
#include "engine/gameplay/option_mouse_geometry.hpp"
namespace gf {
enum class OptionPresentationPage {Settings,More,Cheats};
enum class OptionWordNotice {Saved,SaveError};
struct OptionWordPlacement {std::string ascii,encoded;unsigned row=0;Mat4 world;std::array<f32,4> plane_bounds{};f32 extent=32,horizontal_scale=1;u32 colour=0;};
// Original widget operations: 801D0434/0570/0764/08F0/0B1C/0D20/0E2C.
// Instances use content-bound original meshes; no hand-drawn replacement bars.
class OptionScene {
public:
    bool load(ByteView archive,ByteView content,std::string* error){
        loaded_=false;pages_ready_=word_pages_=mouse_enabled_=false;mouse_geometry_.clear();words_.clear();instances_.clear();clips_.clear();pose_={};
        HsdScene source;if(!hsd_load(archive,&source,error)||!option_widget_profile_load(content,archive,source,&profile_,error)||!hsd_share_texture_images(source))return false;
        for(const auto& binding:source.material_animation_roots){HsdLeafMaterialClip clip;
            if(!hsd_leaf_material_clip_load(archive,source,binding.second,&clip,error))return false;
            for(u32 time=0;time<=5;++time){HsdAnimPose::Material sample;
                if(!hsd_leaf_material_sample(clip,time,&sample,error))return false;}
            clips_.push_back(std::move(clip));}
        HsdScene instances;instances.textures=source.textures;
        for(const auto& w:profile_.widgets)for(unsigned part=0;part<w.count;++part){
            const u32 root=part?w.secondary:w.primary;std::size_t m=0;
            for(;m<source.meshes.size();++m)if(source.joints[source.mesh_joint[m]].archive_offset==root)break;
            if(m==source.meshes.size())return false; // profile loader already proves it
            const auto& original=source.joints[source.mesh_joint[m]];
            Instance item;item.widget=w;item.part=part;
            item.authored=mat4_from_srt(original.rotation,original.scale,original.translation);
            item.local_left=1e6f;for(const auto& vertex:source.meshes[m].vertices)item.local_left=std::min(item.local_left,vertex.position[0]);
            for(std::size_t i=0;i<clips_.size();++i)if(clips_[i].joint==root)item.clip=int(i);
            if((w.kind==3||w.kind==5)&&item.clip<0){if(error)*error="option scene: missing required material clip";return false;}
            instances_.push_back(item);instances.mesh_joint.push_back(u32(instances.joints.size()));
            instances.joints.push_back(original);instances.joints.back().parent=kHsdNoJoint;
            instances.meshes.push_back(source.meshes[m]);
        }
        if(!renderer_.load_scene(std::move(instances),error))return false;
        renderer_.set_interaction(false);pose_.joints.resize(instances_.size());pose_.materials.resize(instances_.size());
        cursor_phase_=0;loaded_=true;
        if(!update(0,OptionSettings{},false,error)){loaded_=false;return false;}
        return true;
    }
    bool configure_pages(const SourceFont& font,RenderEngine& render,std::string* error){
        pages_ready_=word_pages_=mouse_enabled_=false;mouse_geometry_.clear();words_.clear();if(!loaded_||!font.ready()||!text_.load(font,render,0x21000))return false;
        for(std::string_view label:{"MORE","CHEATS","PRESS Y FOR DETAILS","A OR B TO RETURN"}){std::string encoded;if(!font.encode_ascii(label,&encoded))return false;}
        for(const auto& info:kCheatDescriptions){std::string encoded;if(!font.encode_ascii(info.label,&encoded))return false;for(auto line:info.lines)if(!font.encode_ascii(line,&encoded))return false;}
        for(unsigned i=0;i<instances_.size();++i)if(instances_[i].widget.kind==1&&instances_[i].widget.id==3){label_world_=instances_[i].authored;label_left_=instances_[i].local_left;pages_ready_=true;break;}
        if(!pages_ready_&&error)*error="option pages: missing source label anchor";
        return pages_ready_;
    }
    //Owner correction: EVERY word uses the sampled TV font in explicit source
    //slots. The old widget/default text path remains an independent entry.
    bool configure_word_pages(const SourceFont& font,const SourceTextMetrics& metrics,RenderEngine& render,std::string* error){
        if(!configure_pages(font,render,error))return false;
        for(std::string_view label:{"SOUND VOLUME RUMBLE MORE RESTORE DEFAULT SETTINGS","STEREO / MONO","BGM SFX VOICE","1234P ON / OFF","SETTINGS SAVED","SETTINGS SAVE FAILED","A / SPACE RETURN TO MAIN MENU","A RETRY / B RETURN"}){std::string encoded;if(!font.encode_ascii(label,&encoded)){pages_ready_=false;return word_fail(error,"incomplete wording font");}}
        pages_ready_=false;if(!text_.load_measured(font,metrics,render,0x21000)||!word_span(1,3,0,&primary_)||!word_span(1,4,0,&wide_)||!word_span(4,0,0,&secondary_)||!word_span(5,0,0,&sound_[0])||!word_span(5,0,1,&sound_[1])||!word_span(1,0,1,&sound_slash_)||!word_span(4,3,0,&player_)||!word_span(5,1,0,&on_)||!word_span(5,1,1,&off_)||!word_span(4,3,1,&rumble_slash_))return word_fail(error,"font or original slot geometry");
        WordSpan bars;if(!word_span(3,0,0,&bars))return word_fail(error,"original volume bar boundary");
        primary_.right=std::min(primary_.right,secondary_.left-8);secondary_.right=std::min(secondary_.right,bars.left-8);
        if(primary_.right<=primary_.left||secondary_.right<=secondary_.left)return word_fail(error,"column separation");
        word_pages_=pages_ready_=true;return true;
    }
    const std::vector<OptionWordPlacement>& word_plan()const{return words_;}
    bool enable_mouse(bool enabled){mouse_geometry_.clear();mouse_enabled_=enabled&&loaded_&&pages_ready_&&word_pages_;return !enabled||mouse_enabled_;}
    bool mouse_hit(f32 x,f32 y,OptionMouseArea area,OptionMouseTarget* out)const{if(out)*out={};return mouse_enabled_&&mouse_geometry_.hit(x,y,area,out);}
    const std::vector<OptionMouseRegion>& mouse_regions()const{return mouse_geometry_.regions();}
    bool update(unsigned row,const OptionSettings& settings,bool advance,std::string* error,bool cursor_visible=true,bool selection_visible=true){
        return update_impl(row,settings,advance,error,cursor_visible,selection_visible,OptionPresentationPage::Settings,false,{});
    }
    bool update_page(OptionPresentationPage page,unsigned row,const OptionSettings& settings,std::array<bool,2> cheats,bool advance,std::string* error){
        if(!pages_ready_||(page==OptionPresentationPage::More&&row>=2)||(page==OptionPresentationPage::Cheats&&row>=7)||(page!=OptionPresentationPage::Settings&&page!=OptionPresentationPage::More&&page!=OptionPresentationPage::Cheats))return false;
        return update_impl(row,settings,advance,error,true,true,page,true,cheats);
    }
private:
    bool update_impl(unsigned row,const OptionSettings& settings,bool advance,std::string* error,bool cursor_visible,bool selection_visible,OptionPresentationPage page,bool extensions,std::array<bool,2> cheats){
        if(mouse_enabled_)mouse_geometry_.clear();
        if(!loaded_||row>=10||!option_settings_valid(settings))return false;
        const u16 phase=advance?u16(u32(cursor_phase_)+u32(profile_.cursor_step)):cursor_phase_;
        HsdAnimPose candidate=pose_;
        for(std::size_t i=0;i<instances_.size();++i){const auto& item=instances_[i];const auto& w=item.widget;
            auto& joint=candidate.joints[i];joint.hidden=!selection_visible&&(w.kind==0||w.kind==2);
            if(extensions){
                if(page==OptionPresentationPage::Settings&&w.kind==1&&w.id==3)joint.hidden=true;
                if(page==OptionPresentationPage::More)joint.hidden=!(w.kind==0||w.kind==2||(w.kind==1&&w.id==3));
                if(page==OptionPresentationPage::Cheats)joint.hidden=!(w.kind==0||w.kind==2||(w.kind==5&&w.id>=1&&w.id<=2));
                if(word_pages_&&(w.kind==1||w.kind==4||w.kind==5))joint.hidden=true;
            }
            Mat4 matrix=item.authored;
            if(extensions&&page==OptionPresentationPage::More&&w.kind==1&&w.id==3)matrix.at(1,3)=profile_.row_y[1];
            if(w.kind==0||w.kind==2||w.kind==3||w.kind==5||item.part){
                f32 position[3]={w.position[0],w.position[1],w.position[2]};
                if(w.kind==0||w.kind==2)position[1]=profile_.row_y[row];
                if(extensions&&page==OptionPresentationPage::Cheats&&w.kind==5&&w.id>=1&&w.id<=2)position[1]=profile_.row_y[w.id-1];
                const f32 scale[3]={1,1,1};matrix=mat4_from_srt(w.rotation,scale,position);
                if(w.kind==2){
                    const auto& range=profile_.cursor_rows[w.id];joint.hidden=!selection_visible||!cursor_visible||row<range[0]||row>range[1];
                    // 80047C38 uses Ry * Rz * Rx, unlike 80047AA4's Rz * Ry * Rx.
                    matrix=mat4_multiply(mat4_translation(position),mat4_multiply(mat4_rotation_y(w.rotation[1]),
                        mat4_multiply(mat4_rotation_z(w.rotation[2]+phase*profile_.angle_unit),mat4_rotation_x(w.rotation[0]))));
                }
                if(w.kind==5&&item.part)for(unsigned k=0;k<3;++k)matrix.at(k,3)+=w.secondary_offset[k];
            }
            joint.matrix_override=true;for(unsigned k=0;k<12;++k)joint.matrix[k]=matrix.m[k];
            auto& material=candidate.materials[i];const auto& mesh=renderer_.scene().meshes[i];
            material={};material.present=true;material.alpha=mesh.material_alpha;
            for(unsigned k=0;k<3;++k){material.ambient[k]=mesh.material_ambient[k]/255.0f;material.diffuse[k]=mesh.material_colour[k]/255.0f;}
            if(item.clip>=0){
                u32 time=0;
                if(w.kind==3)time=settings.volume[w.id]+1;
                else if(w.kind==5){const unsigned choice=extensions&&page==OptionPresentationPage::Cheats&&w.id>=1&&w.id<=2?(cheats[w.id-1]?0u:1u):w.id==0?settings.mono:((settings.rumble_mask&(1u<<(w.id-1)))?0:1);
                    time=item.part==choice?1:0;}
                if(!hsd_leaf_material_sample(clips_[std::size_t(item.clip)],time,&material,error))return false;
            }
            if(w.kind==1||w.kind==4){
                bool selected=false;
                if(w.kind==1){const auto& range=profile_.group_rows[w.id];selected=row>=range[0]&&row<=range[1];}
                else selected=row==profile_.value_rows[w.id];
                if(extensions&&page==OptionPresentationPage::More&&w.kind==1&&w.id==3)selected=row==1;
                const u32 colour=selected&&selection_visible?profile_.selected_colour:profile_.normal_colour;
                for(unsigned k=0;k<3;++k)material.diffuse[k]=u8(colour>>(24-k*8))/255.0f;
            }
        }
        std::vector<OptionWordPlacement> words;if(extensions&&word_pages_&&!build_words(page,row,candidate,&words,error))return false;
        if(!renderer_.set_pose(&candidate))return false;
        pose_=std::move(candidate);words_=std::move(words);cursor_phase_=phase;page_=page;page_row_=row;extensions_=extensions;return true;
    }
public:
    unsigned upload_textures(RenderEngine& render){return renderer_.upload_textures(render,0x20000);}
    unsigned draw(RenderEngine& render,const Camera& camera){return loaded_?renderer_.draw(render,camera):0;}
    bool draw_page(RenderEngine& render,const Camera& camera,std::string* error,bool show_details_hint=true){
        if(mouse_enabled_)mouse_geometry_.clear();
        if(!loaded_||!pages_ready_||!extensions_||render.model_pass())return false;
        if(word_pages_){if(words_.empty())return word_fail(error,"missing prepared word plan");renderer_.draw(render,camera);for(const auto& word:words_){if(!show_details_hint&&page_==OptionPresentationPage::Cheats&&word.row==9)continue;if(!text_.draw_measured(render,camera,word.world,word.encoded,word.extent,{255,255,255,255},false,0x000000ff,word.colour))return word_fail(error,"word submission");}return !mouse_enabled_||mouse_page(camera,show_details_hint,error);}
        renderer_.draw(render,camera);
        if(page_==OptionPresentationPage::Settings)return label(render,camera,"MORE",8,page_row_==8,false,error);
        if(page_==OptionPresentationPage::More)return label(render,camera,"CHEATS",0,page_row_==0,false,error);
        for(unsigned i=0;i<kCheatDescriptions.size();++i)if(!label(render,camera,kCheatDescriptions[i].label,i,page_row_==i,!kCheatDescriptions[i].implemented,error))return false;
        return !show_details_hint||label(render,camera,"PRESS Y FOR DETAILS",9,true,false,error);
    }
    bool draw_details(RenderEngine& render,unsigned row,std::string* error){
        if(!pages_ready_||row>=kCheatDescriptions.size()||render.model_pass())return false;
        if(word_pages_)return draw_word_details(render,row,error);
        const auto saved=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;
        if(!render.upload_matrix(0,identity.m)){render.state()=saved;return false;}
        std::vector<Vertex> panel;ScreenDraw::textured_quad(panel,96,100,544,402,0,0,0,1,1,{24,31,48,250});const auto first=render.append_vertices(panel.data(),panel.size());const bool ok=render.draw(Primitive::Triangles,first,u32(panel.size()));render.state()=saved;if(!ok)return false;
        const auto& info=kCheatDescriptions[row];if(!detail_text(render,info.label,116,120,24,14,profile_.selected_colour,error))return false;
        for(unsigned i=0;i<info.lines.size();++i)if(!info.lines[i].empty()&&!detail_text(render,info.lines[i],116,174+28.f*i,18,10,0xffffffffu,error))return false;
        return detail_text(render,"A OR B TO RETURN",116,368,18,10,profile_.selected_colour,error);
    }
    bool draw_word_notice(RenderEngine& render,OptionWordNotice notice,std::string* error,OptionMouseArea mouse_area=OptionMouseArea::None){
        if(mouse_enabled_)mouse_geometry_.clear();
        if(!word_pages_||!pages_ready_||render.model_pass())return false;
        const bool saved=notice==OptionWordNotice::Saved;if(!saved&&notice!=OptionWordNotice::SaveError)return false;
        std::vector<ScreenWord> words;if(!screen_word(saved?"SETTINGS SAVED":"SETTINGS SAVE FAILED",145,198,350,30,24,profile_.selected_colour,&words,error)||!screen_word(saved?"A / SPACE RETURN TO MAIN MENU":"A RETRY / B RETURN",145,234,350,24,16,0xffffffffu,&words,error))return false;
        if(!word_panel(render,125,180,515,270))return false;
        if(!draw_screen_words(render,words,error))return false;
        if(mouse_enabled_){const auto area=saved?OptionMouseArea::Saved:mouse_area==OptionMouseArea::CheatSaveError?mouse_area:OptionMouseArea::SaveError;mouse_geometry_.begin(area);if(saved){if(!mouse_screen_part(words.back(),0,words.back().encoded.size()/2,{area,-1,-1,0x100}))return word_fail(error,"saved mouse footer");}else if(!mouse_screen_part(words.back(),0,7,{area,-1,-1,0x100})||!mouse_screen_part(words.back(),10,8,{area,-1,-1,0x200}))return word_fail(error,"error mouse footer");if(!mouse_geometry_.finish())return word_fail(error,"notice mouse geometry");}return true;
    }
    bool loaded()const{return loaded_;}
    const OptionWidgetProfile& profile()const{return profile_;}
    const HsdAnimPose& pose()const{return pose_;}
    const HsdScene& scene()const{return renderer_.scene();}
private:
    struct WordSpan {f32 left=0,right=0;};
    struct ScreenWord {std::string encoded;f32 x=0,y=0,extent=0,sx=1;u32 colour=0;};
    bool mouse_screen_part(const ScreenWord& word,std::size_t first,std::size_t count,const OptionMouseTarget& target){
        if(first+count>word.encoded.size()/2)return false;
        SourceTextMeasure prefix,part;if(!text_.measure(std::string_view(word.encoded).substr(0,first*2),word.extent,&prefix)||!text_.measure(std::string_view(word.encoded).substr(first*2,count*2),word.extent,&part)||!part.ink)return false;
        return mouse_geometry_.screen((word.x+prefix.advance+part.left)*word.sx,word.y+part.top,(word.x+prefix.advance+part.right)*word.sx,word.y+part.bottom,target);
    }
    bool mouse_page(const Camera& camera,bool details_hint,std::string* error){
        const auto area=page_==OptionPresentationPage::Settings?OptionMouseArea::Settings:page_==OptionPresentationPage::More?OptionMouseArea::More:OptionMouseArea::Cheats;mouse_geometry_.begin(area);
        for(const auto& word:words_){if(!details_hint&&page_==OptionPresentationPage::Cheats&&word.row==9)continue;OptionMouseTarget target{area,s16(word.row),-1,0};if(area==OptionMouseArea::Settings){if(word.row>=8)target.left_pressed=0x100;if(word.ascii=="STEREO"||word.ascii=="ON")target.value=0;else if(word.ascii=="MONO"||word.ascii=="OFF")target.value=1;}else if(area==OptionMouseArea::More)target.left_pressed=0x100;else if(word.row==9){target.row=-1;target.left_pressed=0x800;}else{target.left_pressed=0x100;if(word.ascii=="ON"||word.ascii=="OFF"){target.value=word.ascii=="ON"?0:1;target.left_pressed=0;}}
            SourceTextMeasure measure;if(!text_.measure(word.encoded,word.extent,&measure)||!measure.ink)return word_fail(error,"mouse word measurement");const f32 half=word.extent*.5f;const std::array<std::array<f32,3>,4> box={{{measure.left-half,half-measure.top,0},{measure.right-half,half-measure.top,0},{measure.right-half,half-measure.bottom,0},{measure.left-half,half-measure.bottom,0}}};if(!mouse_geometry_.world(camera,word.world,box,target))return word_fail(error,"projected mouse word");}
        if(area==OptionMouseArea::Settings)for(unsigned i=0;i<instances_.size();++i){const auto& item=instances_[i];if(item.widget.kind!=3)continue;const auto& mesh=renderer_.scene().meshes[i];
            //The source widget is FIVE authored quads with different heights,
            //not five fitted subdivisions of one rectangle. Use its current
            //evaluated world geometry once, including the source TV rotation.
            if(mesh.vertices.size()!=20||mesh.indices.size()!=30)return word_fail(error,"mouse five-bar source geometry");
            for(unsigned level=0;level<5;++level){std::array<std::array<f32,3>,4> part;for(unsigned j=0;j<4;++j)std::copy_n(mesh.vertices[level*4+j].position,3,part[j].begin());for(unsigned j=0;j<6;++j)if(mesh.indices[level*6+j]/4!=level)return word_fail(error,"mouse bar triangle binding");if(!mouse_geometry_.world(camera,Mat4{},part,{area,s16(item.widget.id+1),s16(level),0}))return word_fail(error,"projected authored volume choice");}}
        return mouse_geometry_.finish()?true:word_fail(error,"mouse page publication");
    }
    static bool word_fail(std::string* error,const char* why){if(error&&error->empty())*error=std::string("option words: ")+why;return false;}
    bool word_span(unsigned kind,unsigned id,unsigned part,WordSpan* out)const{
        if(!out)return false;
        Mat4 inverse;if(!mat4_inverse_affine(label_world_,&inverse))return false;
        for(unsigned i=0;i<instances_.size();++i){const auto& item=instances_[i];if(item.widget.kind!=kind||item.widget.id!=id||item.part!=part)continue;WordSpan span{1e6f,-1e6f};for(const auto& vertex:renderer_.scene().meshes[i].vertices){f32 local[3];mat4_transform_point(inverse,vertex.position,local);span.left=std::min(span.left,local[0]);span.right=std::max(span.right,local[0]);}if(!std::isfinite(span.left)||!std::isfinite(span.right)||span.left>=span.right||std::abs(span.left)>10000||std::abs(span.right)>10000)return false;*out=span;return true;}return false;
    }
    bool word_colour(const HsdAnimPose& pose,unsigned kind,unsigned id,unsigned part,u32* out)const{
        if(!out||pose.materials.size()!=instances_.size())return false;
        for(unsigned i=0;i<instances_.size();++i){const auto& item=instances_[i];if(item.widget.kind!=kind||item.widget.id!=id||item.part!=part)continue;u32 colour=255;for(unsigned k=0;k<3;++k){const f32 value=pose.materials[i].diffuse[k];if(!std::isfinite(value)||value<0||value>1)return false;colour|=u32(u8(value*255))<<(24-k*8);}*out=colour;return true;}return false;
    }
    bool place_word(std::string_view ascii,unsigned row,WordSpan span,u32 colour,std::vector<OptionWordPlacement>* out,std::string* error)const{
        if(!out||out->size()>=64||row>=10||!std::isfinite(span.left)||!std::isfinite(span.right)||span.right-span.left<=4)return word_fail(error,"slot domain");
        OptionWordPlacement word;word.ascii=ascii;word.row=row;word.colour=colour;if(!text_.font().encode_ascii(ascii,&word.encoded))return word_fail(error,"unmapped character");SourceTextMeasure measure;if(!text_.measure(word.encoded,word.extent,&measure)||!measure.ink)return word_fail(error,"empty/unmeasured word");
        const f32 width=measure.right-measure.left;word.horizontal_scale=std::min(1.f,(span.right-span.left-4)/width);if(!std::isfinite(word.horizontal_scale)||word.horizontal_scale<.5f)return word_fail(error,"word exceeds its slot");
        const f32 half=word.extent*.5f,x=span.left+2-measure.left*word.horizontal_scale+half*word.horizontal_scale;const f32 translation[3]={x,0,0},scale[3]={word.horizontal_scale,1,1};auto plane=label_world_;plane.at(1,3)=profile_.row_y[row];word.world=mat4_multiply(plane,mat4_multiply(mat4_translation(translation),mat4_scale(scale)));
        word.plane_bounds={span.left+2,profile_.row_y[row]+half-measure.bottom,span.left+2+width*word.horizontal_scale,profile_.row_y[row]+half-measure.top};
        for(const auto& other:*out){const auto& a=word.plane_bounds;const auto& b=other.plane_bounds;if(a[0]<b[2]&&b[0]<a[2]&&a[1]<b[3]&&b[1]<a[3])return word_fail(error,"visible word slots overlap");}out->push_back(std::move(word));return true;
    }
    bool build_words(OptionPresentationPage page,unsigned row,const HsdAnimPose& pose,std::vector<OptionWordPlacement>* out,std::string* error)const{
        if(!out)return false;
        out->clear();const auto selected=[&](unsigned selected_row){return row==selected_row?profile_.selected_colour:profile_.normal_colour;};const auto original=[&](std::string_view text,unsigned word_row,WordSpan slot,unsigned kind,unsigned id,unsigned part){u32 colour=0;return word_colour(pose,kind,id,part,&colour)&&place_word(text,word_row,slot,colour,out,error);};
        if(page==OptionPresentationPage::Settings){const std::array<const char*,5> labels={"SOUND","VOLUME","RUMBLE","MORE","RESTORE DEFAULT SETTINGS"};const std::array<unsigned,5> rows={0,1,4,8,9};for(unsigned i=0;i<5;++i)if(!original(labels[i],rows[i],i==4?wide_:primary_,1,i,0))return false;
            if(!original("STEREO",0,sound_[0],5,0,0)||!original("/",0,sound_slash_,1,0,1)||!original("MONO",0,sound_[1],5,0,1))return false;
            for(unsigned i=0;i<3;++i)if(!original(std::array<const char*,3>{"BGM","SFX","VOICE"}[i],i+1,secondary_,4,i,0))return false;
            for(unsigned i=0;i<4;++i)if(!original(std::to_string(i+1)+"P",i+4,player_,4,i+3,0)||!original("ON",i+4,on_,5,i+1,0)||!original("/",i+4,rumble_slash_,4,i+3,1)||!original("OFF",i+4,off_,5,i+1,1))return false;
        }else if(page==OptionPresentationPage::More){if(!place_word("CHEATS",0,primary_,selected(0),out,error)||!place_word("STAFF CREDITS",1,primary_,selected(1),out,error))return false;
        }else if(page==OptionPresentationPage::Cheats){for(unsigned i=0;i<kCheatDescriptions.size();++i){const bool enabled=kCheatDescriptions[i].implemented;const u32 colour=enabled?selected(i):row==i?0xaaaaaaff:0x606060ff;if(!place_word(kCheatDescriptions[i].label,i,primary_,colour,out,error))return false;}for(unsigned i=0;i<2;++i)if(!original("ON",i,on_,5,i+1,0)||!place_word("/",i,rumble_slash_,selected(i),out,error)||!original("OFF",i,off_,5,i+1,1))return false;if(!place_word("PRESS Y FOR DETAILS",9,wide_,profile_.selected_colour,out,error))return false;
        }else return false;
        return true;
    }
    bool screen_word(std::string_view ascii,f32 x,f32 y,f32 width,f32 height,f32 extent,u32 colour,std::vector<ScreenWord>* out,std::string* error)const{
        ScreenWord word;word.extent=extent;word.colour=colour;if(!text_.font().encode_ascii(ascii,&word.encoded))return word_fail(error,"screen character map");SourceTextMeasure measure;if(!text_.measure(word.encoded,extent,&measure)||!measure.ink)return word_fail(error,"screen measurement");word.sx=std::min(1.f,width/(measure.right-measure.left));if(word.sx<.65f||measure.bottom-measure.top>height)return word_fail(error,"screen word exceeds slot");word.x=(x-measure.left*word.sx)/word.sx;word.y=y-measure.top;out->push_back(std::move(word));return true;
    }
    bool draw_screen_words(RenderEngine& render,const std::vector<ScreenWord>& words,std::string* error){for(const auto& word:words)if(!text_.draw_screen_measured(render,word.encoded,word.x,word.y,word.extent,word.sx,1,0x000000ff,word.colour))return word_fail(error,"screen word submission");return true;}
    static bool word_panel(RenderEngine& render,f32 x0,f32 y0,f32 x1,f32 y1){const auto saved=render.state();render.state()={};render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;render.state().depth_test=render.state().depth_write=false;render.state().cull_backface=false;Mat4 identity;bool ok=render.upload_matrix(0,identity.m);if(ok){std::vector<Vertex> vertices;ScreenDraw::textured_quad(vertices,x0,y0,x1,y1,0,0,0,1,1,{24,31,48,250});const auto at=render.append_vertices(vertices.data(),vertices.size());ok=render.draw(Primitive::Triangles,at,u32(vertices.size()));}render.state()=saved;return ok;}
    bool draw_word_details(RenderEngine& render,unsigned row,std::string* error){if(mouse_enabled_)mouse_geometry_.clear();const auto& info=kCheatDescriptions[row];std::vector<ScreenWord> words;if(!screen_word(info.label,116,120,408,30,24,profile_.selected_colour,&words,error))return false;for(unsigned i=0;i<info.lines.size();++i)if(!info.lines[i].empty()&&!screen_word(info.lines[i],116,174+28.f*i,408,24,16,0xffffffffu,&words,error))return false;if(!screen_word("A OR B TO RETURN",116,368,408,24,16,profile_.selected_colour,&words,error)||!word_panel(render,96,100,544,402)||!draw_screen_words(render,words,error))return false;if(mouse_enabled_){mouse_geometry_.begin(OptionMouseArea::Details);if(!mouse_screen_part(words.back(),0,words.back().encoded.size()/2,{OptionMouseArea::Details,-1,-1,0x100})||!mouse_geometry_.finish())return word_fail(error,"details mouse footer");}return true;}
    bool detail_text(RenderEngine& render,std::string_view ascii,f32 x,f32 y,f32 extent,f32 advance,u32 colour,std::string* error){
        std::string encoded;if(!text_.font().encode_ascii(ascii,&encoded))return false;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)if(!text_.draw_screen(render,encoded,x+dx,y+dy,extent,advance,1,1,0x000000ff,0x000000ff))return false;
        const bool ok=text_.draw_screen(render,encoded,x,y,extent,advance,1,1,0x000000ff,colour);if(!ok&&error)*error="option details: source text draw";return ok;
    }
    bool label(RenderEngine& render,const Camera& camera,std::string_view ascii,unsigned row,bool selected,bool disabled,std::string* error){
        std::string encoded;if(!text_.font().encode_ascii(ascii,&encoded))return false;
        auto world=label_world_;world.at(1,3)=profile_.row_y[row];const f32 extent=32,advance=20;const f32 anchor[3]={label_left_+extent*.5f,0,0};world=mat4_multiply(world,mat4_translation(anchor));
        // Native labels use original game-font pixels, source label plane/rows
        // and colours, with a black outline matching the OPTION word artwork.
        for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x)if(x||y){const f32 offset[3]={f32(x)*1.5f,f32(y)*1.5f,0};const auto shadow=mat4_multiply(world,mat4_translation(offset));if(!text_.draw(render,camera,shadow,encoded,extent,advance,{255,255,255,255},false,0x000000ff,0x000000ff))return false;}
        const u32 colour=disabled?(selected?0xaaaaaaffu:0x606060ffu):selected?profile_.selected_colour:profile_.normal_colour;
        const bool ok=text_.draw(render,camera,world,encoded,extent,advance,{255,255,255,255},false,0x000000ff,colour);if(!ok&&error)*error="option pages: source text draw";return ok;
    }
    struct Instance {OptionWidgetTemplate widget;unsigned part=0;int clip=-1;Mat4 authored;f32 local_left=0;};
    bool loaded_=false;u16 cursor_phase_=0;OptionWidgetProfile profile_;
    bool pages_ready_=false,extensions_=false;OptionPresentationPage page_=OptionPresentationPage::Settings;unsigned page_row_=0;SourceTextRenderer text_;Mat4 label_world_;f32 label_left_=0;
    bool word_pages_=false;WordSpan primary_,wide_,secondary_,sound_slash_,player_,on_,off_,rumble_slash_;std::array<WordSpan,2> sound_{};std::vector<OptionWordPlacement> words_;
    std::vector<Instance> instances_;std::vector<HsdLeafMaterialClip> clips_;
    HsdAnimPose pose_;MenuScene renderer_;
    OptionMouseGeometry mouse_geometry_;bool mouse_enabled_=false;
};
} // namespace gf
