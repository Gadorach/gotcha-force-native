#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "tests/unit/startup_actor_setup_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"startup actor setup: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(){
    using namespace gf;using namespace startup_actor_setup_fixture;
    auto bytes=profile();StartupActorSetupProfile p;std::string error;
    check(startup_actor_setup_load({bytes.data(),bytes.size()},1234,&p,&error),"owned model-bound profile loads");
    StartupActorInitialSetup s;StartupActorInitialSetupInputs input;check(startup_actor_initial_setup(p,input,&s,&error),"fresh numeric source branch");
    const f32 scale=1.f+16.f*.1f;check(s.rank.requested==16&&s.rank.selected==16&&s.scale_b4==scale&&s.rank.scale_b8==scale&&s.fieldc0==scale&&s.fieldc4==scale&&s.actor_scale==std::array<f32,3>{scale,scale,scale},"constructor rank16, not assumed unit scales or reversed rank bounds");
    check(s.health736==1234&&s.health738==1234&&s.field5f4==1&&s.field768==1&&s.fields678==std::array<f32,3>{-1,0,1},"source counter/health/copy fields preserve values");
    check(s.field668==f32(80*scale)&&s.field66c==f32(30*scale)&&s.field670==f32(5*scale)&&s.field674==f32(20*scale)&&s.field770==f32(45*scale)&&s.field7fc==1,"scaled dimensions vs UNscaled max/divisor, no double scale");
    for(unsigned encoded=0;encoded<256;++encoded){const s8 rank=std::bit_cast<s8>(u8(encoded));const unsigned selected=rank<8?8:rank>31?31:unsigned(rank);StartupActorRankState r;
        check(startup_actor_rank_select(p,rank,&r)&&r.requested==encoded&&r.selected==selected&&r.scale_b8==p.ranks[selected][0]&&r.field1dd0==1&&r.field5f8==1,"signed8 rank clamp and complete three-scalar tuple for every input");}
    const auto unchanged=fields(s);
    for(unsigned i=0;i<4;++i){input={};if(i==0)input.field489=1;else if(i==1)input.flags48a=1;else if(i==2)input.field500=1;else input.field400=1;
        check(!startup_actor_initial_setup(p,input,&s,&error)&&fields(s)==unchanged,"unqualified nonfresh/alternate scale branch refuses without partial publication");}
    input={};input.field400=NAN;check(!startup_actor_initial_setup(p,input,&s,&error)&&fields(s)==unchanged,"nonfinite branch input refuses");input={};
    auto derived=p;derived.class_config.fill(0);for(unsigned at:{0x44u,0x48u,0x4cu,0x50u,0x54u})store_be_f32(derived.class_config.data()+at,1e4f);for(auto& r:derived.ranks)r[0]=1e4f;
    check(!startup_actor_initial_setup(derived,input,&s,&error)&&fields(s)==unchanged,"finite but overbudget derived dimensions refuse atomically");
    check(!startup_actor_initial_setup(p,input,nullptr,&error)&&!startup_actor_rank_select(p,16,nullptr),"missing destinations refuse");
    for(unsigned at=0;at<48;++at){auto bad=bytes;bad[at]^=1;auto rejected=p;check(!startup_actor_setup_load({bad.data(),bad.size()},1234,&rejected,&error)&&!rejected.loaded,"every header byte bound to version/identity/table/extent constraints");}
    for(auto [at,bits]:std::array<std::pair<unsigned,u32>,7>{{{672,0x7fc00000},{480+0x4c,0xbf800000},{1056,0},{1060,0},{48+0x74,0x7fc00000},{48+4,0xffff0000},{672,0}}}){auto bad=bytes;store_be32(bad.data()+at,bits);check(!startup_actor_setup_load({bad.data(),bad.size()},1234,&p,&error),"invalid source scalar/counter/rank domain refuses");}
    for(auto [at,value]:std::array<std::pair<unsigned,u8>,4>{{{48+0xa2,32},{48+0xa1,7},{48+0xa3,1},{48+0xa2,255}}}){auto bad=bytes;bad[at]=value;check(!startup_actor_setup_load({bad.data(),bad.size()},1234,&p,&error),"rank bounds/group unknowns refuse, not reversed or unsigned reinterpretation");}
    check(!startup_actor_setup_load({bytes.data(),bytes.size()-1},1234,&p,&error)&&!startup_actor_setup_load({bytes.data(),bytes.size()},1235,&p,&error),"extent and foreign model scope refuse");
    check(startup_actor_setup_load({bytes.data(),bytes.size()},1234,&p,&error),"restore owned profile");bytes.assign(bytes.size(),0);check(startup_actor_initial_setup(p,input,&s,&error)&&fields(s)==unchanged,"owns source buffers after input mutation/lifetime");
    unsigned source_profiles=0;
    for(const char* name:{"g-red","sasuke"}){auto actual=read(std::string("local/content/startup-actor-setup/")+name+".gfcs");if(actual.empty())continue;auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");
        check(!model.empty()&&startup_actor_setup_load({actual.data(),actual.size()},hash({model.data(),model.size()}),&p,&error)&&startup_actor_initial_setup(p,input,&s,&error),"actual source constructor inputs load with matching model");
        check(s.rank.requested==16&&s.rank.selected==16&&s.rank.scale_b8==1&&s.rank.field1dd0==1&&s.rank.field5f8==1&&s.scale_b4==1&&s.health736==10000&&s.health738==10000,"real source bounds8..31 selectrank16 with unit triple/health10000");
        for(unsigned encoded=0;encoded<256;++encoded){StartupActorRankState rank;check(startup_actor_rank_select(p,std::bit_cast<s8>(u8(encoded)),&rank)&&rank.selected>=8&&rank.selected<=31,"actual complete signed rank lookup domain");}
        ++source_profiles;
    }
    check(source_profiles==0||source_profiles==2,"partial source bundle cannot be counted as full constructor-data coverage");
    std::printf("{\"startup_actor_setup_checks\":%u,\"source_profiles\":%u,\"source_rank_inputs\":%u,\"numeric_constructor_subset_only\":true,\"actor_construction_placement_or_visible_startup_admitted\":false,\"all_passed\":true}\n",checks,source_profiles,source_profiles*256);
}
