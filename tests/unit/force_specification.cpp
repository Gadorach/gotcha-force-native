#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/assets/force_specification.hpp"
#include "engine/gameplay/force_spec_navigation.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force specification: %s\n",why);std::abort();}}
std::vector<u8> fixture(const ForceCatalog& catalog){
    std::vector<u8> b(26+138*catalog.entries().size());store_be32(b.data(),0x47465344);store_be32(b.data()+4,1);store_be32(b.data()+8,catalog.fingerprint());store_be16(b.data()+12,u16(catalog.entries().size()));store_be16(b.data()+14,138);
    const std::array<u8,10> selectors={0,0,0,1,1,1,2,3,0,3};std::copy(selectors.begin(),selectors.end(),b.begin()+16);
    for(unsigned i=0;i<catalog.entries().size();++i){const auto at=26+i*138;store_be16(b.data()+at,catalog.entries()[i].model);store_be16(b.data()+at+2,catalog.entries()[i].number);std::array<u8,13> fields={3,2,u8(i==1),255,1,2,3,4,1,255,26,255,7};if(i==2)std::fill(fields.begin()+8,fields.end(),255);std::copy(fields.begin(),fields.end(),b.begin()+at+5);for(unsigned v=0;v<6;++v)for(unsigned l=0;l<10;++l)store_be16(b.data()+at+18+(v*10+l)*2,u16(100+i*50+v*10+l));}return b;
}
std::vector<u8> read(const char* p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(int argc,char** argv){
    using namespace gf;auto catalog=force_inventory_fixture::catalog();auto bytes=fixture(catalog);ForceSpecificationCatalog spec;std::string error;auto view=[](const auto& b){return ByteView{b.data(),b.size()};};
    check(spec.load(view(bytes),catalog,&error)&&spec.for_catalog(catalog),"owned metadata binding");ForceSpecAuthoredView result;
    check(spec.authored_view(catalog,0x1001,5,9,ForceSpecMoveAccess::Available,&result)&&result.hp==159&&result.cost==150&&result.rarity==5&&result.level==10,"variant/level HP and shared catalog cost");
    check(result.move_count==3&&result.moves[0].source_slot==0&&result.moves[1].source_slot==2&&result.moves[2].source_slot==4&&result.moves[1].text_index==2&&result.moves[2].icon==7,"compacted moves retain original text slots");
    check(result.feature==-1&&result.jump_count==2&&result.ratings==std::array<u8,4>{1,2,3,4},"signed hidden feature and displayed ratings");
    check(spec.authored_view(catalog,0x1002,0,0,ForceSpecMoveAccess::Available,&result)&&result.jump_count==6&&result.moves[2].text_index==9,"override jump count and numbered text bank");
    check(spec.authored_view(catalog,0x1003,0,0,ForceSpecMoveAccess::Available,&result)&&result.page_mask==3&&result.move_count==0,"no authored moves hides page");
    check(spec.authored_view(catalog,0x1001,0,0,ForceSpecMoveAccess::Unavailable,&result)&&result.page_mask==3&&result.move_count==3,"runtime unavailable hides page without deleting authored rows");
    check(!spec.authored_view(catalog,0x1001,0,0,ForceSpecMoveAccess::Unknown,&result)&&result.move_count==0,"unknown runtime availability refuses/clears");
    check(!spec.authored_view(catalog,0x1001,6,0,ForceSpecMoveAccess::Available,&result)&&!spec.authored_view(catalog,0x1001,0,10,ForceSpecMoveAccess::Available,&result)&&!spec.authored_view(catalog,0xffff,0,0,ForceSpecMoveAccess::Available,&result),"normal variant/level/model bounds");
    for(std::size_t size:{std::size_t(0),std::size_t(25),bytes.size()-1}){ForceSpecificationCatalog bad;check(!bad.load({bytes.data(),size},catalog,&error),"truncated profile");}
    for(auto [at,value]:std::array<std::pair<unsigned,u8>,8>{{{8,u8(bytes[8]^1)},{30,1},{31,5},{32,7},{33,2},{34,5},{35,11},{39,27}}}){auto b=bytes;b[at]=value;ForceSpecificationCatalog bad;check(!bad.load(view(b),catalog,&error)&&!bad.for_catalog(catalog),"malformed identity/selector refuses");}
    auto zero=bytes;zero[44]=zero[45]=0;check(!spec.load(view(zero),catalog,&error)&&!spec.find(0x1001),"invalid HP retires old loaded view");
    ForceSpecNavigation nav;ForceSpecNavigationEvent e;check(!nav.open(0,ForceSpecPage::Statistics)&&!nav.open(16,ForceSpecPage::Description)&&!nav.open(1,ForceSpecPage::Moves),"invalid page set");check(nav.open(7,ForceSpecPage::Statistics),"Borg starts statistics");
    check(nav.step(2,&e)&&nav.page()==ForceSpecPage::Moves&&e.sound==119&&e.changed,"right to moves");check(nav.step(2,&e)&&nav.page()==ForceSpecPage::Description,"wrap to description");check(nav.step(1,&e)&&nav.page()==ForceSpecPage::Moves,"left wrap");check(nav.step(3,&e)&&nav.page()==ForceSpecPage::Description,"right wins simultaneous left");
    check(nav.step(0x203,&e)&&e.close&&e.sound==123&&!e.changed&&nav.page()==ForceSpecPage::Description&&!nav.step(2,&e)&&!e.close,"B wins and close cannot replay");
    check(nav.open(3,ForceSpecPage::Statistics)&&nav.step(2,&e)&&nav.page()==ForceSpecPage::Description,"unavailable moves skipped");check(nav.open(8,ForceSpecPage::Component)&&nav.step(3,&e)&&!e.changed&&e.sound==0,"component only page no spurious navigation");
    check(nav.open(1,ForceSpecPage::Description),"single page");for(unsigned i=1;i<=71;++i)check(nav.step(0,&e)&&nav.blink()==i%71,"source71 callback blink");
    auto angles=force_spec_rotate({0xffffff80u,0x20},0x800000u|0x200000u);check(angles.pitch==0x100&&angles.yaw==0xfffffea0u,"defined32bit rotation wrap");angles=force_spec_rotate(angles,0xf00000u);check(angles.pitch==0x100&&angles.yaw==0xfffffea0u,"opposing held rotations cancel");
    if(argc==4){auto cb=read(argv[1]),sb=read(argv[2]);ForceCatalog actual;ForceSpecificationCatalog data;check(actual.load(view(cb),&error)&&data.load(view(sb),actual,&error),"original catalog/profile load");std::ofstream journal(argv[3]);
        for(const auto& m:actual.entries())for(unsigned v=0;v<6;++v)for(unsigned l=0;l<10;++l){ForceSpecAuthoredView x;check(data.authored_view(actual,m.model,v,l,ForceSpecMoveAccess::Available,&x),"source authored view with declared available access");journal<<x.model<<' '<<v<<' '<<l<<' '<<x.hp<<' '<<x.cost<<' '<<x.rarity<<' '<<unsigned(x.tribe)<<' '<<x.number<<' '<<unsigned(x.jump_type)<<' '<<unsigned(x.jump_count)<<' '<<int(x.feature);for(auto n:x.ratings)journal<<' '<<unsigned(n);journal<<' '<<unsigned(x.move_count)<<' '<<unsigned(x.page_mask);for(const auto& move:x.moves)journal<<' '<<unsigned(move.source_slot)<<' '<<unsigned(move.button)<<' '<<unsigned(move.activation)<<' '<<unsigned(move.icon)<<' '<<move.text_index;journal<<'\n';}
        for(u8 mask=1;mask<16;++mask)for(unsigned page=0;page<4;++page)if(mask&(1u<<page))for(u32 pressed:{0u,1u,2u,3u,0x200u,0x203u,0x10000u,0x20000u}){ForceSpecNavigation n;check(n.open(mask,ForceSpecPage(page))&&n.step(pressed,&e),"declared page table");journal<<"nav "<<unsigned(mask)<<' '<<page<<' '<<pressed<<' '<<unsigned(n.page())<<' '<<unsigned(n.blink())<<' '<<e.close<<' '<<e.changed<<' '<<e.sound<<'\n';}
    }else check(argc==1,"arguments");
    std::printf("force specification data/navigation: %u checks PASS\n",checks);
}
