#include <cstdio>
#include <cstdlib>
#include "engine/core/headless_backends.hpp"
#include "platform/pc/account_save_storage.hpp"
#include "platform/pc/portable_battle_host.hpp"
#include "platform/pc/portable_challenge_launch.hpp"

namespace {
using namespace gf;
using namespace gf::pc;
using Status=PortableBattleStatus;

void check(bool condition,const char* message){
    if(!condition){std::fprintf(stderr,"portable duel: %s\n",message);std::abort();}
}

void saved_force_cases(){
    MixedBorgForceView force;
    const PackageForceSlotView exact{7,{5,{"example:flight-stick","Flight Stick"},false,SavedPackageVariant{"clear","Clear"}},
        PackageForceSlotStatus::MetadataReady,80};
    PortableBattleForceSnapshot out;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied,"empty Force");
    force.slots.push_back({7,{},exact});
    check(portable_single_saved_force(force,12,{},&out).status==Status::Ready&&out.revision==12&&
        out.members.size()==1&&out.members[0].package_id=="example:flight-stick"&&out.members[0].variant_id=="clear",
        "pin exact saved variant");
    const auto prior=out;
    check(portable_single_saved_force(force,12,{70},&out).status==Status::Denied&&out==prior,"mode budget refusal");
    force.slots.push_back({8,RetailForceSlotView{},{}});
    check(portable_single_saved_force(force,12,{},&out).status==Status::Unsupported&&out==prior,"mixed Force not truncated");
    force.slots.erase(force.slots.begin());
    check(portable_single_saved_force(force,12,{},&out).status==Status::Unsupported&&out==prior,"retail not substituted");
    force.slots.front().packaged=exact;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied&&out==prior,"overlap refused");
    force.slots.front().retail.reset();
    force.slots.front().packaged->saved.disabled=true;
    check(portable_single_saved_force(force,12,{},&out).status==Status::Denied&&out==prior,"disabled copy refused");
    force.slots.front().packaged->saved.disabled=false;
    force.slots.front().packaged->saved.variant.reset();
    check(portable_single_saved_force(force,12,{},&out).status==Status::Missing&&out==prior,"no default variant fallback");
}

std::shared_ptr<const BorgGameplayProfile> definition(std::string_view movement,std::string_view body){
    BorgGameplayProfile profile;
    std::string error;
    const auto valid=borg_read_gameplay_profile("GFBORG-GAMEPLAY 1\nvariant=normal\nmovement="+std::string(movement)+
        "\nrig-prefix=body.prefix\npose-profile=body.pose\nidle=0:0\n"+std::string(body),&profile,&error);
    check(valid,error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(profile));
}

void duel_cases(){
    const auto unarmed=definition("stationary-v1","health=1\n");
    const auto armed=definition("grounded-v1|8|13|14","health=15\nwalk=0:1\nair=0:2\ndash=0:3\n"
        "projectile=shot|beam|1|60|0|0|0|8|30|19|ignore-floor\n"
        "move=fire|b|10|any|4|0:4\nrelease=fire|2|none|0|shot\n");
    BattleMvpConfig config;
    config.spawn={BattlePoint{0,0},BattlePoint{0,220}};
    config.packages={unarmed,armed};
    NativeBattleMvp world;
    using End=PortableDuelEnd;
    check(portable_duel_end(world,1)==End::Fighting,"unstarted world has no result");
    check(world.start(config),"start exact authored duel");
    check(portable_duel_end(world,1)==End::Fighting,"tick zero has no result");
    auto step_enemy=[&](){
        const auto enemy=portable_duel_enemy_input(world);
        check(enemy.attack.valid(),"AI command respects action input shape");
        check(world.step({GamePadState{},enemy.movement},false,nullptr,nullptr,nullptr,nullptr,{{{},enemy.attack}}),
            "authored duel advances through shared input path");
    };
    const auto first=portable_duel_enemy_input(world);
    check(first.attack.pressed==BorgBattleInputs::b&&first.attack.held==BorgBattleInputs::b,"AI presses primary on tick zero");
    step_enemy();
    check(portable_duel_end(world,1)==End::Timeout&&!portable_duel_campaign_outcome(End::Timeout),"expiry is not a win or loss");
    while(world.tick()<40&&portable_duel_end(world,0)==End::Fighting)step_enemy();
    check(world.hits()==1&&portable_duel_end(world,world.tick())==End::Loss&&
        portable_duel_campaign_outcome(End::Loss)==PortableBattleOutcome::Loss,"AI's actual KO has precedence over same-tick expiry");
    config.packages={armed,unarmed};
    check(world.start(config),"start flipped authored duel");
    while(world.tick()<40&&portable_duel_end(world,0)==End::Fighting){
        BorgBattleInputs player;
        if(!world.tick())player.pressed=player.held=BorgBattleInputs::b;
        check(world.step({},false,nullptr,nullptr,nullptr,nullptr,{player,{}}),"player steps");
    }
    check(world.hits()==1&&portable_duel_end(world,world.tick())==End::Win&&
        portable_duel_campaign_outcome(End::Win)==PortableBattleOutcome::Win,"only player KO produces win");
    check(!portable_duel_campaign_outcome(End::Fighting)&&!portable_duel_campaign_outcome(End::Draw)&&
        !portable_duel_campaign_outcome(End::Cancelled),"non-decisions do not produce a campaign result");
}

