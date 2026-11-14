#pragma once
#include "engine/assets/battle_views.hpp"
#include "engine/graphics/camera_transform.hpp"
#include <memory>
#include <optional>
#include <exception>
namespace gf {
struct BattleViewRecord {
    u8 active00=0,index10=0,group11=0,kind83=0,index2e5=0;
    std::array<f32,3> eye2e8{},up9c{},target300{};f32 distance348=0;
    Mat4 transform114,view394; //written by actual identity/view operations below
};
//0BB70 followed by2844C's viewer aliases/common-root20B61C. No projection,
//selected battle-camera, actor tracking, draw callback or live view-frame grant.
class BattleViews {
public:
    BattleViews()=default;BattleViews(const BattleViews&)=delete;BattleViews& operator=(const BattleViews&)=delete;BattleViews(BattleViews&&)=default;BattleViews& operator=(BattleViews&&)=default;
    bool initialize(const BattleViewsProfile& profile,std::string* error){
        if(attempted_||!profile.ready()){if(error)*error="battle views: fresh source profile required";return false;}attempted_=true;
        try{auto next=std::make_unique<Storage>();const auto* eye=profile.vector(0);const auto* up=profile.vector(1);const auto* target=profile.vector(2);
            for(unsigned i=0;i<13;++i){auto& v=next->views[i];v.index10=u8(i);v.group11=u8(i/6);v.index2e5=u8(i%6);v.kind83=32;for(unsigned k=0;k<3;++k){v.eye2e8[k]=eye[k];v.up9c[k]=up[k];v.target300[k]=target[k];}f32 sum=0;for(unsigned k=0;k<3;++k){const f32 d=f32(eye[k]-target[k]);sum=f32(sum+f32(d*d));}v.distance348=sum>0?f32(std::sqrt(sum)):sum;
                if(!camera_orientation_view_matrix(eye,target,up,&v.view394)){if(error)*error="battle views: initial axes invalid";return false;}v.transform114=mat4_identity();}
            next->viewer_indices.fill(-1);next->common_root_links.fill(true); //five typed SAME-root relationships, not pointer words
            next->reset_dc=0;next->reset_da=0;next->reset_d8=0; //10B50 byteDC,byteDA,halfD8 stores
            if(!camera_orientation_view_matrix(profile.vector(3),profile.vector(5),profile.vector(4),&next->common_root)){if(error)*error="battle views: common root axes invalid";return false;}
            storage_=std::move(next);if(error)error->clear();return true;
        }catch(const std::exception&){if(error)*error="battle views: uncertain native construction; no retry";return false;}
    }
    bool ready()const{return bool(storage_);}const BattleViewRecord* view(unsigned i)const{return ready()&&i<13?&storage_->views[i]:nullptr;}
    const Mat4* common_root()const{return ready()?&storage_->common_root:nullptr;}
    std::optional<s8> viewer_index(unsigned i)const{return ready()&&i<5?std::optional<s8>{storage_->viewer_indices[i]}:std::nullopt;}
    const Mat4* viewer_root(unsigned i)const{return ready()&&i<5&&storage_->common_root_links[i]?&storage_->common_root:nullptr;}
    std::optional<std::array<u16,3>> reset_fields_d8_da_dc()const{return ready()?std::optional<std::array<u16,3>>{{u16(storage_->reset_d8),storage_->reset_da,storage_->reset_dc}}:std::nullopt;}
    bool projection_and_active_camera_pending()const{return true;}
    void retire(){storage_.reset();attempted_=true;}
private:struct Storage {std::array<BattleViewRecord,13> views;std::array<s8,5> viewer_indices{};std::array<bool,5> common_root_links{};Mat4 common_root;s16 reset_d8=0;u8 reset_da=0,reset_dc=0;};std::unique_ptr<Storage> storage_;bool attempted_=false;
};
} // namespace gf
