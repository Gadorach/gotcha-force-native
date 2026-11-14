#pragma once
#include "engine/graphics/convex_text_layout.hpp"
#include "engine/core/backends.hpp"
namespace gf {
//Read-only layout/hit-test coverage of submitted UI triangles and their actual
//single-stage alpha texture. No draw calls, camera fitting, or second renderer.
//Unknown/non-row-convex masks refuse rather than filling holes with a rectangle.
class AlphaTextRegion {
public:
    struct Row {s32 left=0,right=0;};
    bool load(std::span<const Vertex> triangles,const TextureDesc& texture,ByteView pixels){
        *this={};if(triangles.empty()||triangles.size()%3||triangles.size()>768||!texture.pixels||!texture.width||!texture.height||texture.width>2048||texture.height>2048||texture.wrap_s||texture.wrap_t||pixels.data!=texture.pixels||pixels.size!=std::size_t(texture.width)*texture.height*4)return false;
        f32 l=triangles[0].x,r=l,t=triangles[0].y,b=t;
        for(const auto& v:triangles){for(f32 x:{v.x,v.y,v.u,v.v,v.inv_w})if(!std::isfinite(x)||std::abs(x)>1e7f)return false;if(v.inv_w<=0)return false;l=std::min(l,v.x);r=std::max(r,v.x);t=std::min(t,v.y);b=std::max(b,v.y);}
        if(l<0||t<0||r>8192||b>8192)return false;
        const s32 left=s32(std::floor(l)),right=s32(std::ceil(r)),top=s32(std::floor(t)),bottom=s32(std::ceil(b));
        if(right<=left||bottom<=top||u64(right-left)*u64(bottom-top)>262144)return false;
        std::vector<Row> rows;std::vector<TextRegionPoint> outline;bool started=false,ended=false;s32 first=0;
        for(s32 y=top;y<bottom;++y){Row row;bool ink=false,gap=false;
            for(s32 x=left;x<right;++x){
                bool covered=true;
                for(const auto& offset:std::array<TextRegionPoint,5>{{{.001f,.001f},{.999f,.001f},{.999f,.999f},{.001f,.999f},{.5f,.5f}}})if(!opaque(triangles,texture,x+offset.x,y+offset.y)){covered=false;break;}
                if(covered){if(gap)return false;if(!ink){row.left=x;ink=true;}row.right=x+1;}
                else if(ink)gap=true;
            }
            if(ink){if(ended)return false;if(!started){first=y;started=true;}rows.push_back(row);outline.push_back({row.left+.5f,y+.5f});outline.push_back({row.right-.5f,y+.5f});}
            else if(started)ended=true;
        }
        if(!started||rows.size()<3||!hull_.load(outline))return false;
        rows_=std::move(rows);first_row_=first;return true;
    }
    bool contains(const TextInkRect& rect,f32 inset)const{
        if(rows_.empty()||!hull_.contains(rect,0)||!std::isfinite(inset)||inset<0||inset>10000)return false;
        const s32 top=s32(std::floor(rect.top-inset)),bottom=s32(std::ceil(rect.bottom+inset));
        if(top<first_row_||bottom>first_row_+s32(rows_.size()))return false;
        for(s32 y=top;y<bottom;++y){
            const f64 distance=y+1<rect.top?f64(rect.top)-(y+1):y>rect.bottom?y-f64(rect.bottom):0;
            if(distance>inset)continue;
            const f64 extra=std::sqrt(std::max(0.,f64(inset)*inset-distance*distance));const auto& row=rows_[std::size_t(y-first_row_)];
            if(f64(rect.left)-extra<row.left||f64(rect.right)+extra>row.right)return false;
        }
        return true;
    }
    bool center(std::span<const SourceTextMeasure> lines,f32 step,f32 sx,f32 sy,f32 inset,std::vector<CenteredTextLine>* out)const{
        if(out)out->clear();
        if(!out)return false;
        std::vector<CenteredTextLine> next;
        if(!hull_.center(lines,step,sx,sy,inset,&next))return false;
        for(const auto& line:next)if(!contains(line.ink,inset))return false;
        *out=std::move(next);return true;
    }
    const ConvexTextRegion& hull()const{return hull_;}const std::vector<Row>& rows()const{return rows_;}s32 first_row()const{return first_row_;}
private:
    static f64 edge(const Vertex& a,const Vertex& b,f64 x,f64 y){return (f64(b.x)-a.x)*(y-a.y)-(f64(b.y)-a.y)*(x-a.x);}
    static bool texture_opaque(const TextureDesc& t,f64 u,f64 v){
        u=std::clamp(u,0.,1.);v=std::clamp(v,0.,1.);
        auto alpha=[&](s32 x,s32 y){x=std::clamp(x,0,s32(t.width)-1);y=std::clamp(y,0,s32(t.height)-1);return t.pixels[(std::size_t(y)*t.width+unsigned(x))*4+3];};
        if(!t.linear)return alpha(s32(std::floor(u*t.width)),s32(std::floor(v*t.height)))==255;
        const s32 x=s32(std::floor(u*t.width-.5)),y=s32(std::floor(v*t.height-.5));
        return alpha(x,y)==255&&alpha(x+1,y)==255&&alpha(x,y+1)==255&&alpha(x+1,y+1)==255;
    }
    static bool opaque(std::span<const Vertex> triangles,const TextureDesc& texture,f32 x,f32 y){
        for(unsigned i=0;i<triangles.size();i+=3){const auto& a=triangles[i];const auto& b=triangles[i+1];const auto& c=triangles[i+2];const f64 area=edge(a,b,c.x,c.y);if(!area)continue;
            const f64 wa=edge(b,c,x,y)/area,wb=edge(c,a,x,y)/area,wc=1-wa-wb;if(wa<0||wb<0||wc<0)continue;
            const f64 w=wa*a.inv_w+wb*b.inv_w+wc*c.inv_w;if(w<=0)return false;
            const f64 u=(wa*a.u*a.inv_w+wb*b.u*b.inv_w+wc*c.u*c.inv_w)/w,v=(wa*a.v*a.inv_w+wb*b.v*b.inv_w+wc*c.v*c.inv_w)/w;
            if(texture_opaque(texture,u,v))return true;
        }
        return false;
    }
    ConvexTextRegion hull_;std::vector<Row> rows_;s32 first_row_=0;
};
} //namespace gf
