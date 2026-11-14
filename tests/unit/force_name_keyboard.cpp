#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/force_name_scene.hpp"
#include "engine/gameplay/force_editor_session.hpp"
#include "engine/gameplay/force_budget_band.hpp"
#include "tests/unit/force_name_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"name keyboard: %s\n",why);std::abort();}}
const std::string A="\x82\x60",B="\x82\x61";
void edit(ForceNameKeyboard& k){ForceNameEvents e;for(unsigned i=0;i<33;++i)check(k.step(0,0,&e)&&!e.committed,"opening callback");check(k.phase()==ForceNamePhase::Editing&&k.offset()==0,"opening completion");}
struct Save final:ISaveBackend {unsigned writes=0;bool load(u32,std::vector<u8>&)override{return false;}bool store(u32,const u8*,std::size_t)override{++writes;return true;}bool erase(u32)override{return false;}};
}
int main(){
    using namespace gf;auto font=force_name_fixture::font();auto art=force_name_fixture::art();auto b=force_name_fixture::profile(art);ForceNameArt loaded;std::string error;
    check(loaded.load({b.data(),b.size()},font,&error)&&loaded.valid_for(font),"owned profile");
    for(std::size_t n:{std::size_t(0),std::size_t(19),b.size()-1})check(!loaded.load({b.data(),n},font,&error)&&!loaded.valid_for(font),"truncated profile clears partial state");
    auto bad=art;bad.weights[0]=NAN;ForceNameKeyboard k;check(!k.open(bad,font,A),"NaN public profile refuses");bad=art;bad.total=0;check(!k.open(bad,font,A),"zero divisor refuses");
    check(!k.open(art,font,"x")&&!k.open(art,font,std::string("\x82\x62",2)),"partial/unknown encoded name refuses");check(k.open(art,font,A+B)&&k.text()==A+B&&k.cursor()==2,"existing encoded name retained");
    ForceNameEvents e;check(k.step(0x40,0x80,&e)&&!e.committed&&k.text()==A+B&&k.offset()==-1453.125f,"opening consumes inputs, first supplied weights");
    for(unsigned i=1;i<33;++i)check(k.step(0,0,&e),"finish opening");
    check(!force_name_widget_pose(art,k,0,{}).visible,"zero-height title omitted");check(k.step(0,0,&e)&&force_name_widget_pose(art,k,0,{}).visible,"next title sample drawn");
    check(k.step(0x40,0,&e)&&k.cursor()==1&&e.sound==119,"L cursor");check(k.step(2,0x100,&e)&&k.text()==A+A+B&&k.cursor()==2&&k.column()==0,"A inserts at cursor before direction");
    check(k.step(0x20,0x200,&e)&&k.text()==A+B&&k.cursor()==1&&e.sound==123,"B deletes preceding character before R");
    check(k.step(0x60,0,&e)&&k.cursor()==0,"L wins R");check(k.step(0x42,0,&e)&&k.column()==0&&k.cursor()==0,"blocked L still wins direction");
    check(k.step(1,0,&e)&&k.column()==14&&k.step(8,0,&e)&&k.row()==6,"grid wraps left/up");check(k.step(2|4,0,&e)&&k.column()==0&&k.row()==6,"right wins down");check(k.step(4,0,&e)&&k.row()==0,"row wraps down");
    check(k.type_ascii('B',&e)&&k.text()==B+A+B&&k.selected()==0x8261,"host key uses encoded insertion");check(k.type_ascii(255,&e)&&e.sound==0,"unsupported host input ignored");
    check(k.step(0,0x380,&e)&&e.committed&&e.sound==122&&k.text()==B+A+B&&k.phase()==ForceNamePhase::Flash,"Start wins A/B, one commit");
    for(unsigned i=0;i<29;++i){check(k.step(0,0x80,&e)&&!e.committed,"confirmation never recommits");}check(k.phase()==ForceNamePhase::Fade&&k.underline_alpha()==1,"30 flash callbacks");
    for(unsigned i=0;i<16;++i){check(k.step(0,0,&e),"fade callback");}check(k.phase()==ForceNamePhase::Closing&&!k.text_visible(),"16 fade callbacks then retire text");
    for(unsigned i=0;i<33;++i){check(k.step(0,0,&e),"closing callback");}check(!k.visible()&&k.phase()==ForceNamePhase::Done&&!k.step(0,0x80,&e)&&!e.committed,"done cannot replay");
    check(k.open(art,font,""),"empty initial name");edit(k);check(k.step(0,0x180,&e)&&!e.committed&&e.sound==125&&k.text().empty(),"empty Start blocks A too");
    std::string full;for(unsigned i=0;i<20;++i)full+=A;check(k.open(art,font,full),"20 characters");edit(k);
    check(k.step(0x40,0,&e)&&k.cursor()==20&&e.sound==0&&!k.cursor_visible(),"full-name L inhibited");check(k.step(0,0x100,&e)&&k.text()==full&&e.sound==0,"full-name insert ignored");check(k.step(0,0x200,&e)&&k.cursor()==19&&k.text().size()==38&&k.cursor_visible(),"delete from full restores cursor");
    for(int prefix=-2;prefix<=31;++prefix){unsigned count=0;for(unsigned s=0;s<31;++s)if(force_budget_marker_visible(prefix,s)){++count;check(s==0||int(s)==prefix,"start or endpoint only");}check(count==unsigned(prefix<0||prefix>30?0:prefix==0||prefix==30?1:2),"marker pair deduplicates coincidence");}
    // Joined path: commits encoded bytes once, stays in Name through animation,
    // preserves force20/inventory and performs no local save on name acceptance.
    auto catalog=force_inventory_fixture::catalog();ForceInventory inventory;check(inventory.create(catalog)&&inventory.rename_force(19,A+B)==ForceChange::Applied,"session fixture");Save save;ForceInventoryStore store(save);ForceSupplement names;std::array<std::string,6> variants={"N","A","G","S","C","B"};std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);
    check(names.assign(catalog,{{0x1001,2}},"PART ",variants,digits,{"A","B"},&error),"supplement");ForceEditorSession session;check(session.open(inventory,names,store,19)&&session.configure_name_keyboard(art,font),"configured name session");
    auto enter=[&](){check(session.step({},0,0x800),"operations");for(unsigned i=0;i<10;++i)check(session.step({},0,0),"operation delay");check(session.step({},4,0x100)&&session.name_keyboard(),"name request");};enter();
    check(session.name_keyboard()->text()==A+B,"session preserved current name");for(unsigned i=0;i<33;++i)check(session.step({},0,0),"session opening");
    const auto rev=inventory.revision();check(session.step({},0,0x200)&&inventory.revision()==rev,"delete is local until Start");check(session.step({},0,0x80)&&inventory.revision()==rev+1&&session.view()==ForceEditorView::Name,"single encoded commit before close");
    for(unsigned i=0;i<78;++i){check(session.step({},0,0),"session close");}check(session.view()==ForceEditorView::Grid&&inventory.revision()==rev+1&&save.writes==0&&inventory.state()->forces[19].name[2]==0,"joined close preserves one revision and no save");
    enter();check(inventory.rename_force(19,B)==ForceChange::Applied&&!session.step({},0,0x80),"external revision invalidates pending name");
    std::printf("name keyboard/session/markers: %u owned checks PASS\n",checks);
}
