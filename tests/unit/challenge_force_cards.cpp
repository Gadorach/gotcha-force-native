#include <cstdio>
#include <cstdlib>
#include <vector>
#include "engine/gameplay/challenge_force_card_layout.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force cards: %s\n",why);std::abort();}}
gf::ChallengeForceCardsArt owned(){
    gf::ChallengeForceCardsArt p;p.bank_hash=1;
    for(unsigned i=0;i<4;++i)p.formats[i]={gf::u16(i),gf::s16(i==3?0:(i+1)*100)};
    for(unsigned i=0;i<9;++i)p.single_row[i]={-gf::f32(i),2};
    p.single_z={3,4,5};p.scale_step=.25f;p.scale_min=1;p.scale_max=2;p.y=.5f;p.z_scale=1;
    p.column_step=2;p.row_span=7.5f;p.row_round=.5f;p.x_even=7;p.x_odd=6;p.z_top=6;p.z_step=4;p.grid_scale=.5f;
    p.row_colour_step=20;p.rejected_alpha=.25f;p.child_z=2;return p;
}
std::vector<gf::u8> encode(const gf::ChallengeForceCardsArt& p){
    std::vector<gf::u8> b(176);gf::store_be32(b.data(),0x47464643);gf::store_be32(b.data()+4,1);gf::store_be32(b.data()+8,p.bank_hash);unsigned at=12;
    for(const auto& f:p.formats){gf::store_be16(b.data()+at,f.record);gf::store_be16(b.data()+at+2,gf::u16(f.angle));at+=4;}
    auto put=[&](float v){gf::store_be32(b.data()+at,gf::f32_to_bits(v));at+=4;};
    for(const auto& row:p.single_row)for(float v:row)put(v);
    for(float v:p.values())put(v);
    return b;
}
}
int main(){
    using namespace gf;auto p=owned();check(p.valid(),"owned profile");auto bytes=encode(p);ChallengeForceCardsArt loaded;std::string error;
    check(challenge_force_cards_load({bytes.data(),bytes.size()},&loaded,&error)&&loaded.values()==p.values(),"typed round trip");
    for(unsigned n=1;n<=30;++n){
        ChallengeForceCardsArt::Format format;check(challenge_force_card_format(p,n,false,&format)&&format.record==unsigned(n>=16)+unsigned(n>=24),"count cuts");
        check(challenge_force_card_format(p,n,true,&format)&&format.record==3&&format.angle==0,"Random format independent of count");
        for(unsigned slot=0;slot<n;++slot){ChallengeForceCardMember a,b;check(challenge_force_card_member(p,n,slot,n,&a)&&challenge_force_card_member(p,n,slot,slot,&b),"prefix endpoints");
            check(a.position==b.position&&a.scale==b.scale&&a.row==b.row&&a.alpha==1&&b.alpha==p.rejected_alpha&&b.source_rgb==0,"rejection changes colour/alpha, not placement");
            const unsigned row=n<=8?0:slot<8?0:slot<15?1:slot<23?2:3;check(a.row==row&&a.source_rgb==255-row*20,"alternating row boundaries");
            if(n>8){const unsigned start[4]={0,8,15,23};check(a.position[0]==2.f*(slot-start[row])-(row&1?6.f:7.f)&&a.position[2]==6.f-4*row&&a.scale[0]==.5f,"owned grid values");}
            else check(a.position[0]==-f32(n)+2.f*slot&&a.position[2]==f32(n<4?3:(slot&1)?4:5)&&a.scale[0]==std::min(1.f+.25f*(8-n),2.f),"owned single-row values");
        }
    }
    ChallengeForceCardMember member;ChallengeForceCardsArt::Format format;
    check(!challenge_force_card_member(p,0,0,0,&member)&&member.alpha==0,"empty member refuses");
    check(!challenge_force_card_member(p,31,0,0,&member),"count cap");check(!challenge_force_card_member(p,30,30,30,&member),"slot cap");
    check(!challenge_force_card_member(p,30,0,31,&member),"prefix cap");check(!challenge_force_card_member(p,1,0,1,nullptr),"null output");
    check(!challenge_force_card_format(p,0,false,&format)&&challenge_force_card_format(p,0,true,&format),"only Random has no stored members");
    check(!challenge_force_card_format(p,31,true,&format),"Random does not bypass count guard");
    for(unsigned at=100;at<bytes.size();at+=4){auto bad=bytes;store_be32(bad.data()+at,0x7fc00000);check(!challenge_force_cards_load({bad.data(),bad.size()},&loaded,&error)&&!loaded.valid(),"nonfinite scalar clears output");}
    for(unsigned at:{0u,4u,8u}){auto bad=bytes;store_be32(bad.data()+at,0);check(!challenge_force_cards_load({bad.data(),bad.size()},&loaded,&error),"header binding guard");}
    auto bad=p;bad.formats[1].record=bad.formats[0].record;check(!bad.valid(),"duplicate format refuses");bad=p;bad.formats[3].angle=1;check(!bad.valid(),"Random tilt refuses");
    bad=p;bad.row_span=1e6f;check(bad.valid()&&!challenge_force_card_member(bad,30,29,30,&member)&&member.alpha==0,"conversion bounded before cast");
    std::printf("force cards: %u owned checks PASS\n",checks);
}
