#pragma once
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>
#include "engine/core/types.hpp"

namespace gf {
// Authored stage presentation, not the retail visibility producer.
inline constexpr unsigned kStageViewFadeObstacles=8;
enum class StageViewFadeGate:u8 {Proximity,Occlusion};
struct StageViewFadeProfile {
    u16 alternate=0,normal=2;
    f32 range=0,low=0,entry=0,step=0;
    std::array<f32,3> lower{},upper{};
    StageViewFadeGate gate=StageViewFadeGate::Proximity;
};
inline bool stage_view_fade_load(std::string_view text,StageViewFadeProfile* out,std::string* error){
    if(out)*out={};
    auto fail=[&](){if(error)*error="stage viewport fade: expected one bounded GFSTAGE-FADE obstacle";return false;};
    constexpr std::string_view proximity="GFSTAGE-FADE 1\nobstacle=";
    constexpr std::string_view occlusion="GFSTAGE-FADE 2\ngate=occlusion\nobstacle=";
    if(!out||text.size()>1024)return fail();
    StageViewFadeProfile result;
    if(text.starts_with(proximity))text.remove_prefix(proximity.size());
    else if(text.starts_with(occlusion)){text.remove_prefix(occlusion.size());result.gate=StageViewFadeGate::Occlusion;}
    else return fail();
    if(text.ends_with('\n'))text.remove_suffix(1);
    std::array<std::string_view,11> fields;
    for(unsigned i=0;i<fields.size();++i){
        const auto at=text.find('|');
        if((i+1==fields.size())!=(at==std::string_view::npos))return fail();
        fields[i]=text.substr(0,at);
        if(fields[i].empty()||fields[i].size()>16)return fail();
        if(at!=std::string_view::npos)text.remove_prefix(at+1);
    }
    const auto record=std::from_chars(fields[0].data(),fields[0].data()+fields[0].size(),result.alternate);
    if(record.ec!=std::errc{}||record.ptr!=fields[0].data()+fields[0].size()||result.alternate<3||result.alternate>255)return fail();
    const auto read=[&](unsigned i,f32 min,f32 max,f32* value){
        const auto token=fields[i];const auto parsed=std::from_chars(token.data(),token.data()+token.size(),*value);
        return parsed.ec==std::errc{}&&parsed.ptr==token.data()+token.size()&&std::isfinite(*value)&&*value>=min&&*value<=max;
    };
    const bool camera=result.gate==StageViewFadeGate::Occlusion;
    if(!read(1,camera?0.f:1.f,camera?0.f:1000.f,&result.range)||!read(2,.05f,.95f,&result.low)||
       !read(3,result.low,.99f,&result.entry)||!read(4,.001f,.25f,&result.step))return fail();
    for(unsigned i=0;i<3;++i)
        if(!read(5+i,-10000,10000,&result.lower[i])||!read(8+i,result.lower[i],10000,&result.upper[i]))return fail();
    *out=result;if(error)error->clear();return true;
}

inline bool stage_view_fades_load(std::string_view text,std::vector<StageViewFadeProfile>* out,std::string* error){
    if(out)out->clear();
    auto fail=[&](){if(error)*error="stage viewport fade: invalid obstacle rows, record alias or budget";return false;};
    if(!out||text.size()>4096)return fail();
    constexpr std::string_view header="GFSTAGE-FADE 3\n";
    if(!text.starts_with(header)){
        StageViewFadeProfile legacy;
        if(!stage_view_fade_load(text,&legacy,error))return false;
        out->push_back(legacy);return true;
    }
    text.remove_prefix(header.size());
    std::vector<StageViewFadeProfile> next;
    std::array<bool,256> used{};
    while(!text.empty()){
        const auto end=text.find('\n');
        auto row=text.substr(0,end);
        text=end==std::string_view::npos?std::string_view{}:text.substr(end+1);
        if(next.size()>=kStageViewFadeObstacles||row.size()>200||!row.starts_with("obstacle="))return fail();
        row.remove_prefix(9);
        const auto split=row.find('|');
        if(split==std::string_view::npos)return fail();
        const auto mode=row.substr(0,split);row.remove_prefix(split+1);
        if(mode!="near"&&mode!="occlusion")return fail();
        const auto record_end=row.find('|');
        StageViewFadeProfile spec;
        u16 normal=0;
        if(record_end==std::string_view::npos)return fail();
        const auto parsed=std::from_chars(row.data(),row.data()+record_end,normal);
        if(parsed.ec!=std::errc{}||parsed.ptr!=row.data()+record_end||normal<2||normal>255)return fail();
        row.remove_prefix(record_end+1);
        const std::string legacy=(mode=="near"?"GFSTAGE-FADE 1\nobstacle=":"GFSTAGE-FADE 2\ngate=occlusion\nobstacle=")+std::string(row);
        if(!stage_view_fade_load(legacy,&spec,error))return false;
        if(normal==spec.alternate||used[normal]||used[spec.alternate])return fail();
        spec.normal=normal;used[normal]=used[spec.alternate]=true;next.push_back(spec);
    }
    if(next.empty())return fail();
    *out=std::move(next);if(error)error->clear();return true;
}

// Authored axis-aligned presentation box, no physics/retail shape receipt.
inline bool stage_view_fade_occludes(const StageViewFadeProfile& p,const std::array<f32,3>& eye,const std::array<f32,3>& target){
    f64 first=0,last=1;bool nonzero=false;
    for(unsigned k=0;k<3;++k){
        if(!std::isfinite(eye[k])||!std::isfinite(target[k]))return false;
        const f64 d=f64(target[k])-eye[k];
        if(!d){
            if(eye[k]<p.lower[k]||eye[k]>p.upper[k])return false;
            continue;
        }
        nonzero=true;
        const auto a=(p.lower[k]-f64(eye[k]))/d,b=(p.upper[k]-f64(eye[k]))/d;
        first=std::max(first,std::min(a,b));last=std::min(last,std::max(a,b));
        if(first>last)return false;
    }
    return nonzero;
}
inline bool stage_view_fade_near(const StageViewFadeProfile& p,const std::array<f32,3>& position){
    f32 distance=0;
    for(unsigned k=0;k<3;++k){
        const auto delta=position[k]-std::clamp(position[k],p.lower[k],p.upper[k]);distance+=delta*delta;
    }
    return distance<=p.range*p.range;
}

struct StageViewFadeState {
    f32 alpha=1;u32 tick=0;bool sampled=false;
    f32 update(const StageViewFadeProfile& p,const std::array<f32,3>& position,u32 now){
        return update_gate(p,stage_view_fade_near(p,position),now);
    }
    f32 update_gate(const StageViewFadeProfile& p,bool hidden,u32 now){
        if(sampled&&now==tick)return alpha;
        if(sampled&&now<tick)*this={};
        if(hidden)alpha=alpha>p.entry?p.entry:std::max(p.low,alpha-p.step);
        else alpha=alpha>=p.entry?1.f:std::min(p.entry,alpha+p.step);
        sampled=true;tick=now;return alpha;
    }
};
} // namespace gf
