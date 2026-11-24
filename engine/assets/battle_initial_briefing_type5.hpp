#pragma once
#include "engine/assets/force_editor_art.hpp"
namespace gf {
class BattleInitialBriefingType5Profile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto fail=[&](){if(error)*error="briefing type5: exact mode1/source36-card recipe required";return false;};
        const std::array<u32,32> h{0x47464235,1,208,0,0x80029434,0x80089898,0x80199c0c,0x80199c74,0x8019b358,0x8019bf18,0x80006fb4,0x80007d68,0x800092bc,0x80089100,0x80047aa4,0x80350d0c,0x87d2f859,202,206,0x42d858f4,5,36,5,1,0x11,15,0x41a60000,0x42260000,0xc197999a,0x3ecccccd,0xc3480000,0x80371130};
        const std::array<u32,18> t{0xc2bf0000,0xc03b573f,0xc3970000,0xc280f0a4,0xc29de60b,0xc3970000,0xc280f0a4,0xc29de60b,0xc3970000,0x4280f0a4,0x42557be7,0xc3970000,0x42bf0000,0xc1b4a5e3,0xc3970000,0x42bf0000,0xc1b4a5e3,0xc3970000};
        if(!b.contains(0,208)||b.size!=208)return fail();
        for(unsigned i=0;i<h.size();++i)if(b.be32(4*i)!=h[i])return fail();
        for(unsigned i=0;i<t.size();++i){if(b.be32(128+4*i)!=t[i])return fail();targets_[i/3][i%3]=bits_to_f32(t[i]);}
        if(b.be32(200)||b.be32(204))return fail();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const std::array<f32,3>* target(unsigned team,unsigned ordinal)const{return ready_&&team<2&&ordinal<3?&targets_[team*3+ordinal]:nullptr;}
    bool mapping_valid(const ForceEditorArt& art,const ForceCatalog& catalog)const{
        if(!ready_||!catalog.ready()||catalog.entries().size()!=206||art.icons.size()!=206)return false;
        u32 hash=2166136261u;
        for(unsigned i=0;i<art.icons.size();++i){
            const auto [model,record]=art.icons[i];if(model!=catalog.entries()[i].model||record>=202)return false;
            for(u8 byte:{u8(model>>8),u8(model),u8(record>>8),u8(record)})hash=(hash^byte)*16777619u;
        }
        return hash==0x42d858f4;
    }
private:bool ready_=false;std::array<std::array<f32,3>,6> targets_{};
};
}
