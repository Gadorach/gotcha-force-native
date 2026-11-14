#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/startup_actor_program_class.hpp"
#include "engine/gameplay/startup_actor_dispatch.hpp"
#include "tests/unit/character_motion_driver_fixture.hpp"
#include "tests/unit/startup_actor_class_fixture.hpp"
#include "tests/unit/startup_actor_setup_fixture.hpp"
namespace {
unsigned checks=0;void check(bool v,const char* why){++checks;if(!v){std::fprintf(stderr,"fresh character program: %s\n",why);std::abort();}}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
class CounterClock final:public gf::ICharacterProgramMotion {
public:
    mutable unsigned selects=0,advances=0;bool refuse=false;
    bool select(unsigned,const gf::CharacterProgramDescriptor&,bool,const gf::CharacterProgramMotion&,float start,float,gf::CharacterProgramMotion* out)const override{++selects;if(refuse)return false;*out={true,0,start,4,0,0,{}};return true;}
    bool advance(unsigned,const gf::CharacterProgramMotion& old,float delta,gf::CharacterProgramMotion* out)const override{++advances;if(refuse)return false;*out=old;out->elapsed+=delta;out->frame+=delta;out->finished=out->frame>=4;return true;}
};
}
int main(){
    using namespace gf;std::string error;auto encoded=character_program_fixture::bytes();CharacterProgramAsset asset;check(character_program_load({encoded.data(),encoded.size()},0x12345678,&asset,&error),"owned profile");CounterClock driver;CharacterProgramVm vm;std::vector<CharacterProgramOperation> operations;
    for(unsigned mask=0;mask<4;++mask)for(float delta:{0.f,.1f,1.f,16.f}){
        check(vm.load_fresh_actor(asset,&error)&&vm.ready()&&vm.fresh_actor_mode()&&vm.state().actor.default_group==0&&vm.state().actor.models==std::array<u8,2>{0,0},"fresh pool-cleared actor fields, not imported descriptor fallbacks");
        check(!vm.state().lanes[0].done&&!vm.state().lanes[1].done&&vm.state().lanes[0].metadata[4]==255&&!vm.state().lanes[0].motion.selected,"source initial completion/script/null-motion state");
        check(vm.update(u8(mask),delta,driver,&operations,&error)&&operations.empty()&&vm.state().lanes[0].done&&vm.state().lanes[1].done&&driver.advances==0&&driver.selects==0,"primarynull gate ignores per-lane mask andmarks both admitted lanes, never invokes driver");
    }
    CharacterProgramActor ordinary;check(!vm.load(asset,ordinary,{},&error)&&!vm.fresh_actor_mode(),"ordinaryload stillrejects default0, andclearsfreshmode evenonfailure");ordinary.default_group=5;
    check(vm.load(asset,ordinary,{},&error)&&!vm.fresh_actor_mode()&&vm.request(5,0,2,-1,1,driver,&operations,&error)&&vm.update(2,1,driver,&operations,&error)&&driver.advances>0,"ordinaryexplicitinput per-lane behavior unchanged, notsilently changed toactor-global gate");
    driver.selects=driver.advances=0;check(vm.load_fresh_actor(asset,&error)&&vm.request(5,0,2,-1,1,driver,&operations,&error),"partialsecondary selection beforeprimary");const auto selected=vm.state().lanes[1].motion.frame;
    check(vm.update(2,1,driver,&operations,&error)&&operations.empty()&&vm.state().lanes[1].motion.frame==selected&&vm.state().lanes[1].done&&vm.state().lanes[0].done&&driver.advances==0,"nullprimary overridesselectedsecondary andpreserves itsclock/metadata, notper-lane skip");
    check(vm.load_fresh_actor(asset,&error)&&!vm.request(0,0,3,-1,1,driver,&operations,&error)&&!vm.ready(),"freshgroup0 fallback isunimported andrefuses, no inventeddefault5");
    check(vm.request(5,0,3,-1,1,driver,&operations,&error)&&vm.state().actor.default_group==0&&vm.state().actor.models==asset.default_models,"explicitgroup5 recoverswithsourceprefix; fallbacks applyonlyafterdescriptor select");
    check(vm.load_fresh_actor(asset,&error)&&vm.request(5,0,1,-1,1,driver,&operations,&error),"primary-only request input");const auto oldframe=vm.state().lanes[0].motion.frame;
    check(!vm.update(3,1,driver,&operations,&error)&&!vm.ready()&&operations.empty()&&vm.state().lanes[0].motion.frame==oldframe,"nonnullprimary plus unfinishedunassignedsecondary refuses atomically, notnewnull-lane shortcut");
    auto masked=asset;masked.lane_mask=1;check(!vm.load_fresh_actor(masked,&error),"freshentry limited toreviewedactive lanes0/1");
    check(vm.load_fresh_actor(asset,&error)&&!vm.update(3,NAN,driver,&operations,&error)&&!vm.ready(),"unusednull delta stillobeys boundednative domain");
    CharacterProgramVm a,b;check(a.load_fresh_actor(asset,&error)&&b.load_fresh_actor(asset,&error),"separate fresh scopes");CharacterProgramClassFields observed;check(a.observe_class_fields(&observed)&&!b.write_class_fields(observed,0,0,&error)&&!b.ready(),"foreignsame-asset class observation rejects");
    auto kept=a;const auto keep_scope=observed.scope.lock();check(a.load_fresh_actor(asset,&error)&&keep_scope&&!a.write_class_fields(observed,0,0,&error),"freshreload rejectsold observation evenoldscope keptalive");
    auto setup_bytes=startup_actor_setup_fixture::profile();store_be32(setup_bytes.data()+8,asset.model_hash);StartupActorSetupProfile setup;check(startup_actor_setup_load({setup_bytes.data(),setup_bytes.size()},asset.model_hash,&setup,&error),"owned matching setup");
    auto class_bytes=startup_actor_class_fixture::profile();StartupActorClassProfile classes;check(startup_actor_class_load({class_bytes.data(),class_bytes.size()},&classes,&error),"owned classprofile");
    auto controlled=asset;controlled.programs[0].bytes[9]=254;controlled.programs[0].bytes[13]=1; //opcode2 togglegroup +6E9=1
    check(character_program_detail::valid(controlled)&&vm.load_fresh_actor(controlled,&error),"ownedsource-shaped control program");StartupActorClassState input,klass;input.model_id=0x615;input.angle18e2=-11;input.field6ee=23;input.field1dfc=-7;std::vector<StartupActorClassEffect> effects;
    check(startup_actor_class_initialize(classes,input,&klass,&effects),"GRed classinit");StartupActorProgramClass bridge;check(bridge.bind(vm,setup,classes,klass,&error)&&bridge.pre(vm,&error)&&bridge.state().angle18e2==-9,"classpre applies signedangle damping andnative fields");
    check(vm.observe_class_fields(&observed)&&vm.request(5,0,3,-1,1,driver,&operations,&error)&&vm.state().actor.default_group==1&&vm.state().actor.control0[3]==1,"interveningmotion prefix updatesdefault/control");
    auto stale=vm;check(!stale.write_class_fields(observed,0,0,&error)&&!stale.ready()&&vm.ready(),"stale expectedfields rejectonstagedcopy, originalVM remainsunchanged");
    check(bridge.post(vm,&effects,&error)&&effects.size()==1&&effects[0].kind==StartupActorClassEffectKind::Effect1AB6CRequired&&effects[0].id==50&&vm.state().actor.control0[3]==0&&vm.state().actor.default_group==1&&vm.state().actor.control0[0]==2,"post observesNEWfields, clears6E9only, preservesgroup/primarycontrol andemits32once");
    check(!bridge.post(vm,&effects,&error)&&effects.empty()&&!bridge.ready(),"duplicatepost cannot replayeffect requirement");
    setup.model_id=10;input={};input.model_id=10;check(startup_actor_class_initialize(classes,input,&klass,&effects)&&vm.load_fresh_actor(controlled,&error)&&bridge.bind(vm,setup,classes,klass,&error)&&vm.request(5,0,3,-1,1,driver,&operations,&error),"Sasuke source-group test");
    for(unsigned tick=0;tick<5;++tick){check(bridge.pre(vm,&error)&&vm.state().actor.default_group==0&&bridge.post(vm,&effects,&error)&&effects.empty()&&vm.state().actor.control0[3]==1&&bridge.state().field6ec==((tick+1)&3),"Sasukepre resets5820; postretains6E9 andadvancescounter eachcallback");}
    check(bridge.pre(vm,&error)&&vm.request(5,0,3,-1,1,driver,&operations,&error)&&vm.state().actor.default_group==1&&bridge.post(vm,&effects,&error)&&vm.state().actor.default_group==1&&bridge.state().field6ec==2,"Sasukepost mustnot repeatpre's groupreset overanintervening opcodewrite");
    check(bridge.pre(vm,&error)&&vm.state().actor.default_group==0&&bridge.post(vm,&effects,&error)&&bridge.state().field6ec==3,"nextSasukepre resetsnewgroup onlyonnextcallback stage");
    check(vm.load(asset,ordinary,{},&error)&&!bridge.pre(vm,&error),"ordinaryload invalidatesprevious classbinding andopt-inpath");
    setup.model_id=0x615;input={};input.model_id=0x615;check(startup_actor_class_initialize(classes,input,&klass,&effects)&&vm.load_fresh_actor(asset,&error),"restore fresh case");auto foreign_setup=setup;foreign_setup.model_hash^=1;check(!bridge.bind(vm,foreign_setup,classes,klass,&error),"cross-model profile rejects beforeclass mutation");
    check(bridge.bind(vm,setup,classes,klass,&error)&&!bridge.post(vm,&effects,&error),"post withoutpre refuses");
    unsigned profiles=0,requests=0,updates=0,holds=0;auto actual_classes=read("local/content/startup-actor-class/startup.gfac");
    for(const char* name:{"g-red","sasuke"}){auto model=read(std::string("local/content/startup-characters/")+name+"/model.hsd");if(model.empty())continue;auto prefix=read(std::string("local/content/startup-characters/")+name+"/model-prefix.bin"),pose=read(std::string("local/content/character-pose-bindings/")+name+".gfcp"),program=read(std::string("local/content/character-program-bindings/")+name+".gfmp"),encoded_setup=read(std::string("local/content/startup-actor-setup/")+name+".gfcs");
        HsdModelBones bones;CharacterPoseRig rig;CharacterProgramAsset source;StartupActorSetupProfile source_setup;const auto hash=character_program_model_hash({model.data(),model.size()});check(hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bones,&error)&&character_pose_rig_load({pose.data(),pose.size()},bones,&rig,&error)&&character_program_load({program.data(),program.size()},hash,&source,&error)&&startup_actor_setup_load({encoded_setup.data(),encoded_setup.size()},hash,&source_setup,&error)&&startup_actor_class_load({actual_classes.data(),actual_classes.size()},&classes,&error),"source model/rig/program/setup/class bindings");
        std::vector<CharacterMotionClipBinding> clips;for(unsigned index:{0,1,3,4,7,8,9}){auto raw=read(std::string("local/content/startup-motion-programs/")+name+"/motions/motion-0"+std::to_string(index)+".hsd");CharacterMotionClipBinding clip;clip.index=u8(index);check(hsd_bone_motion_load({raw.data(),raw.size()},rig.binding,&clip.clip,&error),"source requested clip");clips.push_back(std::move(clip));}
        CharacterMotionDriver concrete;check(concrete.load({model.data(),model.size()},rig,source,clips,&error),"source immutablemotionlibrary");
        for(unsigned choice:{0,1,3,4,6,7}){StartupActorDispatch dispatch;dispatch.bind(u8(source_setup.model_id==0x615?0:1));input={};input.model_id=source_setup.model_id;check(vm.load_fresh_actor(source,&error)&&startup_actor_class_initialize(classes,input,&klass,&effects)&&bridge.bind(vm,source_setup,classes,klass,&error),"actualfresh fields/class bind");std::vector<StartupActorEffect> requirements;
            for(unsigned null_step=0;null_step<2;++null_step){check(bridge.pre(vm,&error)&&dispatch.update(vm,1,concrete,&operations,&requirements,&error)&&bridge.post(vm,&effects,&error)&&operations.empty()&&!vm.state().lanes[0].motion.selected,"sourceStartthen globalnull update beforefirstmotionrequest");}
            check(vm.state().lanes[0].done&&vm.state().lanes[1].done&&vm.state().actor.default_group==0,"fresh null path marksdone withoutprofilegroup/model defaultsubstitution");
            check(dispatch.request(vm,u8(choice),-1,1,concrete,&operations,&error),"actualexplicitstartup requestgroup5");++requests;
            for(unsigned tick=0;tick<384;++tick){check(bridge.pre(vm,&error)&&dispatch.update(vm,1,concrete,&operations,&requirements,&error)&&bridge.post(vm,&effects,&error)&&effects.empty(),"classpre / retaineddriver-subdispatch / classpost sourcefieldcoupling");++updates;
                if(dispatch.state().phase==StartupActorDispatchState::ScaleOut||(vm.state().lanes[0].done&&vm.state().lanes[1].done))break;
                if(source_setup.model_id==0x615&&choice==7&&tick==127){check(vm.state().lanes[0].motion.frame==10&&vm.state().lanes[0].threshold==24&&!vm.state().lanes[0].done,"GRed7 sourcehold unchanged, notforced throughfreshentry");++holds;break;}
                check(tick<383,"bounded isolated motion scope");
            }
        }
        ++profiles;
    }
    check(profiles==0||profiles==2,"partialsource inputs are notcounted asfull actorcoverage");
    std::printf("{\"fresh_character_program_checks\":%u,\"source_profiles\":%u,\"source_motion_requests\":%u,\"source_coupled_updates\":%u,\"preserved_G_Red_holds\":%u,\"old_explicit_entry_preserved\":true,\"full_actor_constructor_or_visible_startup\":false,\"all_passed\":true}\n",checks,profiles,requests,updates,holds);
}
