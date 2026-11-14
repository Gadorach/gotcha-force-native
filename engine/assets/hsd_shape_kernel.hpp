#pragma once
#include "engine/assets/hsd_record_animation.hpp"
namespace gf::hsd_shape_detail {
struct Mesh {
    HsdMesh original;std::vector<std::vector<std::array<f32,3>>> shapes;
    HsdAnimTrack track;f32 end=0;
};
// Shared indexed average-shape decoder. Callers separately prove the published
// joint/DObj/POBJ/animation pairing; this kernel never discovers candidate roots.
inline bool load(const hsd_detail::Reader& r,const HsdMesh& original,u32 set,u32 aobj,Mesh* out,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("HSD shape kernel: ")+why;return false;};
    if(!out||!r.has(set,28))return fail("shape descriptor");
    const auto flags=r.block.be16(set),count=r.block.be16(set+2);const auto positions=r.block.be32(set+4);u32 descriptor=0,indices=0,normal=0,normal_indices=0;
    if((flags!=1&&flags!=5)||!count||count>64||!positions||positions>4096||!hsd_record_detail::optional(r,set+8,&descriptor)||!descriptor||!r.has(descriptor,24)||!hsd_record_detail::optional(r,set+12,&indices)||!indices||r.block.be32(set+16)||!hsd_record_detail::optional(r,set+20,&normal)||normal||!hsd_record_detail::optional(r,set+24,&normal_indices)||normal_indices)return fail("average position-only shape domain");
    hsd_detail::VtxDesc vertex;vertex.attr=r.block.be32(descriptor);vertex.type=r.block.be32(descriptor+4);vertex.comp_count=r.block.be32(descriptor+8);vertex.comp_type=r.block.be32(descriptor+12);vertex.shift=r.block.data[descriptor+16];vertex.stride=r.block.be16(descriptor+18);vertex.array=r.block.be32(descriptor+20);
    if(vertex.attr!=9||(vertex.type!=2&&vertex.type!=3)||vertex.comp_count!=1||vertex.comp_type!=4||vertex.stride<12||!std::binary_search(r.fields.begin(),r.fields.end(),descriptor+20))return fail("indexed F32 XYZ shape descriptor required");
    std::vector<std::vector<std::array<f32,3>>> shapes(count);
    for(unsigned shape=0;shape<count;++shape){u32 map=0;if(!hsd_record_detail::optional(r,indices+shape*4,&map)||!map||!r.has(map,std::size_t(positions)*(vertex.type==3?2:1)))return fail("shape index map extent");shapes[shape].reserve(positions);
        for(unsigned i=0;i<positions;++i){const u32 index=vertex.type==3?r.block.be16(map+i*2):r.block.data[map+i];HsdVertex value;bool hp=false,hn=false,hu=false,hc=false;if(!hsd_detail::read_element(r,vertex,index,&value,&hp,&hn,&hu,&hc,error)||!hp)return false;shapes[shape].push_back({value.position[0],value.position[1],value.position[2]});}
    }
    for(const auto& v:original.vertices)if(v.source_position_index>=positions||v.matrix_index!=0)return fail("display position index outside evaluated shape");
    hsd_record_detail::Object animation;std::size_t payload=0;if(!hsd_record_detail::object(r,aobj,false,&animation,&payload)||animation.tracks.size()!=1||animation.tracks[0].channel!=1)return fail("average scalar shape AObj required");
    *out={original,std::move(shapes),std::move(animation.tracks[0]),animation.end};return true;
}
inline bool sample_impl(const Mesh& mesh,f64 frame,bool fractional,std::vector<HsdVertex>* out,f32* blend,std::string* error){
    if(!out||mesh.shapes.empty()||!std::isfinite(frame)||frame<0||frame>hsd_anim_detail::kMaxFrame||frame>mesh.end)return false;
    const auto& track=mesh.track;f32 value=0;const HsdAnimTrackView view{{track.data.data(),track.data.size()},track.frac_value,track.frac_slope,track.startframe};
    const bool sampled=fractional?hsd_anim_sample_requested_track_fractional(view,f32(frame),&value,error):hsd_anim_sample_requested_track(view,u32(frame),&value,error);
    if(!sampled||!std::isfinite(value)||std::abs(value)>1e7f)return false;
    const auto first=unsigned(std::clamp(s32(value),s32(0),s32(mesh.shapes.size()-1))),second=std::min(first+1,unsigned(mesh.shapes.size()-1));const f32 fraction=std::clamp(f32(value-f32(first)),0.f,1.f);auto result=mesh.original.vertices;
    for(auto& vertex:result)for(unsigned k=0;k<3;++k){const auto a=mesh.shapes[first][vertex.source_position_index][k],b=mesh.shapes[second][vertex.source_position_index][k];vertex.position[k]=std::fma(fraction,f32(b-a),a);if(!std::isfinite(vertex.position[k]))return false;}
    *out=std::move(result);if(blend)*blend=value;return true;
}
inline bool sample(const Mesh& mesh,u32 frame,std::vector<HsdVertex>* out,f32* blend,std::string* error){return sample_impl(mesh,frame,false,out,blend,error);}
inline bool sample_fractional(const Mesh& mesh,f32 frame,std::vector<HsdVertex>* out,f32* blend,std::string* error){return sample_impl(mesh,frame,true,out,blend,error);}
} // namespace gf::hsd_shape_detail