// Smoke-test fixture writer, never invoked by the game. Require an explicit
// owned empty root so this tool cannot overwrite a user's saved Force.
int seed(const std::filesystem::path& root){
    check(std::filesystem::is_directory(root)&&std::filesystem::is_empty(root),"fixture root must be owned and empty");
    std::string error;
    auto disk=create_account_save_storage(root.string(),false,&error);
    check(bool(disk),error.c_str());
    AccountSaveRepository repo(*disk);
    AccountSaveEntry account;
    check(repo.create_account(1,"Challenge fixture",&account)==AccountResult::Ok,"create isolated fixture account");
    for(u64 i=1;i<=4;++i){
        AccountSaveEntry entry;
        check(repo.create_save(account,i,"Force "+std::to_string(i),&entry)==AccountResult::Ok,"create isolated fixture save");
        std::unique_ptr<AccountSaveBackend> backend;
        check(repo.open(entry,&backend)==AccountResult::Ok,"select fixture save");
        PackageBorgSaveStore store(*backend);
        PackageBorgRoster roster;
        check(store.load(&roster)==PackageBorgLoad::Missing,"new fixture save has no previous package Force");
        roster.owned.push_back({11,{"example:flight-stick","Flight Stick"},i==4,
            i==3?std::optional<SavedPackageVariant>{}:SavedPackageVariant{"normal","Normal"}});
        roster.placements.push_back({11,0,7});
        if(i==2){
            roster.owned.push_back({12,{"example:jump-stick","Jump Stick"},false,SavedPackageVariant{"normal","Normal"}});
            roster.placements.push_back({12,0,19});
        }
        check(store.write(roster),"seed only the disposable package Force");
        repo.close();
    }
    return 0;
}

// Exercise the real host adapter with generated content, independently of
// the CLI. Production reuses the existing package battle loop.
int run_adapter(const std::string& save_root){
    const auto borgs=scan_borg_packages("borgs");
    PortableChallengeLaunch launch;
    const auto loaded=load_portable_challenge_launch({"example:trials","round-one",save_root,{1,1},{0}},borgs,{},&launch);
    check(bool(loaded),loaded.reason.c_str());
    headless::Bundle backends;
    const auto io=backends.get();
    RenderEngine render(*io.render);
    std::string error;
    check(render.initialize({640,480,false,true,"Challenge adapter test"},io.window,&error),error.c_str());
    NativeBattleMvpView view;
    PortableBattleHost adapter(borgs,*launch.saved_force,view,render);
    const auto accepted=adapter.admit(*launch.run);
    check(bool(accepted),accepted.reason.c_str());
    const PortableBattleBorg expected_player{"example:flight-stick","normal"},expected_enemy{"example:jump-stick","normal"};
    check(adapter.accepted()&&adapter.accepted()->players().size()==1&&adapter.accepted()->players()[0]==expected_player&&
        adapter.accepted()->request.enemies.size()==1&&adapter.accepted()->request.enemies[0]==expected_enemy&&
        view.stage_id()=="example:workshop","accepted exact stage and ordered participants");
    check(!adapter.finish(*launch.run,{PortableDuelEnd::Win,0}),"adapter cannot advance at tick zero");
    NativeBattleMvp world;
    check(world.start(adapter.config(),&view),"admit two real actors to the exact stage");
    check(view.update(world,&error),error.c_str());
    struct Origins final:IBorgBattleOrigins {
        explicit Origins(const NativeBattleMvpView& v):view(v){}
        const NativeBattleMvpView& view;
        bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                    BattleMvpVec* out)const override{
            return view.package_socket(owner,actor,tick,move,frame,spec,nullptr,out);
        }
    } origins(view);
    auto end=PortableDuelEnd::Fighting;
    const auto limit=adapter.accepted()->request.tick_limit;
    for(unsigned frame=0;frame<300&&end==PortableDuelEnd::Fighting;++frame){
        const auto ai=portable_duel_enemy_input(world);
        check(world.step({GamePadState{},ai.movement},false,&view,nullptr,nullptr,nullptr,{{{},ai.attack}},&origins),
            "advance authored world with the shared socket sampler");
        check(view.update(world,&error),error.c_str());
        render.begin_frame({frame,.15f,.21f,.3f});
        render.reset_arena();
        const auto draws=view.draw(render,world,640,480,&error);
        check(render.end_frame()&&draws,error.c_str());
        end=portable_duel_end(world,limit);
    }
    const auto reported=adapter.finish(*launch.run,{end,world.tick()});
    check(end==PortableDuelEnd::Loss&&reported&&world.hits()>0&&!launch.run->completed_count(),"report correlated real AI defeat");
    check(!adapter.finish(*launch.run,{end,world.tick()}),"result cannot replay");
    render.shutdown();
    return 0;
}
} // namespace

int main(int argc,char** argv){
    if(argc==2)return seed(argv[1]);
    if(argc==3&&std::string_view(argv[1])=="--run-adapter")return run_adapter(argv[2]);
    if(argc!=1)return 2;
    saved_force_cases();
    duel_cases();
}
