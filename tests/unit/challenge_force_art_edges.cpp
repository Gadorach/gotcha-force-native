#include <cstdio>
#include <cstdlib>
#include <vector>
#include "tests/unit/challenge_force_visual_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force art edges: %s\n",why);std::abort();}}
std::vector<gf::u8> encode(const gf::ChallengeForceArt& a){
    std::vector<gf::u8> b(244+a.random_name.size());gf::store_be32(b.data(),0x47465641);gf::store_be32(b.data()+4,1);gf::store_be32(b.data()+8,a.bank_hash);unsigned at=12;
    auto real=[&](float x){gf::store_be32(b.data()+at,gf::f32_to_bits(x));at+=4;};for(const auto& p:{a.card_hidden,a.card_active,a.card_exit,a.focus_hidden,a.focus_active,a.cost_hidden,a.cost_active})for(float x:p)real(x);
    auto part=[&](const auto& p){gf::store_be16(b.data()+at,p.record);at+=4;for(float x:p.position)real(x);};for(const auto& p:a.focus)part(p);for(const auto& p:a.cost)part(p);
    for(float x:a.name_origin)real(x);
    for(float x:{a.approach,a.snap,a.alpha_step,a.focus_alpha,a.name_extent,a.name_advance,a.digit_advance,a.angle_unit})real(x);
    gf::store_be16(b.data()+at,gf::u16(a.cost_angle));gf::store_be16(b.data()+at+2,a.shell_record);at+=4;gf::store_be16(b.data()+at,gf::u16(a.random_name.size()));at+=4;std::copy(a.random_name.begin(),a.random_name.end(),b.begin()+at);return b;
}}
int main(){
    using namespace gf;const auto art=challenge_force_visual_fixture::art();auto bytes=encode(art);ChallengeForceArt out;std::string error;check(out.load({bytes.data(),bytes.size()},&error)&&out.valid()&&out.random_name==art.random_name,"owned typed profile");
    for(unsigned n=0;n<bytes.size();++n)check(!out.load({bytes.data(),n},&error)&&!out.valid(),"truncated input clears output");
    for(unsigned at=12;at<96;at+=4){auto bad=bytes;store_be32(bad.data()+at,0x7fc00000);check(!out.load({bad.data(),bad.size()},&error),"position NaN refuses");}
    for(unsigned at=208;at<236;at+=4){auto bad=bytes;store_be32(bad.data()+at,0x7f800000);check(!out.load({bad.data(),bad.size()},&error),"scalar infinity refuses");}
    auto bad=bytes;store_be16(bad.data()+98,1);check(!out.load({bad.data(),bad.size()},&error),"part reserved word refuses");bad=bytes;store_be16(bad.data()+240,5);check(!out.load({bad.data(),bad.size()},&error),"encoded name extent refuses");
    ForceInventory owner;check(owner.create(force_inventory_fixture::catalog()),"inventory");const auto cards=challenge_force_visual_fixture::cards();ChallengeForceVisual visual;
    auto mismatch=cards;mismatch.bank_hash=2;check(!visual.open(art,mismatch,owner),"different bank hash refuses");
    for(const auto input:{ChallengeForceVisualInput{true,20,1500},ChallengeForceVisualInput{true,-2,1500},ChallengeForceVisualInput{true,0,0},ChallengeForceVisualInput{true,19,1500}}){check(visual.open(art,cards,owner)&&!visual.step(owner,input)&&visual.draws().empty(),"invalid selection/energy/empty force refuses");}
    {ForceInventory temporary;check(temporary.create(force_inventory_fixture::catalog())&&visual.open(art,cards,temporary),"temporary owner binding");}
    check(!visual.current(owner),"expired owner is not another identical inventory");
    std::printf("force art edges: %u owned checks PASS\n",checks);
}
