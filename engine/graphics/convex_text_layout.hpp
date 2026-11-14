#pragma once
#include <span>
#include "engine/graphics/source_text_metrics.hpp"
namespace gf {
struct TextRegionPoint {f32 x=0,y=0;bool operator==(const TextRegionPoint&)const=default;};
struct TextInkRect {f32 left=0,top=0,right=0,bottom=0;};
struct CenteredTextLine {unsigned line=0;f32 x=0,y=0;TextInkRect ink;};
//A convex text region built from actual projected mesh vertices. It handles
//rounded/chamfered ends without inventing a capsule radius or fitting a picture.
class ConvexTextRegion {
public:
    bool load(std::span<const TextRegionPoint> points){
        hull_.clear();if(points.size()<3||points.size()>1024)return false;
        std::vector<TextRegionPoint> p(points.begin(),points.end());
        for(const auto& v:p)if(!std::isfinite(v.x)||!std::isfinite(v.y)||std::abs(v.x)>1e7f||std::abs(v.y)>1e7f)return false;
        std::sort(p.begin(),p.end(),[](const auto& a,const auto& b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});p.erase(std::unique(p.begin(),p.end()),p.end());if(p.size()<3)return false;
        std::vector<TextRegionPoint> h;
        for(const auto& v:p){while(h.size()>=2&&cross(h[h.size()-2],h.back(),v)<=0)h.pop_back();h.push_back(v);}
        const auto lower=h.size();
        for(std::size_t i=p.size()-1;i>0;--i){const auto& v=p[i-1];while(h.size()>lower&&cross(h[h.size()-2],h.back(),v)<=0)h.pop_back();h.push_back(v);}
        h.pop_back();if(h.size()<3)return false;
        f64 area=0;for(unsigned i=0;i<h.size();++i)area+=f64(h[i].x)*h[(i+1)%h.size()].y-f64(h[i].y)*h[(i+1)%h.size()].x;
        if(!std::isfinite(area)||area<=1e-6)return false;
        bounds_={h[0].x,h[0].y,h[0].x,h[0].y};for(const auto& v:h){bounds_.left=std::min(bounds_.left,v.x);bounds_.top=std::min(bounds_.top,v.y);bounds_.right=std::max(bounds_.right,v.x);bounds_.bottom=std::max(bounds_.bottom,v.y);}
        hull_=std::move(h);return true;
    }
    bool contains(const TextInkRect& rect,f32 inset)const{
        if(hull_.empty()||!std::isfinite(inset)||inset<0||inset>10000)return false;
        for(f32 v:{rect.left,rect.top,rect.right,rect.bottom})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
        if(rect.left>rect.right||rect.top>rect.bottom)return false;
        for(unsigned i=0;i<hull_.size();++i){const auto& a=hull_[i];const auto& b=hull_[(i+1)%hull_.size()];
            const f64 dx=f64(b.x)-a.x,dy=f64(b.y)-a.y,margin=f64(inset)*std::sqrt(dx*dx+dy*dy);
            for(const auto& p:std::array<TextRegionPoint,4>{{{rect.left,rect.top},{rect.right,rect.top},{rect.right,rect.bottom},{rect.left,rect.bottom}}})if(cross(a,b,p)+1e-5<margin)return false;
        }
        return true;
    }
    //Skip empty/space-only lines, center the complete ink group vertically, and
    //center each nonempty line horizontally. Every ink rectangle must fit every
    //inset polygon half-plane; failure publishes no partial placement list.
    bool center(std::span<const SourceTextMeasure> lines,f32 step,f32 sx,f32 sy,f32 inset,std::vector<CenteredTextLine>* out)const{
        if(out)out->clear();
        if(!out||hull_.empty()||lines.size()>32||!std::isfinite(step)||step<=0||step>10000||!std::isfinite(sx)||!std::isfinite(sy)||sx<=0||sy<=0||sx>1000||sy>1000||!std::isfinite(inset)||inset<0||inset>10000)return false;
        struct Item {unsigned index;SourceTextMeasure measure;f32 baseline;};std::vector<Item> items;f32 top=0,bottom=0;
        for(unsigned i=0;i<lines.size();++i){const auto& m=lines[i];if(!m.ink)continue;
            for(f32 v:{m.left,m.top,m.right,m.bottom})if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
            if(m.left>=m.right||m.top>=m.bottom)return false;
            const f32 baseline=f32(items.size())*step*sy,t=baseline+m.top*sy,b=baseline+m.bottom*sy;
            if(items.empty()){top=t;bottom=b;}else{top=std::min(top,t);bottom=std::max(bottom,b);}items.push_back({i,m,baseline});
        }
        if(items.empty())return true;
        const f32 cx=(bounds_.left+bounds_.right)*.5f,cy=(bounds_.top+bounds_.bottom)*.5f,y_origin=cy-(top+bottom)*.5f;
        std::vector<CenteredTextLine> next;
        for(const auto& item:items){const auto& m=item.measure;const f32 x=cx-(m.left+m.right)*sx*.5f,y=y_origin+item.baseline;
            const TextInkRect rect{x+m.left*sx,y+m.top*sy,x+m.right*sx,y+m.bottom*sy};if(!contains(rect,inset))return false;next.push_back({item.index,x,y,rect});}
        *out=std::move(next);return true;
    }
    const TextInkRect& bounds()const{return bounds_;}const std::vector<TextRegionPoint>& hull()const{return hull_;}
private:
    static f64 cross(const TextRegionPoint& a,const TextRegionPoint& b,const TextRegionPoint& p){return (f64(b.x)-a.x)*(f64(p.y)-a.y)-(f64(b.y)-a.y)*(f64(p.x)-a.x);}
    std::vector<TextRegionPoint> hull_;TextInkRect bounds_{};
};
} //namespace gf
