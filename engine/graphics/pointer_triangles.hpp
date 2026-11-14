#pragma once
#include <vector>
#include <cmath>
#include "engine/core/backends.hpp"
namespace gf {
//Read-only optional capture of the SAME clipped/submitted UI triangles. This is
//geometric coverage, not per-texel alpha or whole-scene occlusion authority.
class PointerTriangles {
public:
    bool append(const std::vector<Vertex>& vertices,bool back=false,bool front=false){
        if(failed_||vertices.size()%3||vertices.size()>16384-triangles_.size())return reject();
        for(std::size_t i=0;i<vertices.size();i+=3){for(unsigned k=0;k<3;++k)for(auto v:{vertices[i+k].x,vertices[i+k].y,vertices[i+k].z})if(!std::isfinite(v)||std::abs(v)>1e7f)return reject();
            const auto area=cross(vertices[i],vertices[i+1],vertices[i+2].x,vertices[i+2].y);if(!area||(back&&area<0)||(front&&area>0))continue;
            triangles_.insert(triangles_.end(),vertices.begin()+i,vertices.begin()+i+3);
        }return true;
    }
    bool hit(f32 x,f32 y)const{
        if(failed_||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=640||y>=480)return false;
        for(std::size_t i=0;i<triangles_.size();i+=3){const auto& a=triangles_[i];const auto& b=triangles_[i+1];const auto& c=triangles_[i+2];const auto area=cross(a,b,c.x,c.y),u=cross(b,c,x,y)/area,v=cross(c,a,x,y)/area;if(u>=0&&v>=0&&1-u-v>=0)return true;}return false;
    }
    bool valid()const{return !failed_;}bool empty()const{return triangles_.empty();}
    const std::vector<Vertex>& vertices()const{return triangles_;}
private:
    static f64 cross(const Vertex& a,const Vertex& b,f64 x,f64 y){return (f64(b.x)-a.x)*(y-a.y)-(f64(b.y)-a.y)*(x-a.x);}
    bool reject(){failed_=true;triangles_.clear();return false;}
    std::vector<Vertex> triangles_;bool failed_=false;
};
}
