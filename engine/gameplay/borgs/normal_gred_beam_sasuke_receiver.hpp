#pragma once
#include "engine/gameplay/collider_sphere_shape.hpp"

namespace gf {
// A prepared body-row binding, not a scene collider. Exact normal member
// 495, slot 2 plus the normal model's row-0 hit parameters.
class NormalSasukeBeamReceiverProfile {
public:
    bool load(ByteView bytes,std::string* error){
        *this={};
        if(!bytes.data||bytes.size!=7864)return refuse(error);
        u32 hash=2166136261u;
        for(std::size_t i=0;i<bytes.size;++i)hash=(hash^bytes.data[i])*16777619u;
        if(hash!=0xefcf3df4u)return refuse(error);
        CharacterCollisionBank bank;
        if(!bank.load({bytes.data,7840},error))return false;
        const auto ids=bank.body_ids();
        CharacterCollisionDescriptor d;
        if(std::find(ids.begin(),ids.end(),9)==ids.end()||!bank.descriptor(CharacterCollisionTable::Body,9,&d,error)||
           d.shape||d.binding1!=2||d.parameter4||d.flags2!=2||d.start6||d.end8!=-1||d.scalar(0x18)!=22)
            return refuse(error);
        body_=d;std::copy_n(bytes.data+7840,hit_.size(),hit_.begin());
        if(error)error->clear();
        return true;
    }
    bool ready()const{return body_.declared;}
    ByteView hit_row()const{return ready()?ByteView{hit_.data(),hit_.size()}:ByteView{};}
    bool seed(const Mat4& key2,f32 physical_c0,ColliderSphereSeed* out)const{
        return ready()&&physical_c0>0&&collider_sphere_seed(body_,key2,physical_c0,out);
    }
private:
    static bool refuse(std::string* error){
        if(error)*error="Sasuke receiver: exact normal bank, body 9 and hit row 0 required";
        return false;
    }
    CharacterCollisionDescriptor body_;
    std::array<u8,24> hit_{};
};
}
