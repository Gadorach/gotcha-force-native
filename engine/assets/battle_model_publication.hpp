#pragma once
#include "engine/assets/model_pass_queue.hpp"
namespace gf {
//The bounded world-group0 actor callback slice. It does not select a camera or
//acknowledge preparation/stage/child callback completion.
class BattleModelPublicationProfile {
public:
    bool load(ByteView b,std::string* error){
        ready_=false;auto reject=[&](){if(error)*error="battle model publication: source callback recipe";return false;};
        if(!b.contains(0,80)||b.size!=80||b.be32(0)!=0x47464250||b.be32(4)!=1||b.be32(8)!=80||b.be32(12))return reject();
        const std::array<u32,16> words{0x8008a16c,0x80053928,0x8008c098,0x8008c0dc,0x8008a0a8,0x80009790,0x80056aa8,0x800097b4,0x8004fcfc,0x8004fdf0,0x8004fe60,0x800076d0,0x47,0,0,0};
        for(unsigned i=0;i<words.size();++i)if(b.be32(16+4*i)!=words[i])return reject();
        ready_=true;if(error)error->clear();return true;
    }
    bool ready()const{return ready_;}
private:bool ready_=false;
};
}
