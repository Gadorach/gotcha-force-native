#pragma once
#include "engine/assets/scene_descriptor_pool.hpp"
#include "engine/assets/scene_archive_pool.hpp"
#include <array>
namespace gf {
struct SceneDescriptorBootstrapRow {u32 offset=0,bytes=0;u16 stride=0;u8 pool=0,profile=0;u32 initialize_key=0,cleanup_key=0;bool operator==(const SceneDescriptorBootstrapRow&)const=default;};
//Complete fresh97854(1) recipe; immutable offsets/hooks are DATA, never host
//pointers. This is not a reset/reinitialize API for already-populated pools.
class SceneDescriptorBootstrapProfile {
public:
    bool load(ByteView b,std::string* error){
        *this={};auto fail=[&](){if(error)*error="descriptor bootstrap: exact fresh profile1/four-row recipe required";return false;};
        if(!b.data||b.size!=128||b.be32(0)!=0x47464442||b.be32(4)!=1||b.be32(8)!=128||b.be32(12)!=1||b.be32(16)!=0x8036f644||b.be32(20)!=0x80197854||b.be32(24)!=0x8036f6b4||b.be32(28))return fail();
        const std::array<SceneDescriptorBootstrapRow,4> expected{{{0x1de800,0x10000,44,0,1,0x80006f60,0x80006ed8},{0x1ee800,0x400,32,1,1,0x80006e3c,0x80006e8c},{0x1eec00,0x2000,16,2,1,0x80006e3c,0x80006e8c},{0x1f0c00,0x2000,16,3,1,0,0x80006f1c}}};
        for(unsigned i=0;i<4;++i){const unsigned at=32+24*i;SceneDescriptorBootstrapRow r{b.be32(at),b.be32(at+4),b.be16(at+8),b.data[at+10],b.data[at+11],b.be32(at+12),b.be32(at+16)};if(r!=expected[i]||b.be32(at+20))return fail();rows_[i]=r;}
        //Feed existing concrete owners through their unchanged profile loaders.
        std::array<u8,40> model{},archive{};auto fill=[&](auto& bytes,u32 magic,const auto& r,u32 header){store_be32(bytes.data(),magic);store_be32(bytes.data()+4,1);store_be32(bytes.data()+8,40);store_be32(bytes.data()+16,r.offset);store_be32(bytes.data()+20,r.bytes);store_be16(bytes.data()+24,r.stride);bytes[26]=r.pool;bytes[27]=r.profile;store_be32(bytes.data()+28,r.initialize_key);store_be32(bytes.data()+32,r.cleanup_key);store_be32(bytes.data()+36,header);};
        fill(model,0x47464450,rows_[0],0);fill(archive,0x47464150,rows_[1],68);if(!models_.load({model.data(),model.size()},error)||!archives_.load({archive.data(),archive.size()},error))return fail();ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}const SceneDescriptorBootstrapRow* row(unsigned i)const{return ready_&&i<4?&rows_[i]:nullptr;}
    const SceneDescriptorPoolProfile& model_profile()const{return models_;}const SceneArchivePoolProfile& archive_profile()const{return archives_;}
private:std::array<SceneDescriptorBootstrapRow,4> rows_{};SceneDescriptorPoolProfile models_;SceneArchivePoolProfile archives_;bool ready_=false;
};
} // namespace gf
