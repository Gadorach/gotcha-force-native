#pragma once
#include <optional>
#include "engine/gameplay/force_inventory.hpp"
#include "engine/graphics/pointer_triangles.hpp"
namespace gf {
struct EditorListMouseScope {};
struct EditorListMouseContext {
    std::weak_ptr<const EditorListMouseScope> session;std::weak_ptr<const ForceInventoryScope> owner;
    u64 serial=0,revision=0;bool active=false,modal=false,waiting=false;
    unsigned first=0,count=0,visible=0;
    bool same(const EditorListMouseContext& other)const{return active&&other.active&&!session.expired()&&!owner.expired()&&session.lock()==other.session.lock()&&owner.lock()==other.owner.lock()&&serial==other.serial&&revision==other.revision&&modal==other.modal&&waiting==other.waiting&&first==other.first&&count==other.count&&visible==other.visible;}
};
struct EditorListMouseTarget {s16 row=-1;u32 pressed=0,repeated=0;};
inline bool editor_list_mouse_target_valid(const EditorListMouseTarget& t){return t.row>=-1&&t.row<2000&&!(t.pressed&~0x1ff0u)&&!(t.repeated&~0xfu)&&(!t.repeated||t.row==-1);}
struct EditorListMouseCommand {EditorListMouseContext context;EditorListMouseTarget target;};
class EditorListMouseGeometry {
public:
    struct Region {EditorListMouseTarget target;PointerTriangles triangles;};
    void begin(){regions_.clear();valid_=failed_=false;}
    bool add(PointerTriangles triangles,EditorListMouseTarget target){
        if(failed_||!editor_list_mouse_target_valid(target)||!triangles.valid()||regions_.size()>=128){failed_=true;valid_=false;regions_.clear();return false;}
        if(!triangles.empty())regions_.push_back({target,std::move(triangles)});
        return true;
    }
    bool finish(bool accepted=true){valid_=accepted&&!failed_;if(!valid_)regions_.clear();return valid_;}
    bool hit(f32 x,f32 y,EditorListMouseTarget* out)const{if(out)*out={};if(!out||!valid_)return false;for(auto r=regions_.rbegin();r!=regions_.rend();++r)if(r->triangles.hit(x,y)){*out=r->target;return true;}return false;}
    const std::vector<Region>& regions()const{return regions_;}
private:
    std::vector<Region> regions_;bool valid_=false,failed_=false;
};
}
