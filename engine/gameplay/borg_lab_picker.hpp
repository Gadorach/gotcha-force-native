#pragma once
#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "engine/graphics/scene_draw.hpp"
#include "engine/graphics/text.hpp"
#include "engine/input/input_snapshot.hpp"

namespace gf {
// Desktop lab overlay. Rows are supplied from the same validated installed
// catalogue; this widget holds no assets, actors, or save authority.
class BorgLabPicker {
public:
    struct Entry {std::string id,name,variant;};
    static constexpr s32 left=20,top=20,row_height=24,page_size=8;
    explicit BorgLabPicker(std::vector<Entry> entries,s32 rows=page_size):
        entries_(std::move(entries)),rows_(std::size_t(std::clamp(rows,1,page_size))){}
    const auto& entries()const{return entries_;}
    std::size_t rows_per_page()const{return rows_;}
    static s32 fitting_rows(s32 height){return std::clamp((height-top)/row_height-2,1,page_size);}
    void fit_height(s32 height){
        const auto rows=std::size_t(fitting_rows(height));
        if(rows!=rows_){page_=(open_?page_*rows_:active_)/rows;rows_=rows;}
    }
    bool open()const{return open_;}
    void activate(std::size_t index){if(index<entries_.size())active_=index;}
    static s32 width(s32 viewport){return std::min(viewport-2*left,540);}
    std::optional<std::size_t> step(const PointerState& pad,s32 viewport){
        if(!pad.within_window||entries_.empty()||width(viewport)<=0)return {};
        const bool inside=pad.x>=left&&pad.x<left+width(viewport);
        if(pad.left_pressed&&inside&&pad.y>=top&&pad.y<top+row_height){
            open_=!open_;
            if(open_)page_=active_/rows_;
            return {};
        }
        if(!open_)return {};
        const auto end=std::min(entries_.size(),(page_+1)*rows_);
        const auto footer=top+s32(end-page_*rows_+1)*row_height;
        if(inside&&pad.y>=top&&pad.y<footer+row_height&&pad.wheel){
            if(pad.wheel>0&&page_)--page_;
            if(pad.wheel<0&&end<entries_.size())++page_;
            return {};
        }
        if(!pad.left_pressed)return {};
        if(inside&&pad.y>=footer&&pad.y<footer+row_height){
            if(pad.x<left+width(viewport)/2){if(page_)--page_;}
            else if(end<entries_.size())++page_;
            return {};
        }
        open_=false;
        if(inside&&pad.y>=top+row_height&&pad.y<footer)
            return page_*rows_+std::size_t((pad.y-top)/row_height-1);
        return {};
    }
    unsigned draw(RenderEngine& render,s32 viewport,std::string_view notice){
        const auto w=width(viewport);
        if(entries_.empty()||w<=0)return 0;
        auto saved=render.state();render.state()={};render.state().blend=true;
        render.state().depth_test=render.state().depth_write=render.state().cull_backface=false;
        render.state().blend_src=5;render.state().blend_dst=6;
        const Mat4 identity;render.upload_matrix(0,identity.m);
        std::vector<Vertex> geometry;
        auto panel=[&](s32 row,Tint tint){
            const f32 y=f32(top+row*row_height);
            ScreenDraw::textured_quad(geometry,left,y,left+w,y+row_height,0,0,0,1,1,tint);
        };
        panel(0,{17,25,41,245});
        const auto first=page_*rows_,end=std::min(entries_.size(),first+rows_);
        const auto footer=s32(end-first+1);
        if(open_){
            for(auto i=first;i<end;++i)panel(s32(i-first+1),i==active_?Tint{37,69,99,252}:Tint{22,32,51,252});
            panel(footer,{17,25,41,250});
        }else if(!notice.empty())panel(1,{51,39,20,250});
        auto offset=render.append_vertices(geometry.data(),geometry.size());
        unsigned draws=unsigned(render.draw(Primitive::Triangles,offset,u32(geometry.size())));
        TextRenderer::Style style;style.scale=1;
        auto line=[&](s32 row,std::string text,s32 x,s32 limit){
            // The scoreboard font is ASCII only; keep exact IDs in the
            // label even if a display name cannot be rendered by this font.
            for(auto& c:text){
                if(c>='a'&&c<='z')c=char(c-'a'+'A');
                if(!TextRenderer::covers(c))c='?';
            }
            while(!text.empty()&&text_.measure_width(text,style)>limit)text.pop_back();
            draws+=text_.draw(render,text,x,top+row*row_height+8,style);
        };
        const auto label=[](const Entry& e){return e.id+" / "+e.variant+"  "+e.name;};
        line(0,label(entries_[active_]),left+8,w-36);
        line(0,open_?"-":"V",left+w-18,16);
        if(open_){
            for(auto i=first;i<end;++i)line(s32(i-first+1),label(entries_[i]),left+8,w-16);
            line(footer,"PREV  "+std::to_string(page_+1)+"/"+std::to_string((entries_.size()+rows_-1)/rows_),left+8,w/2-16);
            line(footer,"NEXT",left+w/2+8,w/2-16);
        }else if(!notice.empty())line(1,std::string(notice),left+8,w-16);
        render.state()=saved;return draws;
    }
private:
    std::vector<Entry> entries_;
    std::size_t rows_,active_=0,page_=0;
    bool open_=false;
    TextRenderer text_;
};
}
