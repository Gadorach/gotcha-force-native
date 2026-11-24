#pragma once
#include "engine/assets/character_collision_bank.hpp"
namespace gf {
class ColliderInitialShapeProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="collider initial shapes: exact source operands required";return false;};
        if(!b.contains(0,208)||b.size!=208||b.be32(0)!=0x47464353||b.be32(4)!=1||b.be32(8)!=208||b.be32(12))return fail();
        const std::array<u32,8> keys{0x8003a4b8,0x8003a8fc,0x8003b170,0x8003c3d8,0x8008a65c,0x8008aa40,0x8008a2bc,0x80089aec};
        for(unsigned i=0;i<8;++i)if(b.be32(16+4*i)!=keys[i])return fail();
        ColliderGeometryProfile geometry;geometry.loaded=true;for(unsigned i=0;i<8;++i)geometry.words[i]=b.be32(48+4*i);
        if(!collider_geometry_profile_valid(geometry))return fail();
        const std::array<std::array<f32,3>,8> corners{{{1,1,-1},{1,1,1},{-1,1,-1},{-1,1,1},{1,-1,-1},{1,-1,1},{-1,-1,-1},{-1,-1,1}}};
        for(unsigned i=0;i<8;++i)for(unsigned k=0;k<3;++k)if(b.be32(80+12*i+4*k)!=f32_to_bits(corners[i][k]))return fail();
        const std::array<std::array<u8,4>,6> faces{{{0,2,3,1},{1,3,7,5},{2,6,7,3},{4,5,7,6},{6,2,0,4},{5,4,0,1}}};
        for(unsigned i=0;i<6;++i)for(unsigned k=0;k<4;++k)if(b.data[176+4*i+k]!=faces[i][k])return fail();
        if(b.be32(200)!=1||b.be32(204))return fail();
        geometry_=geometry;corners_=corners;faces_=faces;ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const ColliderGeometryProfile& geometry()const{return geometry_;}
    const auto& corners()const{return corners_;}const auto& faces()const{return faces_;}
private:ColliderGeometryProfile geometry_;std::array<std::array<f32,3>,8> corners_{};std::array<std::array<u8,4>,6> faces_{};bool ready_=false;
};
}
