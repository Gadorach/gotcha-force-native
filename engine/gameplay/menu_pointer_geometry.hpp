#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>
#include "engine/core/types.hpp"
#include "engine/graphics/render_engine.hpp"
namespace gf {
//Opt-in CPU footprint of actually submitted menu triangles. No fitted circles,
//new camera, texture/GPU readback, gameplay mutation or source mouse parity claim.
class MenuPointerGeometry {
    struct Point {f32 x=0,y=0,z=0;};
    struct Triangle {std::array<Point,3> p;u8 item=7;bool foreground=false;};
public:
    void clear(){triangles_.clear();valid_=building_=failed_=false;}
    void begin(unsigned width,unsigned height){clear();width_=width;height_=height;building_=width&&height&&width<=8192&&height<=8192;}
    bool append(std::span<const Vertex> vertices,unsigned item,bool cull_back,bool cull_front,bool foreground){
        if(!building_||failed_||item>=7||vertices.size()%3||vertices.size()/3>16384-triangles_.size())return invalidate();
        for(unsigned i=0;i<vertices.size();i+=3){Triangle t;t.item=u8(item);t.foreground=foreground;
            for(unsigned k=0;k<3;++k){const auto& v=vertices[i+k];for(auto value:{v.x,v.y,v.z})if(!std::isfinite(value)||std::abs(value)>1e7f)return invalidate();t.p[k]={v.x,v.y,v.z};}
            const auto area=cross(t.p[0],t.p[1],t.p[2].x,t.p[2].y);if(!area||(cull_back&&area<0)||(cull_front&&area>0))continue;
            triangles_.push_back(t);
        }return true;
    }
    void finish(bool accepted){valid_=building_&&!failed_&&accepted&&!triangles_.empty();building_=false;if(!valid_)triangles_.clear();}
    bool hit(f32 x,f32 y,unsigned* item)const{
        if(item)*item=7;
        if(!item||!valid_||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=width_||y>=height_)return false;
        bool found=false,foreground=false;f64 depth=0;
        for(const auto& t:triangles_){const auto area=cross(t.p[0],t.p[1],t.p[2].x,t.p[2].y);const auto a=cross(t.p[1],t.p[2],x,y)/area,b=cross(t.p[2],t.p[0],x,y)/area,c=1-a-b;if(a<0||b<0||c<0)continue;
            const f64 z=a*t.p[0].z+b*t.p[1].z+c*t.p[2].z;
            if(!found||(t.foreground&&!foreground)||(t.foreground==foreground&&z>=depth)){found=true;foreground=t.foreground;depth=z;*item=t.item;}
        }return found;
    }
    bool ready()const{return valid_;}
    //Native mouse snap proximity to the SAME submitted item footprint. The
    //optional item filter keeps a latched snap stable until its release zone is
    //left. No fitted centre/radius or per-texel occlusion authority is invented.
    bool near(f32 x,f32 y,f32 distance,unsigned* item,int only=-1)const{
        if(item)*item=7;
        if(!item||!valid_||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(distance)||distance<0||distance>128||x<0||y<0||x>=width_||y>=height_||only< -1||only>=7)return false;
        unsigned inside=7;if(hit(x,y,&inside)&&(only<0||inside==unsigned(only))){*item=inside;return true;}
        f64 best=f64(distance)*distance;bool found=false,foreground=false;f64 depth=0;
        for(const auto& t:triangles_){if(only>=0&&t.item!=unsigned(only))continue;const auto area=cross(t.p[0],t.p[1],t.p[2].x,t.p[2].y);const auto a=cross(t.p[1],t.p[2],x,y)/area,b=cross(t.p[2],t.p[0],x,y)/area;
            if(a>=0&&b>=0&&1-a-b>=0){*item=t.item;return true;}
            for(unsigned i=0;i<3;++i){const auto& p=t.p[i];const auto& q=t.p[(i+1)%3];const f64 dx=f64(q.x)-p.x,dy=f64(q.y)-p.y,length=dx*dx+dy*dy;if(length==0)continue;const f64 along=std::clamp(((x-p.x)*dx+(y-p.y)*dy)/length,0.0,1.0),px=p.x+along*dx,py=p.y+along*dy,d=(x-px)*(x-px)+(y-py)*(y-py),z=p.z+along*(q.z-p.z);
                if(d>best)continue;
                if(!found||d<best||(d==best&&((t.foreground&&!foreground)||(t.foreground==foreground&&z>=depth)))){best=d;depth=z;foreground=t.foreground;*item=t.item;found=true;}}
        }return found;
    }
private:
    static f64 cross(const Point& a,const Point& b,f64 x,f64 y){return (f64(b.x)-a.x)*(y-a.y)-(f64(b.y)-a.y)*(x-a.x);}
    bool invalidate(){triangles_.clear();valid_=false;failed_=true;return false;}
    std::vector<Triangle> triangles_;unsigned width_=0,height_=0;bool valid_=false,building_=false,failed_=false;
};
} //namespace gf
