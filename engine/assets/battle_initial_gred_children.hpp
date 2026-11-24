#pragma once
#include "engine/assets/startup_attachments.hpp"
namespace gf {
class BattleInitialGRedChildrenProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;profile_={};auto fail=[&](){if(error)*error="initial G-Red children: exact slot3/type24-25 source recipe required";return false;};
        if(!b.contains(0,180)||b.size!=180||b.be32(0)!=0x47464743||b.be32(4)!=1||b.be32(8)!=180||b.be32(12))return fail();
        const std::array<u32,12> keys{0x80089a94,0x8008997c,0x801860c4,0x800660e8,0x80186118,0x801862d4,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4,0x80186480};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,4> meta{0x615,0x3ecccccd,0x13,3};for(unsigned i=0;i<meta.size();++i)if(b.be32(64+4*i)!=meta[i])return fail();
        const std::array<u32,25> canonical{0x47464154,0x00010002,100,0xe2b4439a,0x06150101,0,0x3f800000,0,0x3dcccccd,0x184b000a,0x01000000,0x07004000,0x0000c000,0xc0d9999a,0xc0666666,0,0,0x19410009,0x02000000,0x07000000,0,0xc0c7ae14,0xc091eb85,0,0};
        for(unsigned i=0;i<canonical.size();++i)if(b.be32(80+4*i)!=canonical[i])return fail();
        if(!startup_attachments_load({b.data+80,100},0xe2b4439a,&profile_,error))return false;
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const StartupAttachmentProfile* attachments()const{return ready_?&profile_:nullptr;}
private:bool ready_=false;StartupAttachmentProfile profile_;
};
}
