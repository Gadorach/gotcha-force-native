#pragma once
// EFB3C/EFCB8 -> non-random F0F84/F1238 -> ED614/684/6F4/764 queue subset.
// Packets retain a known-byte mask: source stack padding is NOT invented data.
// Candidate queue admission is NOT command execution, voice or PCM completion.
#include <algorithm>
#include "engine/assets/startup_sound_commands.hpp"
#include "engine/gameplay/startup_sound_spatial.hpp"
namespace gf {
struct StartupSoundPacket {
    std::array<u8,16> bytes{};u16 known=0;
    bool operator==(const StartupSoundPacket&)const=default;
};
struct StartupSoundCommandQueue {
    std::array<StartupSoundPacket,32> packets{};u32 count=0,start_calls=0;
    bool operator==(const StartupSoundCommandQueue&)const=default;
};
struct StartupSoundCommandObservation {bool gate_declared=false,settings_declared=false;u8 disabled=0;s8 sfx_index=0;};
struct StartupSoundCommandOutcome {unsigned added=0,visited=0;bool full=false,ignored=false,index_limit=false;};
struct StartupSoundCommandPlan {
    StartupSoundCommandQueue candidate;std::array<StartupSoundCommandOutcome,48> outcomes{};unsigned count=0;
};
namespace startup_sound_commands_detail {
inline bool queue_valid(const StartupSoundCommandQueue& q){
    if(q.count>32)return false;
    for(const auto& packet:q.packets)for(unsigned i=0;i<16;++i)if(!(packet.known&(1u<<i))&&packet.bytes[i])return false;
    return true;
}
inline void byte(StartupSoundPacket& p,unsigned at,u8 value){p.bytes[at]=value;p.known=u16(p.known|(1u<<at));}
inline void word(StartupSoundPacket& p,u32 value){for(unsigned i=0;i<4;++i)byte(p,i,u8(value>>(24-8*i)));}
inline bool enqueue(StartupSoundCommandQueue& q,StartupSoundPacket* scratch,u8 opcode){
    if(q.count>=32)return false; // source checks capacity BEFORE rewriting header
    u32 head=load_be32(scratch->bytes.data());head=opcode==6?(head&0x00ffffffu):(head&0x0fffffffu);
    word(*scratch,head|(u32(opcode)<<28));q.packets[q.count++]=*scratch;return true;
}
struct Parameters {bool update=false,replace=true;u32 tone=0;u8 group=0;s32 volume=0,pan=0,pitch=8192;s8 key_offset=0;u8 bank=0;};
inline bool chain(const StartupSoundToneBank& bank,Parameters params,StartupSoundCommandQueue* q,StartupSoundCommandOutcome* outcome,std::string* error){
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound tone chain: ")+why;return false;};
    if(params.bank>127||params.group>127||params.volume<-1||params.volume>127||params.pan<-1||params.pan>127||params.pitch<-1||params.pitch>16383)return fail("bounded parameters");
    std::array<bool,128> seen{};StartupSoundPacket scratch;
    // One scratch frame per invocation. Known tail bytes survive within a chain,
    // but source-unwritten initial bytes stay UNKNOWN, never source-zero credit.
    for(unsigned steps=0;steps<128;++steps){
        if(params.tone>=128){outcome->index_limit=true;return true;}
        if(params.tone>=bank.tones.size()||seen[params.tone])return fail("missing/cyclic tone");
        seen[params.tone]=true;++outcome->visited;const auto& t=bank.tones[params.tone];
        if(t[0]==255)return true; // source no-op ends chain even if next!=FF
        if(t[0]!=0&&t[0]!=1)return fail("unknown tone type");
        const u32 identity=u32(params.bank)<<16|u32(params.tone&127u)<<8|u32(t[6]&128u)|u32((u32(t[6]&127u)+params.group)&127u);
        u8 opcode=3;
        if(t[0]==1){word(scratch,0x07000000u|identity);byte(scratch,4,u8(s32(t[2])+params.key_offset));byte(scratch,5,t[3]);}
        else{
            if(t[12]||t[13])return fail("randomized tone requires unported RNG consumer");
            if(params.pan==-1){if(!params.update)params.pan=t[9]==255?64:t[9];}
            else if(t[9]!=255)params.pan=std::clamp(params.pan+s32(t[9])-64,0,127);
            const s32 base=std::bit_cast<s16>(load_be16(t.data()+10));
            if(!params.update||params.pitch!=-1)params.pitch=std::clamp(base+(params.pitch==-1?8192:params.pitch),0,16383);
            if(params.volume==-1&&!params.update)params.volume=127;
            if(params.volume!=-1)params.volume=std::clamp(params.volume*s32(t[8]==255?127:t[8])/127,0,127);
            word(scratch,(params.update?0x07000000u:0u)|identity);byte(scratch,4,t[2]);byte(scratch,5,t[3]);byte(scratch,6,t[4]);byte(scratch,7,t[5]);
            byte(scratch,8,u8(params.volume));byte(scratch,9,u8(params.pan));byte(scratch,10,u8(u32(params.pitch)>>8));byte(scratch,11,u8(params.pitch));byte(scratch,12,u8(params.key_offset));byte(scratch,13,t[7]);
            opcode=params.update?(params.replace?5:4):6;
        }
        if(!enqueue(*q,&scratch,opcode)){outcome->full=true;return true;}
        ++outcome->added;if(t[15]==255)return true;params.tone=t[15];
    }
    return fail("tone work budget");
}
template<class Resolve>
inline bool plan_bound(const StartupSoundCommandProfile& p,Resolve resolve,
                                       const StartupSoundCommandQueue& input,const StartupSoundCommandObservation& observation,
                                       std::span<const StartupSoundScalarCommand> calls,StartupSoundCommandPlan* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("startup sound commands: ")+why;return false;};
    using namespace startup_sound_commands_detail;
    if(!out||!startup_sound_command_profile_valid(p)||!queue_valid(input)||calls.size()>48)return fail("profile/queue/budget");
    StartupSoundCommandPlan plan;plan.candidate=input;
    for(const auto& call:calls){auto& outcome=plan.outcomes[plan.count];
        if(!observation.gate_declared)return fail("source sound gate observation required");
        if(observation.disabled||call.cue>=384){outcome.ignored=true;++plan.count;continue;}
        if(call.cue<0||call.actor96<0||call.actor96>=6||call.volume>127||call.pan>127||call.pitch>16383)return fail("scalar call domain");
        if(!observation.settings_declared||observation.sfx_index<0||observation.sfx_index>=5)return fail("source SFX setting index required");
        const StartupSoundToneBank* bank=nullptr;u8 bank_id=0;u32 tone=0;if(!resolve(call.cue,&bank,&bank_id,&tone,error))return false;
        Parameters parameters;parameters.update=call.update;parameters.bank=bank_id;parameters.tone=tone;parameters.group=u8(u8(call.actor96)*8u);parameters.volume=s32(f32(f32(call.volume)*p.gains[unsigned(observation.sfx_index)]));parameters.pan=call.pan;parameters.pitch=call.pitch;
        if(!chain(*bank,parameters,&plan.candidate,&outcome,error))return false;
        // EFB3C increments even after full queue/no-op/index-limit. EFCB8 doesn't.
        if(!call.update)plan.candidate.start_calls+=1u;
        ++plan.count;
    }
    *out=plan;if(error)error->clear();return true;
}
} // namespace startup_sound_commands_detail
inline bool startup_sound_command_plan(const StartupSoundCommandProfile& p,const StartupSoundToneBank* bank,const StartupSoundCommandQueue& input,const StartupSoundCommandObservation& observation,std::span<const StartupSoundScalarCommand> calls,StartupSoundCommandPlan* out,std::string* error){
    auto resolve=[&](s16 cue,const StartupSoundToneBank** selected,u8* bank_id,u32* tone,std::string* why){if(cue>=128||!bank||!startup_sound_tones_valid(p,*bank)){if(why)*why="startup sound commands: only explicitly bound common01 bank0 admitted";return false;}*selected=bank;*bank_id=0;*tone=u32(cue);return true;};
    return startup_sound_commands_detail::plan_bound(p,resolve,input,observation,calls,out,error);
}
} // namespace gf
