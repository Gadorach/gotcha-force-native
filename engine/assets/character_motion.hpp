#pragma once
// Original 8004BE08/8004D548: sixteen relative motion-bank tables, each
// 0xFFFFFFFF terminated. Runtime +1 tags mean "not instantiated yet"; this
// read-only native owner never installs host pointers or edits imported bytes.
// 8004ADC8/8024DC04 bind each selected HSD public entry by LOGICAL BONE INDEX,
// attaching its AObj only, not recursively walking the AnimJoint tree.
#include "engine/assets/hsd_bones.hpp"
namespace gf {
inline constexpr u32 kCharacterNoMotion=0xffffffffu;
struct CharacterMotionBank {
    std::array<std::vector<u32>,16> groups;
    std::vector<std::vector<u8>> archives;
    bool loaded=false;
};
struct CharacterMotionSelection {
    ByteView archive{}; // borrowed from the owned bank, not the input byte view
    u32 index=kCharacterNoMotion;
    bool fallback=false;
};
inline bool character_motion_bank_load(ByteView input,CharacterMotionBank* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character motion bank: ")+why;return false;};
    if(!out||!input.contains(0,64)||input.size>(64u<<20))return fail("input budget/destination");
    CharacterMotionBank next;
    struct Range{std::size_t at,size;};
    std::vector<Range> tables{{0,64}},clips;
    std::vector<u32> offsets;
    auto overlap=[](Range a,Range b){return a.at<b.at+b.size&&b.at<a.at+a.size;};
    for(unsigned group=0;group<16;++group){
        const u32 table=input.be32(group*4);if(!table)continue;
        if(table<64||(table&3))return fail("group table offset");
        bool ended=false;std::size_t entries=0;
        for(;entries<=256;++entries){
            const std::size_t at=std::size_t(table)+entries*4;
            if(!input.contains(at,4))return fail("unterminated group table");
            const u32 offset=input.be32(at);
            if(offset==kCharacterNoMotion){ended=true;break;}
            if(entries==256)return fail("group entry budget");
            if(!offset){next.groups[group].push_back(kCharacterNoMotion);continue;}
            if(offset<64||(offset&3)||!input.contains(offset,32))return fail("unrelocated archive offset");
            u32 index=0;
            while(index<offsets.size()&&offsets[index]!=offset)++index;
            if(index==offsets.size()){
                const u32 size=input.be32(offset);
                if(size<32||!input.contains(offset,size))return fail("archive extent");
                // Reject overlapping ranges, but permit exact archive/table aliases.
                Range range{offset,size};for(auto prior:clips)if(overlap(range,prior))return fail("overlapping archives");
                hsd_detail::Reader r;
                if(input.be32(offset+12)>4096||input.be32(offset+16)>4096)return fail("archive symbol budget");
                if(!hsd_anim_detail::open_container({input.data+offset,size},&r,error))return false;
                offsets.push_back(offset);clips.push_back(range);
                next.archives.emplace_back(input.data+offset,input.data+offset+size);
            }
            next.groups[group].push_back(index);
        }
        if(!ended)return fail("missing table terminator");
        Range range{table,(entries+1)*4};
        for(auto prior:tables)if(overlap(range,prior)&&(range.at!=prior.at||range.size!=prior.size))return fail("overlapping group tables");
        tables.push_back(range);
    }
    for(auto table:tables)for(auto clip:clips)if(overlap(table,clip))return fail("table/archive overlap");
    if(next.groups[0].empty()||next.groups[0][0]==kCharacterNoMotion)return fail("missing source default group0/entry0");
    next.loaded=true;*out=std::move(next);return true;
}
inline bool character_motion_select(const CharacterMotionBank& bank,u32 group,u32 choice,CharacterMotionSelection* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("character motion select: ")+why;return false;};
    // Selection of actor bank versus common bank via metadata bit7 belongs to
    // the caller. Do not silently mask that bit and choose the wrong bank here.
    if(!out||!bank.loaded||group>=16||choice>=128||bank.groups[0].empty())return fail("bank/group/signed-choice domain");
    const auto& entries=bank.groups[group];u32 index=kCharacterNoMotion;
    if(!entries.empty()){
        if(choice>=entries.size())return fail("choice outside declared table (not a source null entry)");
        index=entries[choice];
    }
    CharacterMotionSelection next;
    if(index==kCharacterNoMotion){index=bank.groups[0][0];next.fallback=true;}
    if(index>=bank.archives.size()||bank.archives[index].empty())return fail("archive/default binding");
    const auto& bytes=bank.archives[index];next.index=index;next.archive={bytes.data(),bytes.size()};*out=next;return true;
}
struct HsdBoneMotion {
    u32 joint_offset=0;
    bool has_aobj=false,classical_scale=false;
    std::vector<HsdAnimTrack> tracks;
};
struct HsdBoneMotionClip {
    std::vector<HsdBoneMotion> bones; // logical slots, not descriptor preorder
    u32 public_bone_count=0; // absent public != public with a null AObj
    f32 end_frame=0;
    bool loaded=false;
};
struct HsdBoneMotionValues {
    u32 joint_offset=0;
    u16 present=0; // bit = HSD channel number; absent channels are NOT defaults
    std::array<f32,11> value{};
};
struct HsdBoneMotionSample {std::vector<HsdBoneMotionValues> bones;};
inline bool hsd_bone_motion_load(ByteView archive,const HsdModelBones& binding,HsdBoneMotionClip* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd bone motion: ")+why;return false;};
    if(!out||!binding.loaded||binding.bones.empty()||binding.bones.size()>kHsdMaxCharacterBones||
       !archive.contains(0,32)||archive.size>(4u<<20))return fail("binding/input budget/destination");
    const u32 publics=archive.be32(12),externs=archive.be32(16);
    if(!publics||publics>binding.bones.size()||externs)return fail("public bone count/unsupported extern binding");
    for(std::size_t i=0;i<binding.bones.size();++i){
        if(!binding.bones[i].joint_offset)return fail("missing bone identity");
        for(std::size_t j=0;j<i;++j)if(binding.bones[j].joint_offset==binding.bones[i].joint_offset)return fail("ambiguous bone identity");
    }
    hsd_detail::Reader r;if(!hsd_anim_detail::open_container(archive,&r,error))return false;
    auto relocated=[&](u32 field){return std::binary_search(r.fields.begin(),r.fields.end(),field);};
    auto optional=[&](u32 field,u32* target){
        // A relocated zero is a real data-block address, NOT null. Only the
        // packed-payload-at-zero case below is admitted in this bounded path.
        if(!r.has(field,4)||(r.block.be32(field)==0&&relocated(field)))return false;
        return r.ptr(field,target);
    };
    const std::size_t symbols=32+r.block.size+std::size_t(archive.be32(8))*4,strings=symbols+std::size_t(publics)*8;
    HsdBoneMotionClip next;next.public_bone_count=publics;next.bones.resize(binding.bones.size());
    for(std::size_t i=0;i<binding.bones.size();++i)next.bones[i].joint_offset=binding.bones[i].joint_offset;
    std::vector<std::string> names;std::size_t bytes=0;
    for(u32 slot=0;slot<publics;++slot){
        const u32 name=archive.be32(symbols+slot*8+4);const std::size_t size=archive.be32(0);
        if(name>=size-strings)return fail("symbol name offset");
        const std::size_t begin=strings+name;std::size_t end=begin;
        while(end<size&&archive.data[end]){if(end-begin>=256)return fail("symbol name budget");++end;}
        if(end==size||end==begin)return fail("unterminated/empty symbol name");
        std::string text(reinterpret_cast<const char*>(archive.data+begin),end-begin);
        if(std::find(names.begin(),names.end(),text)!=names.end())return fail("ambiguous named lookup for public index");
        names.push_back(std::move(text));
        const u32 node=archive.be32(symbols+slot*8);u32 aobj=0,robj=0;
        if(!r.has(node,20)||!optional(node+8,&aobj)||!optional(node+12,&robj)||robj||r.block.be32(node+16)>1)
            return fail("AnimJoint/constraints/flags");
        // Source JObjAddAnim reads AObj, RObj, flags only. Child/next fields do
        // NOT define binding order and are deliberately not traversed here.
        auto& bone=next.bones[slot];bone.classical_scale=(r.block.be32(node+16)&1)!=0;
        if(!aobj)continue;
        u32 track=0,object=0;
        if(!r.has(aobj,16)||r.block.be32(aobj)!=0||!optional(aobj+8,&track)||!track||!optional(aobj+12,&object)||object)
            return fail("unsupported AObj flags/object/track binding");
        const f32 end_frame=r.block.bef32(aobj+4);
        if(!std::isfinite(end_frame)||end_frame<=0||end_frame>8192||std::floor(end_frame)!=end_frame)return fail("integer-time clip end budget");
        if(next.end_frame&&end_frame!=next.end_frame)return fail("mixed bone end times require AObj lifecycle evaluation");
        next.end_frame=end_frame;bone.has_aobj=true;u16 channels=0;std::vector<u32> seen;
        while(track){
            if(seen.size()>=9||std::find(seen.begin(),seen.end(),track)!=seen.end()||!r.has(track,20))return fail("track count/cycle/descriptor");
            seen.push_back(track);const u8 channel=r.block.data[track+12];
            if(!((channel>=1&&channel<=3)||(channel>=5&&channel<=10))||(channels&(1u<<channel)))return fail("unsupported/duplicate SRT channel");
            channels|=u16(1u<<channel);const u32 length=r.block.be32(track+4),data=r.block.be32(track+16);
            // Original8025B00C adds block base to EVERY relocated field,
            // including stored offset0. Actual motion clips put the first
            // packed payload at block0; don't use the generic null classifier.
            if(!relocated(track+16)||!r.has(data,length)||!length||length>hsd_anim_detail::kMaxTrackBytes||length>(2u<<20)-bytes)return fail("relocated payload extent/budget");
            bytes+=length;HsdAnimTrack parsed;parsed.channel=channel;parsed.frac_value=r.block.data[track+13];parsed.frac_slope=r.block.data[track+14];parsed.startframe=r.block.bef32(track+8);
            if(!std::isfinite(parsed.startframe)||parsed.startframe<0||parsed.startframe>8192)return fail("track start budget");
            parsed.data.assign(r.block.data+data,r.block.data+data+length);
            HsdAnimTrackView view{{parsed.data.data(),parsed.data.size()},parsed.frac_value,parsed.frac_slope,parsed.startframe};f32 sample=0;
            if(!hsd_anim_sample_requested_track(view,0,&sample,error)||!hsd_anim_sample_requested_track(view,u32(end_frame),&sample,error))return false;
            bone.tracks.push_back(std::move(parsed));
            if(!optional(track,&track))return fail("track next pointer");
        }
    }
    next.loaded=true;*out=std::move(next);return true;
}
// Explicit source AObj request: nonloop FIRST_PLAY samples the requested time
// even beyond end_frame, BEFORE the actor/AObj lifecycle stops it (80243BD0).
// This does not loop, clamp, advance a controller or simulate repeated ticks.
inline bool hsd_bone_motion_sample_requested(const HsdBoneMotionClip& clip,u32 requested_time,HsdBoneMotionSample* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd bone motion sample: ")+why;return false;};
    if(!out||!clip.loaded||clip.bones.empty()||clip.bones.size()>kHsdMaxCharacterBones||!std::isfinite(clip.end_frame)||
       clip.end_frame<0||clip.end_frame>8192||requested_time>8192)return fail("clip/time/destination domain");
    HsdBoneMotionSample next;next.bones.resize(clip.bones.size());std::size_t bytes=0;
    for(std::size_t i=0;i<clip.bones.size();++i){const auto& bone=clip.bones[i];auto& target=next.bones[i];target.joint_offset=bone.joint_offset;
        if(!bone.joint_offset||bone.tracks.size()>9||(!bone.has_aobj&&!bone.tracks.empty()))return fail("bone/track binding");
        for(std::size_t j=0;j<i;++j)if(clip.bones[j].joint_offset==bone.joint_offset)return fail("ambiguous bone identity");
        for(const auto& track:bone.tracks){const auto channel=track.channel;
            if(!((channel>=1&&channel<=3)||(channel>=5&&channel<=10))||(target.present&(1u<<channel)))return fail("unsupported/duplicate channel");
            if(track.data.size()>(2u<<20)-bytes)return fail("payload budget");
            bytes+=track.data.size();HsdAnimTrackView view{{track.data.data(),track.data.size()},track.frac_value,track.frac_slope,track.startframe};
            if(!hsd_anim_sample_requested_track(view,requested_time,&target.value[channel],error))return false;
            target.present|=u16(1u<<channel);
        }
    }
    *out=std::move(next);return true;
}
// Preserve the earlier declared-range API. Callers needing the source's explicit
// out-of-range request must opt into the separate operation above.
inline bool hsd_bone_motion_sample(const HsdBoneMotionClip& clip,u32 requested_time,HsdBoneMotionSample* out,std::string* error){
    if(requested_time>clip.end_frame){
        if(out)*out={};
        if(error)*error="hsd bone motion sample: time exceeds declared clip end";
        return false;
    }
    return hsd_bone_motion_sample_requested(clip,requested_time,out,error);
}
} // namespace gf
