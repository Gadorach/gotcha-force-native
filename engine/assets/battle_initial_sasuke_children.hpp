#pragma once
#include "engine/assets/startup_attachments.hpp"
namespace gf {
//Source-qualified envelope around the EXISTING GFAT/state implementation.
//Only the unchanged normal Sasuke bank/type6+7 rows; not isolated-parent ACK.
class BattleInitialSasukeChildrenProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;profile_={};auto fail=[&](){if(error)*error="initial Sasuke children: exact source callback/bank/GFAT recipe required";return false;};
        if(!b.contains(0,276)||b.size!=276||b.be32(0)!=0x47465343||b.be32(4)!=1||b.be32(8)!=276||b.be32(12))return fail();
        const std::array<u32,12> keys{0x80089a94,0x8008997c,0x8004d7fc,0x800660e8,0x8004d850,0x8004d978,0x80089100,0x80007030,0x80007c54,0x80007cac,0x80047aa4,0x8004dabc};
        for(unsigned i=0;i<keys.size();++i)if(b.be32(16+4*i)!=keys[i])return fail();
        const std::array<u32,4> meta{10,0x3ecccccd,0x11,2};for(unsigned i=0;i<meta.size();++i)if(b.be32(64+4*i)!=meta[i])return fail();
        const std::array<u32,49> canonical{
            0x47464154,0x00010002,0xc4,0xeab3632a,0x000a0003,0,0x3f800000,0,0x3dcccccd,
            0x064b000b,0,0x07000000,0,0xc1000000,0xc04ccccd,0x40c66666,0,
            0x0d00c000,0x00004000,0xc1fc28f6,0xbfdd70a4,0xc1533333,0,
            0x07000000,0,0xc1000000,0xc04ccccd,0x40c66666,0,
            0x074b000e,0,0x0b000000,0,0x41a00000,0xc1a00000,0,0,
            0x0b000000,0,0x41a00000,0xc1a00000,0,0,
            0x0b000000,0,0x41a00000,0xc1a00000,0,0
        };
        for(unsigned i=0;i<canonical.size();++i)if(b.be32(80+4*i)!=canonical[i])return fail();
        if(!startup_attachments_load({b.data+80,196},0xeab3632a,&profile_,error))return false;
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
    const StartupAttachmentProfile* attachments()const{return ready_?&profile_:nullptr;}
private:bool ready_=false;StartupAttachmentProfile profile_;
};
}
