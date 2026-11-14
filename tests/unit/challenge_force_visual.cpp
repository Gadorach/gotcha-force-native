#include <cstdio>
#include <cstdlib>
#include "tests/unit/challenge_force_visual_fixture.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
#include "tests/unit/hsd_record_animation_fixture.hpp"
#include "engine/gameplay/frame_digest.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"force visual: %s\n",why);std::abort();}}}
int main(){
    using namespace gf;const auto a=challenge_force_visual_fixture::art();const auto cards=challenge_force_visual_fixture::cards();check(a.valid()&&cards.valid(),"owned art");
    ForceInventory owner;check(owner.create(force_inventory_fixture::catalog()),"inventory");check(owner.place(0,1,1)==ForceChange::Applied&&owner.place(0,2,2)==ForceChange::Applied,"explicit three-member fixture");ChallengeForceVisual v;check(v.open(a,cards,owner)&&v.draws().empty(),"open without invented callback");
    check(v.step(owner,{true,0,1000})&&v.card_position()[0]==50&&v.focus_position()[0]==50&&v.cost_position()[2]==-50,"first callback approaches once");
    check(v.name_alpha()==1&&v.draws().size()==11,"three members + plate + three focus parts + three digits + cost label");
    check(v.draws()[0].model==0x1001&&v.draws()[0].pose.always_depth&&v.draws()[0].pose.mask==0x25,"source icon path and mask");
    for(unsigned i=0;i<4;++i)check(v.step(owner,{false,-1,1000}),"fade callback");
    check(v.name_alpha()==0&&v.name().empty(),"zero alpha hides focus/name");check(v.step(owner,{false,-1,1000})&&v.focus_position()==a.focus_hidden&&v.cost_position()==a.cost_hidden,"zero-prior callback restores hidden points");
    check(v.step(owner,{true,-1,1000})&&v.draws()[0].pose.record==cards.formats[3].record&&v.draws().size()==7,"Random has no fabricated icons or stored number");
    check(owner.remove(0,0)==ForceChange::Applied&&owner.place(0,29,0)==ForceChange::Applied,"sparse actual slot");
    check(!v.current(owner)&&!v.step(owner,{true,0,1000})&&!v.ready()&&v.draws().empty(),"stale owner refuses old publication");
    check(v.open(a,cards,owner)&&v.step(owner,{true,0,1000}),"explicit fresh owner binding");
    unsigned icons=0;for(const auto& d:v.draws())if(d.model>=0){++icons;check(d.icon_appearance.alpha==1,"prefix beyond member count retained");}check(icons==3,"sparse late slot not dropped");
    ForceInventory foreign;check(foreign.create(force_inventory_fixture::catalog())&&!v.step(foreign,{true,0,1000}),"same data foreign owner refuses");
    for(unsigned count=1;count<=30;++count)for(unsigned slot=0;slot<30;++slot){ChallengeForceCardMember p;check(challenge_force_card_slot(cards,count,slot,30,&p),"all source physical slots");}
    ChallengeForceCardMember p;check(!challenge_force_card_member(cards,1,29,1,&p),"old compact-member guard retained");
    check(!challenge_force_card_slot(cards,1,30,30,&p)&&!challenge_force_card_slot(cards,1,29,31,&p),"physical slot/prefix caps");
    hsd_record_animation_fixture::Fixture fixture;const auto bytes=fixture.archive();HsdRecordAnimationBank bank;std::string error;check(bank.load({bytes.data(),bytes.size()},&error),"owned static record");RecordingRenderBackend backend;RenderEngine render(backend);std::map<u16,AnimatedModelScene> cache;Camera camera;camera.eye[2]=10;camera.near_plane=.1f;camera.far_plane=100;
    auto draw=[&](const StaticRecordAppearance& style){render.begin_frame({});render.reset_arena();check(draw_static_record(render,bank,cache,0,Mat4{},camera,4096,&error,true,true,&style),"styled shared helper");check(render.end_frame(),"styled submit");render.present();return backend.last_frame_digest();};
    const auto ordinary=draw({1,{255,255,255}});check(draw({.25f,{0,0,0}})!=ordinary,"source rejected style changes actual submissions");check(draw({1,{255,255,255}})==ordinary,"style does not stick across shared record reuse");
    std::printf("force visual: %u owned checks PASS\n",checks);
}
