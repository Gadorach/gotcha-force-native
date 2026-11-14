#pragma once
#include <array>
#include <cmath>
#include <vector>
#include "engine/gameplay/option_mouse.hpp"
#include "engine/graphics/camera.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
struct OptionMouseRegion {std::array<std::array<f32,2>,4> quad{};OptionMouseTarget target;};
//Bounded projected coverage of the actual TV words/bars or modal footer text.
//The owner publishes only after drawing; beginning a new page invalidates the
//previous page. Later regions (specific choices) win over general row labels.
class OptionMouseGeometry {
public:
    void clear(){regions_.clear();area_=OptionMouseArea::None;valid_=false;failed_=false;}
    void begin(OptionMouseArea area){clear();if(area!=OptionMouseArea::None&&unsigned(area)<=unsigned(OptionMouseArea::Credits))area_=area;}
    bool add(const std::array<std::array<f32,2>,4>& quad,const OptionMouseTarget& target){
        if(failed_||area_==OptionMouseArea::None||target.area!=area_||regions_.size()>=64||!option_mouse_valid({target.area,target.row,target.value,target.left_pressed}))return reject();
        f64 sign=0;for(unsigned i=0;i<4;++i){for(auto value:quad[i])if(!std::isfinite(value)||std::abs(value)>1e6f)return reject();const auto c=cross(quad[i],quad[(i+1)%4],quad[(i+2)%4][0],quad[(i+2)%4][1]);if(!c||(sign&&sign*c<=0))return reject();sign=c;}
        regions_.push_back({quad,target});return true;
    }
    bool world(const Camera& camera,const Mat4& world,const std::array<std::array<f32,3>,4>& local,const OptionMouseTarget& target){
        if(camera.viewport_width!=640||camera.viewport_height!=480)return reject();
        for(auto v:world.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return reject();
        std::array<std::array<f32,2>,4> quad;for(unsigned i=0;i<4;++i){for(auto v:local[i])if(!std::isfinite(v)||std::abs(v)>1e7f)return reject();f32 p[3];mat4_transform_point(world,local[i].data(),p);if(!camera.project(p,&quad[i][0],&quad[i][1],nullptr))return reject();}return add(quad,target);
    }
    bool screen(f32 x0,f32 y0,f32 x1,f32 y1,const OptionMouseTarget& target){if(x0>=x1||y0>=y1)return reject();return add({{{x0,y0},{x1,y0},{x1,y1},{x0,y1}}},target);}
    bool finish(bool accepted=true){valid_=accepted&&!failed_&&area_!=OptionMouseArea::None&&!regions_.empty();if(!valid_)regions_.clear();return valid_;}
    bool hit(f32 x,f32 y,OptionMouseArea expected,OptionMouseTarget* out)const{
        if(out)*out={};
        if(!out||!valid_||expected!=area_||!std::isfinite(x)||!std::isfinite(y)||x<0||y<0||x>=640||y>=480)return false;
        for(auto i=regions_.rbegin();i!=regions_.rend();++i){if(inside(i->quad[0],i->quad[1],i->quad[2],x,y)||inside(i->quad[0],i->quad[2],i->quad[3],x,y)){*out=i->target;return true;}}
        return false;
    }
    const std::vector<OptionMouseRegion>& regions()const{return regions_;}
private:
    static f64 cross(const std::array<f32,2>& a,const std::array<f32,2>& b,f64 x,f64 y){return (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);}
    static bool inside(const std::array<f32,2>& a,const std::array<f32,2>& b,const std::array<f32,2>& c,f32 x,f32 y){const auto area=cross(a,b,c[0],c[1]);const auto u=cross(b,c,x,y)/area,v=cross(c,a,x,y)/area;return u>=0&&v>=0&&1-u-v>=0;}
    bool reject(){valid_=false;failed_=true;regions_.clear();return false;}
    std::vector<OptionMouseRegion> regions_;OptionMouseArea area_=OptionMouseArea::None;bool valid_=false,failed_=false;
};
} //namespace gf
