#pragma once
// Entirely owned two-bone, envelope+rigid, normal-reflection fixture.
#include <vector>
#include "engine/core/types.hpp"
namespace character_model_fixture {
inline void word(std::vector<gf::u8>& b,std::size_t at,gf::u32 v){gf::store_be32(b.data()+at,v);}
inline void scalar(std::vector<gf::u8>& b,std::size_t at,float v){gf::store_be_f32(b.data()+at,v);}
inline std::vector<gf::u8> archive(bool explicit_child_uv=false,bool root_at_zero=false,bool mounted_part=false){
    std::vector<gf::u8> b(0x800,0);std::vector<gf::u32> fields;
    auto ptr=[&](unsigned at,unsigned value){word(b,at,value);fields.push_back(at);};
    const unsigned root=root_at_zero?0:0x40,scene=root_at_zero?0x740:0x10;
    ptr(scene,scene+0x10);ptr(scene+0x10,scene+0x20);ptr(scene+0x20,root);ptr(root+8,0x80);
    for(unsigned j:{root,0x80u}){word(b,j+4,j==root?0x20000092u:0x20000080u);for(unsigned i=0;i<3;++i)scalar(b,j+0x20+i*4,1);ptr(j+0x38,j==root?0xc0:0xf0);}
    scalar(b,0x80+0x2c,1);
    for(unsigned matrix:{0xc0u,0xf0u})for(unsigned i=0;i<3;++i)scalar(b,matrix+i*16+i*4,1);
    ptr(root+0x10,0x120);ptr(0x128,0x130);ptr(0x12c,0x150);ptr(0x138,0x500);ptr(0x13c,0x280);word(b,0x134,0x201d);
    ptr(0x158,0x180);word(b,0x15c,0xa0010003);ptr(0x160,0x220);ptr(0x164,0x300);
    const unsigned attrs[]={0,1,9,10};
    for(unsigned i=0;i<4;++i){const auto at=0x180+i*24;word(b,at,attrs[i]);word(b,at+4,1);word(b,at+8,attrs[i]==9?1:0);word(b,at+12,4);}
    word(b,0x1e0,255);b[0x220]=0x90;gf::store_be16(b.data()+0x221,3);
    // Clockwise world winding, matching the source/GX front-face contract.
    const float positions[3][3]={{-1,-1,0},{0,1,0},{1,-1,0}};
    for(unsigned v=0;v<3;++v){const auto at=0x223+v*26;b[at]=0;b[at+1]=30;
        for(unsigned k=0;k<3;++k){scalar(b,at+2+k*4,positions[v][k]);scalar(b,at+14+k*4,k==0?1:0);}}
    for(unsigned i=0;i<3;++i)word(b,0x280+i*4,0xffffffffu);
    scalar(b,0x28c,1);scalar(b,0x290,0);
    ptr(0x300,0x320);
    if(root_at_zero){ptr(0x320,0x80);scalar(b,0x324,1);}
    else{ptr(0x320,root);scalar(b,0x324,.25f);ptr(0x328,0x80);scalar(b,0x32c,.75f);}
    ptr(0x90,0x400);ptr(0x408,explicit_child_uv?0x440:0x130);ptr(0x40c,0x420);
    ptr(0x428,explicit_child_uv?0x580:0x180);word(b,0x42c,explicit_child_uv?0x80000004:0x80000003);
    ptr(0x430,explicit_child_uv?0x600:0x220);
    word(b,0x50c,4);for(unsigned i=0;i<3;++i)scalar(b,0x51c+i*4,1);b[0x53c]=b[0x53d]=1;word(b,0x540,0x40081);scalar(b,0x544,1);ptr(0x54c,0x560);
    ptr(0x560,0x700);gf::store_be16(b.data()+0x564,4);gf::store_be16(b.data()+0x566,4);word(b,0x568,6);
    for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){unsigned i=y*4+x;const bool red=x<2;b[0x700+i*2]=255;b[0x700+i*2+1]=red?255:0;b[0x720+i*2]=0;b[0x720+i*2+1]=red?0:255;}
    if(explicit_child_uv){
        word(b,0x444,0x201d);ptr(0x448,0x480);ptr(0x44c,0x280);
        word(b,0x48c,5);for(unsigned i=0;i<3;++i)scalar(b,0x49c+i*4,1);
        word(b,0x4b4,1);b[0x4bc]=b[0x4bd]=1;word(b,0x4c0,0x40010);scalar(b,0x4c4,1);ptr(0x4cc,0x560);
        const unsigned formats[]={9,10,14};
        for(unsigned i=0;i<3;++i){const auto at=0x580+24*i;word(b,at,formats[i]);word(b,at+4,1);word(b,at+8,i!=1);word(b,at+12,4);}
        word(b,0x5c8,255);b[0x600]=0x90;gf::store_be16(b.data()+0x601,3);
        for(unsigned i=0;i<3;++i){const auto at=0x603+32*i;
            for(unsigned k=0;k<3;++k){scalar(b,at+4*k,positions[i][k]);scalar(b,at+12+4*k,k==0?1:0);}
            scalar(b,at+24,.25f*i);scalar(b,at+28,.5f);
        }
    }
    if(mounted_part){
        const unsigned part_dobj=root_at_zero?0x780:0x740,part_pobj=part_dobj+0x20;
        ptr(scene+0x14,0x6e0);ptr(0x6e0,0x660);ptr(0x668,0x6a0);ptr(0x6b0,part_dobj);
        // Keep independent topology while sharing the same immutable display list.
        ptr(part_dobj+8,explicit_child_uv?0x440:0x130);ptr(part_dobj+0xc,part_pobj);
        ptr(part_pobj+8,explicit_child_uv?0x580:0x180);
        word(b,part_pobj+0xc,explicit_child_uv?0x80000004:0x80000003);
        ptr(part_pobj+0x10,explicit_child_uv?0x600:0x220);
        word(b,0x664,0x10000000);word(b,0x6a4,0x00050080);
        for(unsigned j:{0x660u,0x6a0u})for(unsigned i=0;i<3;++i)scalar(b,j+0x20+i*4,1);
        scalar(b,0x68c,100);scalar(b,0x6cc,3);
    }
    std::vector<gf::u8> out(32,0);out.insert(out.end(),b.begin(),b.end());
    for(auto field:fields){auto at=out.size();out.resize(at+4);word(out,at,field);}
    auto at=out.size();out.resize(at+8);word(out,at,scene);const char name[]="scene_data";out.insert(out.end(),name,name+sizeof(name));
    word(out,0,out.size());word(out,4,b.size());word(out,8,fields.size());word(out,12,1);return out;
}
inline std::vector<gf::u8> prefix(){std::vector<gf::u8> p(256,0);word(p,0,256);word(p,4,32);word(p,8,192);p[32]=1;p[33]=p[34]=255;p[36]=p[37]=255;p[38]=0;p[192]=0;p[193]=1;return p;}
inline std::vector<gf::u8> profile(){std::vector<gf::u8> p(44,0);word(p,0,0x47464350);gf::store_be16(p.data()+4,1);gf::store_be16(p.data()+6,2);p[8]=1;p[9]=p[10]=p[11]=2;word(p,12,32);word(p,16,34);word(p,20,36);word(p,24,p.size());word(p,28,0x40);p[33]=p[35]=1;word(p,36,0x40);word(p,40,0x80);return p;}
} // namespace character_model_fixture
