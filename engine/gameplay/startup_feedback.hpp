#pragma once
// AEE0/AFFC/B00C/B024,8C3F0,AC10/B034. Checked shared-backend framebuffer copy,
// not a camera fit/flash substitute or GX copy-filter/hardware parity claim.
#include "engine/assets/startup_feedback.hpp"
#include "engine/gameplay/startup_director.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace gf {
struct StartupFeedbackState {u32 flags=0,frame=0,rendered_frame=0,generation=0,captures=0,draws=0;f32 alpha=0;};
struct StartupFeedbackRender {bool captured=false,drawn=false;u8 alpha=0;};
class StartupFeedback {
public:
    bool load(const StartupFeedbackProfile& p,u32 texture,std::string* error){
        ready_=false;state_={};profile_={};texture_=0;
        if(!startup_feedback_profile_valid(p)||!texture)return fail(error,"source profile/native texture required");
        profile_=p;texture_=texture;ready_=true;if(error)error->clear();return true;
    }
    bool step(const StartupDirectorState& director,std::span<const StartupEffect> events,std::string* error){
        if(!ready_||director.updates!=state_.frame+1||director.updates>8192)return refuse(error,"consecutive source update required");
        auto s=state_;
        for(const auto& e:events){
            if(e.kind==StartupEffectKind::Command&&e.opcode==15){
                if(e.operands[0]==0)s.flags=0;
                else if(e.operands[0]==1){if(director.config_words!=profile_.source_words)return refuse(error,"source configuration binding");s.flags=profile_.mode|0x80;s.alpha=director.config_value;++s.generation;}
                else return refuse(error,"unsupported source enable operand");
            }else if(e.kind==StartupEffectKind::ConfigSample&&s.flags)s.alpha=e.value;
        }
        if(s.flags&&(!std::isfinite(s.alpha)||s.alpha<0||s.alpha>1))return refuse(error,"supported source alpha domain");
        s.frame=director.updates;state_=s;if(error)error->clear();return true;
    }
    bool render(RenderEngine& render,u32 frame,u32 canvas_width,u32 canvas_height,StartupFeedbackRender* out,std::string* error){
        if(out)*out={};
        if(!out||!ready_||frame!=state_.frame||frame==state_.rendered_frame||!canvas_width||!canvas_height||canvas_width>16384||canvas_height>16384)return refuse(error,"matching single render/canvas required");
        auto s=state_;StartupFeedbackRender effects;
        auto capture=[&](){FramebufferCopy c;c.texture=texture_;c.width=profile_.copy[2];c.height=profile_.copy[3];c.x=profile_.copy[0];c.y=profile_.copy[1];c.source_width=profile_.copy[2];c.source_height=profile_.copy[3];c.reference_width=u32(profile_.copy[0])+profile_.copy[2];c.reference_height=u32(profile_.copy[1])+profile_.copy[3];
            if(!render.copy_framebuffer(c))return false;
            s.flags&=~0x80u;++s.captures;effects.captured=true;return true;};
        auto draw=[&](){if(s.flags&0x80)return true;
            const f32 product=f32(profile_.alpha_factor*s.alpha);if(!std::isfinite(product)||product<0||product>255)return false;const auto alpha=u8(u32(product));
            const f32 sx=f32(canvas_width)/f32(profile_.copy[2]),sy=f32(canvas_height)/f32(profile_.copy[3]);std::vector<Vertex> vertices;
            ScreenDraw::textured_quad(vertices,f32(profile_.destination[0]*sx),f32(profile_.destination[1]*sy),f32((u32(profile_.destination[0])+profile_.destination[2])*sx),f32((u32(profile_.destination[1])+profile_.destination[3])*sy),0,0,0,1,1,{255,255,255,alpha});
            const auto saved=render.state();RenderState state;state.depth_test=true;state.depth_compare=7;state.depth_write=false;state.blend=true;state.blend_src=5;state.blend_dst=6;state.cull_backface=true;state.texture=texture_;state.position_matrix=kIdentityPositionMatrix;state.texture_colour_mode=4;state.texture_alpha_mode=3;render.state()=state;
            const auto base=render.append_vertices(vertices.data(),vertices.size());const bool ok=render.draw(Primitive::Triangles,base,u32(vertices.size()));render.state()=saved;
            if(ok){++s.draws;effects.drawn=true;effects.alpha=alpha;}return ok;};
        if(s.flags){const u32 mode=s.flags&~0x80u;if(mode==1){if(!capture()||!draw())return refuse(error,"capture-before-draw failed");}
            else if(mode==2){if(!draw()||!capture())return refuse(error,"draw-before-capture failed");}
            else return refuse(error,"unsupported source mode");}
        s.rendered_frame=frame;state_=s;*out=effects;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const StartupFeedbackState& state()const{return state_;}
private:
    static bool fail(std::string* error,const char* why){if(error)*error=std::string("startup feedback: ")+why;return false;}
    bool refuse(std::string* error,const char* why){ready_=false;return fail(error,why);}
    StartupFeedbackProfile profile_;StartupFeedbackState state_;u32 texture_=0;bool ready_=false;
};
} // namespace gf
