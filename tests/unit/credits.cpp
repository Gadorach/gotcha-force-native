#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/credits_scene.hpp"
#include "engine/gameplay/option_menu.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"credits: %s\n",why);std::abort();}}
gf::CreditsConfig fixture(){
    gf::CreditsConfig c;c.model_count=3;c.max_active=10;c.max_updates=100;c.entry_wait=2;c.final_hold=3;
    c.speed=2;c.spawn_y=0;c.delete_y=8;c.hold_y=3;c.scale=1;c.x_step=1;c.z_step=1;c.grid_factor=0.5f;
    c.rows={{1,2,0,2,0},{5,2,2,1,0},{9,0,3,0,0}};c.glyphs={{1,0},{2,2},{2,0}};return c;
}
std::vector<gf::u8> encode(const gf::CreditsConfig& c,const std::vector<gf::u8>& archive){
    std::vector<gf::u8> b(128+c.model_count*4+c.rows.size()*20+c.glyphs.size()*8);auto word=[&](unsigned at,gf::u32 v){gf::store_be32(b.data()+at,v);};
    unsigned at=0;for(gf::u32 v:{0x47464352u,1u,gf::menu_asset_hash({archive.data(),archive.size()}),c.model_count,gf::u32(c.rows.size()),gf::u32(c.glyphs.size()),c.max_active,c.max_updates,c.entry_wait,c.final_hold}){word(at,v);at+=4;}
    for(float v:{c.speed,c.spawn_y,c.delete_y,c.hold_y,c.x_base,c.z_base,c.x_step,c.z_step,c.grid_factor,c.image_x,c.image_z,c.rotation_y,c.scale}){word(at,gf::f32_to_bits(v));at+=4;}
    at=128;for(unsigned i=0;i<c.model_count;++i){word(at,0x100+i*0x40);at+=4;}
    for(const auto& row:c.rows){for(gf::u32 v:{row.kind,row.delay,row.first,row.count,gf::f32_to_bits(row.spawn_y)}){word(at,v);at+=4;}}
    for(const auto& glyph:c.glyphs){word(at,glyph.model);word(at+4,glyph.column);at+=8;}return b;
}
struct Save:gf::ISaveBackend {
    unsigned writes=0;bool load(gf::u32,std::vector<gf::u8>&)override{return false;}
    gf::SaveReadStatus read_slot(gf::u32,std::vector<gf::u8>&)override{return gf::SaveReadStatus::Missing;}
    bool store(gf::u32,const gf::u8*,std::size_t)override{++writes;return true;}bool erase(gf::u32)override{return false;}
};
std::vector<gf::u8> read(const char* p){std::ifstream f(p,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    auto c=fixture();gf::u32 peak=0,total=0;check(gf::credits_preflight(c,&peak,&total)&&peak==4&&total==8,"owned timeline completes with bounded source actor ordering");
    gf::CreditsTimeline t;check(t.begin(c)&&t.active()==1,"background counts as one actor");
    check(t.step()&&t.rows()[0].y==2&&t.active()==3,"first producer dispatch followed by actor advance");
    check(t.step()&&t.rows()[1].y==2&&t.active()==4,"next record and final image spawn");
    check(t.step()&&t.ended()&&t.rows()[1].y==4&&!t.rows()[1].holding,"producer ends but actors continue; image permits source overshoot");
    check(t.step()&&t.active()==2&&t.rows()[1].holding&&t.rows()[1].hold_left==3,"final image waits until other actors clear, then starts hold");
    t.step();t.step();check(t.running()&&t.rows()[1].hold_left==1,"hold doesn't expire early");
    t.step();check(t.active()==1&&t.running(),"source completion observed by producer on following update");
    check(t.step()&&!t.running()&&t.ended()&&!t.failed()&&t.updates()==8,"automatic completion clears background");
    check(!t.step(),"completed run doesn't replay");check(t.begin(c)&&t.step(),"reentry resets old state");t.cancel();check(!t.running()&&!t.active()&&!t.rows()[0].alive&&!t.step(),"B cancellation clears active models");
    {gf::CreditsTimeline owned;{auto temporary=fixture();check(owned.begin(temporary),"timeline accepts owned copy");}
     check(owned.step()&&owned.active()==3,"timeline doesn't borrow a destroyed configuration");}
    {auto bad=c;bad.max_active=3;check(!gf::credits_preflight(bad),"active-actor budget refuses");}
    {auto bad=c;bad.max_updates=7;check(!gf::credits_preflight(bad),"completion work budget refuses");}
    {auto bad=c;bad.rows[0].delay=20;bad.delete_y=2;bad.hold_y=1;check(!gf::credits_preflight(bad),"early source completion before END refuses instead of inventing a wait");}
    for(unsigned which=0;which<8;++which){auto bad=c;
        if(which==0)bad.speed=NAN;
        if(which==1)bad.speed=0;
        if(which==2)bad.rows[0].first=1;
        if(which==3)bad.rows.back().kind=4;
        if(which==4)bad.rows[0].count=100;
        if(which==5)bad.glyphs[0].model=0;
        if(which==6)bad.glyphs[0].column=50;
        if(which==7)bad.entry_wait=601;
        check(!gf::credits_config_valid(bad),"invalid semantics refuse");}
    std::vector<gf::u8> archive={4,5,6};gf::HsdScene source;
    for(unsigned i=0;i<3;++i){gf::HsdJoint j;j.archive_offset=0x100+i*0x40;source.joints.push_back(j);source.meshes.push_back({});source.mesh_joint.push_back(i);}
    auto bytes=encode(c,archive);gf::CreditsProfile p;std::string error;
    auto load=[&](const auto& b){return gf::credits_profile_load({b.data(),b.size()},{archive.data(),archive.size()},source,&p,&error);};
    check(load(bytes)&&p.config.glyphs.size()==3,"owned native profile decodes and preflights");
    for(unsigned at:{0u,4u,8u,12u,16u,20u,24u,28u,32u,36u,40u,92u,128u}){auto bad=bytes;bad[at]=255;check(!load(bad)&&p.roots.empty(),"malformed profile has no partial publication");}
    {auto bad=bytes;bad.pop_back();check(!load(bad),"truncated glyph table refuses");}
    Save backend;gf::OptionSettingsStore store(backend);gf::OptionMenu menu;menu.configure(35,30);
    check(menu.configure_credits(2)&&!menu.configure_credits(601),"credits entry delay config bounds");
    std::array<gf::GamePadState,4> pads{};menu.begin(store,pads);
    auto tap=[&](gf::GamePadState pad){pads={};pads[3]=pad;menu.update(store,pads);pads={};menu.update(store,pads);};
    gf::GamePadState up,a,b;up.dpad_up=true;a.a=true;b.b=true;
    tap(up);tap(up);check(menu.row()==8,"ordinary wrapped navigation selects STAFF CREDITS");
    pads[3]=a;menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::CreditsDelay&&menu.credits_delay()==2,"A starts source delay without immediate script start");
    pads[3]=b;menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::CreditsDelay,"B during delay consumed");
    menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::Credits,"delay reaches credits");menu.update(store,pads);
    check(menu.phase()==gf::OptionMenuPhase::Credits&&backend.writes==0,"held B across delay isn't replayed as return or save");
    pads={};menu.update(store,pads);pads[3]=a;menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::Credits,"A doesn't cancel actual credits");
    pads[3]=b;menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::Editing&&menu.row()==8&&backend.writes==0,"fresh any-port B returns to same row without save");
    menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::Editing&&backend.writes==0,"held return B doesn't immediately save/exit");
    pads={};menu.update(store,pads);pads[3]=a;menu.update(store,pads);menu.update(store,pads);menu.update(store,pads);
    check(menu.finish_credits()&&menu.row()==8&&menu.phase()==gf::OptionMenuPhase::Editing,"automatic finish returns same selection");
    menu.update(store,pads);check(menu.phase()==gf::OptionMenuPhase::Editing&&backend.writes==0,"held A at automatic finish cannot reopen credits");
    bool real=false;const auto asset=read("local/analysis/arc/staff00_mdl.arc"),content=read("local/content/credits.gfcr");
    if(!asset.empty()&&!content.empty()){
        gf::HsdScene decoded;check(gf::hsd_load({asset.data(),asset.size()},&decoded,&error)&&!decoded.meshes[0].colour_write&&!decoded.meshes[0].alpha_write&&decoded.meshes[0].depth_write,"original PE38 mask is depth-only, not white artwork");
        gf::CreditsScene scene;check(scene.load({asset.data(),asset.size()},{content.data(),content.size()},&error)&&scene.begin(),"real credits geometry/style/script load");
        const auto& config=scene.config();check(config.model_count==80&&config.rows.size()==131&&config.glyphs.size()==1430,"whole private script and model inventory bound");
        check(gf::credits_preflight(config,&peak,&total)&&peak==180&&total==4267,"entire real sequence reaches source final-image completion within budgets");
        check(config.rows[0].kind==2&&config.rows[0].count==8&&config.glyphs[0].column==17,"first role centered using source byte-column operation");
        for(unsigned i=0;i<250;++i)check(scene.step(),"real scene scroll pose update");
        check(scene.pose().joints.size()==1431&&scene.pose().joints[1].matrix[7]==640&&!scene.pose().joints[1].hidden,"native pose follows authored speed/spawn position");
        check(scene.timeline().active()==109&&scene.timeline().emitted()==10,"native row-weighted timeline agrees with independent individual-actor source replay");
        check(scene.pose().materials[1].diffuse[0]==229/255.0f&&scene.pose().materials[1].diffuse[2]==25/255.0f,"heading uses original yellow material style, not a replacement font");
        gf::CreditsTimeline full;bool ok=full.begin(config);while(ok&&full.running())ok=full.step();
        check(ok&&!full.failed()&&full.rows()[config.rows.size()-2].y==426&&full.rows()[config.rows.size()-2].holding,"final image keeps source overshoot and completes its hold");
        scene.cancel();check(!scene.running()&&scene.begin()&&scene.timeline().emitted()==0,"reentry starts fresh, no old script queue");real=true;
    }
    std::printf("{\"credits_checks\":%u,\"real_present\":%s,\"all_passed\":true}\n",checks,real?"true":"false");
}
