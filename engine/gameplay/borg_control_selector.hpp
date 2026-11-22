#pragma once
#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/graphics/transform.hpp"
#include "engine/input/input_snapshot.hpp"

namespace gf {
// Exact controllers currently usable in the local desktop control test.
// Discovery/preview alone never qualifies another model for this list.
class BorgControlSelector {
public:
    struct Entry {u16 model;std::string_view label,controls;};
    static constexpr std::array entries{
        Entry{0x0615,"G-RED / RECOVERED MOVEMENT","ARROWS WALK   WASD FLICK DASH   SHIFT JUMP"},
        Entry{0x000a,"SASUKE / PROVISIONAL MVP","WASD OR ARROWS MOVE   L DASH   SHIFT JUMP"},
    };
    static constexpr s32 left=20,top=20,width=400,row_height=32;

    bool open()const{return open_;}
    std::optional<unsigned> select(const PointerState& p){
        if(!p.within_window||!p.left_pressed)return {};
        const bool inside=p.x>=left&&p.x<left+width;
        if(inside&&p.y>=top&&p.y<top+row_height){open_=!open_;return {};}
        if(!open_)return {};
        open_=false;
        if(!inside||p.y<top+row_height||p.y>=top+row_height*s32(entries.size()+1))return {};
        return unsigned((p.y-top)/row_height-1);
    }

    unsigned draw(RenderEngine& render,unsigned selected,s32 viewport_width,s32 height,bool paused,bool refused,
                  std::string_view readout){
        if(selected>=entries.size())return 0;
        const auto saved=render.state();
        render.state()={};render.state().depth_test=false;render.state().depth_write=false;
        render.state().cull_backface=false;render.state().blend=true;render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        vertices_.clear();
        auto fill=[&](s32 x,s32 y,s32 w,s32 h,Tint tint){
            ScreenDraw::textured_quad(vertices_,f32(x),f32(y),f32(x+w),f32(y+h),0,0,0,1,1,tint);
        };
        fill(left,top,width,row_height,{17,25,41,246});
        fill(left,top+row_height-2,width,2,{117,202,246,255});
        if(open_)for(unsigned i=0;i<entries.size();++i)
            fill(left,top+s32(i+1)*row_height,width,row_height,
                 i==selected?Tint{37,69,99,252}:Tint{22,32,51,252});
        const s32 bottom=height-92;
        fill(left,bottom,std::min(540,viewport_width-2*left),72,{17,25,41,236});
        const auto base=render.append_vertices(vertices_.data(),vertices_.size());
        unsigned draws=render.draw(Primitive::Triangles,base,u32(vertices_.size()))?1:0;
        TextRenderer::Style style;style.scale=viewport_width>=580?2:1;
        const s32 text_inset=(row_height-TextRenderer::kGlyphRows*style.scale)/2;
        draws+=text_.draw(render,entries[selected].label,left+10,top+text_inset,style);
        draws+=text_.draw(render,open_?"-":"V",left+width-24,top+text_inset,style);
        if(open_)for(unsigned i=0;i<entries.size();++i)
            draws+=text_.draw(render,entries[i].label,left+10,top+s32(i+1)*row_height+text_inset,style);
        draws+=text_.draw(render,entries[selected].controls,left+10,bottom+8,style);
        draws+=text_.draw(render,"Q RESET   ENTER PAUSE   ESC CLOSE",left+10,bottom+29,style);
        draws+=text_.draw(render,refused?"MOTION REFUSED - Q TO RESET":paused?"PAUSED":readout,left+10,bottom+50,style);
        render.state()=saved;
        return draws;
    }
private:
    bool open_=false;
    TextRenderer text_;
    std::vector<Vertex> vertices_;
};
}
