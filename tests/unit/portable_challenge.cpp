#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include "engine/gameplay/portable_challenge.hpp"
#include "platform/pc/portable_challenge_scan.hpp"

namespace {
using namespace gf;
using Status=PortableBattleStatus;

#define CHECK(x) do { if(!(x)){std::cerr<<__FILE__<<':'<<__LINE__<<": " #x "\n"; std::exit(1);} } while(false)

constexpr std::string_view first_encounter="encounter=round-one|example:shared|west|east|duel-ai-v1|defeat-opponent|216000\n";
constexpr std::string_view first_enemy="enemy=round-one|example:shared|gold\n";
constexpr std::string_view prefix="GFCHALLENGE 1\nid=example:trials\nname=Challenge \xc3\x89lan\n";

std::string manifest_text(){
    // Enemy rows can precede their encounter; the encounter order does not
    // come from the enemy rows, package folder or sorted IDs.
    return std::string(prefix)+std::string(first_enemy)+
        "enemy=alpha|example:shared|normal\n"+
        std::string(first_encounter)+
        "encounter=alpha|example:shared|west|east|duel-ai-v1|defeat-opponent|0\n";
}

PortableChallengeManifest manifest(){
    PortableChallengeManifest out;
    CHECK(portable_challenge_read_manifest(manifest_text(),&out));
    return out;
}

void parse(){
    auto parsed=manifest();
    CHECK(parsed.id=="example:trials"&&parsed.name=="Challenge \xc3\x89lan");
    CHECK(parsed.encounters.size()==2&&parsed.encounters[0].id=="round-one"&&parsed.encounters[1].id=="alpha");
    CHECK(parsed.encounters[0].tick_limit==kPortableBattleTickLimit&&parsed.encounters[1].tick_limit==0);
    CHECK((parsed.encounters[0].enemies==std::vector<PortableBattleBorg>{{"example:shared","gold"}}));
    CHECK((parsed.encounters[1].enemies==std::vector<PortableBattleBorg>{{"example:shared","normal"}}));
    CHECK(!parsed.find("ROUND-ONE"));
    const auto invalid=[&](std::string text){
        CHECK(!portable_challenge_read_manifest(text,&parsed));
        CHECK(parsed.id=="example:trials"&&parsed.encounters.size()==2);
    };
    const auto one=std::string(prefix)+std::string(first_encounter)+std::string(first_enemy);
    PortableChallengeManifest commented;
    CHECK(portable_challenge_read_manifest(one+"# valid UTF-8 comment\t\xc3\x89\n",&commented));
    invalid("# header must be first\n"+one);
    invalid(one+"id=example:duplicate\n");
    invalid(one+"name=Duplicate\n");
    invalid(one+"reward=free-borg\n");
    invalid(one+"# malformed UTF-8 in comment: \xed\xa0\x80\n");
    invalid(one+"# malformed UTF-8 in comment: \xf0\x80\x80\x80\n");
    invalid(one+std::string(first_encounter));
    invalid(one+"enemy=missing|example:shared|normal\n");
    invalid(one+"enemy=round-one|example:shared|\n");
    invalid(one+"enemy=round-one|example:shared|Gold\n");
    invalid(one+"enemy=round-one|../other|normal\n");
    invalid(one+"encounter=no-enemy|example:shared|west|east|duel-ai-v1|defeat-opponent|0\n");
    for(const auto ticks:{"216001","-1","+1","4294967296","1x",""}){
        invalid(std::string(prefix)+"encounter=round-one|example:shared|west|east|duel-ai-v1|defeat-opponent|"+
            ticks+"\n"+std::string(first_enemy));
    }
    PortableChallengeManifest limit;
    auto padded=one+"#";
    padded.resize(kPortableChallengeManifestBytes,'x');
    CHECK(portable_challenge_read_manifest(padded,&limit));
    invalid(padded+"x");
    auto max_encounters=std::string(prefix);
    for(std::size_t i=0;i<kPortableChallengeEncounters;++i){
        const auto id=std::to_string(i);
        max_encounters+="encounter="+id+"|example:shared|west|east|duel-ai-v1|defeat-opponent|0\n"+
            "enemy="+id+"|example:shared|gold\n";
    }
    CHECK(portable_challenge_read_manifest(max_encounters,&limit));
    invalid(max_encounters+"encounter=too-many|example:shared|west|east|duel-ai-v1|defeat-opponent|0\n");
    auto max_enemies=one;
    for(std::size_t i=1;i<kPortableBattleRosterLimit;++i)max_enemies+=first_enemy;
    CHECK(portable_challenge_read_manifest(max_enemies,&limit));
    CHECK(limit.encounters.front().enemies.size()==kPortableBattleRosterLimit);
    invalid(max_enemies+std::string(first_enemy));
    CHECK(portable_challenge_read_manifest("GFCHALLENGE 1\r\nid=example:trials\r\nname=CRLF\r\n"+
        std::string(first_encounter)+std::string(first_enemy),&limit));
    auto ordered=one+"enemy=round-one|example:shared|clear\n"+"enemy=round-one|example:other|normal\n";
    CHECK(portable_challenge_read_manifest(ordered,&limit));
    CHECK((limit.encounters[0].enemies==std::vector<PortableBattleBorg>{{"example:shared","gold"},
        {"example:shared","clear"},{"example:other","normal"}}));
}

void discovery(){
    namespace fs=std::filesystem;
    using Scan=PortableChallengeScanStatus;
    const struct Temp {
        fs::path path=fs::temp_directory_path()/("gf-portable-challenge-"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
        Temp(){CHECK(fs::create_directory(path));}
        ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
    } temp;
    const auto root=temp.path/"challenges";
#if !defined(__linux__)
    CHECK(pc::scan_portable_challenges(root).status==Scan::Unavailable);
#else
    using Issue=PortableChallengeIssueKind;
    const auto import=temp.path/"import";
    CHECK(pc::scan_portable_challenges(root).status==Scan::NoDirectory);
    const auto write=[](const fs::path& file,std::string_view text){
        std::ofstream stream(file,std::ios::binary);
        CHECK(bool(stream.write(text.data(),std::streamsize(text.size()))));
    };
    const auto package=[&](const fs::path& folder,std::string_view text){
        CHECK(fs::create_directories(folder));
        write(folder/"challenge.manifest",text);
    };
    const auto one=manifest_text();
    auto two=one;
    two.replace(two.find("example:trials"),std::string_view("example:trials").size(),"example:a");
    package(root/"aardvark-folder",one);
    package(root/"zebra-folder",two);
    package(root/"malformed","GFCHALLENGE 2\n");
    package(root/"nested/ignored",one); // No recursive discovery.
    CHECK(fs::create_directory(root/"missing"));
    const auto first=pc::scan_portable_challenges(root);
    CHECK(first.status==Scan::Complete&&first.discovered.size()==2&&first.issues.size()==3);
    CHECK(first.discovered[0].manifest.id=="example:a"&&first.discovered[1].manifest.id=="example:trials");
    CHECK(first.find("example:trials")&&first.find("example:trials")->folder=="aardvark-folder");
    CHECK(first.issues[0].kind==Issue::Malformed&&first.issues[1].kind==Issue::Missing);
    package(import/"collision",one);
    const std::array roots{root,import};
    const auto merged=pc::scan_portable_challenges(roots);
    CHECK(merged.status==Scan::Complete&&merged.discovered.size()==1&&merged.find("example:a"));
    CHECK(!merged.find("example:trials")&&merged.issues.size()==5);
    CHECK(std::count_if(merged.issues.begin(),merged.issues.end(),
        [](const auto& issue){return issue.kind==Issue::DuplicateId&&issue.id=="example:trials";})==2);
    CHECK(first.find("example:trials")); // Prior parsed data stays pinned.
    std::error_code error;
    fs::create_directory_symlink(root,temp.path/"root-link",error);
    if(!error){
        CHECK(pc::scan_portable_challenges(temp.path/"root-link").status==Scan::Unavailable);
        CHECK(pc::scan_portable_challenges(temp.path/"root-link"/"").status==Scan::Unavailable);
        CHECK(pc::scan_portable_challenges(temp.path/"root-link"/"missing").status==Scan::Unavailable);
    }
    CHECK(pc::scan_portable_challenges(root/".."/"challenges").status==Scan::Unavailable);
    const auto link_check=[&](std::string_view folder_name){
        const auto catalog=pc::scan_portable_challenges(root);
        CHECK(catalog.status==Scan::Complete&&catalog.discovered.size()==2);
        CHECK(std::any_of(catalog.issues.begin(),catalog.issues.end(),[&](const auto& issue){
            return issue.folder==folder_name&&issue.kind==Issue::Malformed;
        }));
    };
    fs::create_directory_symlink(import/"collision",root/"folder-link",error);
    if(!error)link_check("folder-link");
    CHECK(fs::create_directory(root/"manifest-link"));
    fs::create_symlink(import/"collision/challenge.manifest",root/"manifest-link/challenge.manifest",error);
    if(!error)link_check("manifest-link");
    CHECK(fs::create_directory(root/"hard-link"));
    fs::create_hard_link(import/"collision/challenge.manifest",root/"hard-link/challenge.manifest",error);
    if(!error)link_check("hard-link");
    std::array<fs::path,pc::kPortableChallengeScanRoots+1> too_many_roots;
    CHECK(pc::scan_portable_challenges(too_many_roots).status==Scan::EntryBudget);
#endif
}

struct Fixture {
    PortableBattleForceSnapshot force{1,{{"example:player","silver"}}};
    PortableBattleCheck stage_check{Status::Ready,{}};
    PortableBattleCheck force_check{Status::Ready,{}};
    std::vector<std::pair<PortableBattleSide,PortableBattleBorg>> checked_borgs;
    bool supports_timed=true,supports_gold=true;
    unsigned stage_calls=0,force_calls=0;
    PortableBattleResolver resolver{
        [&](const PortableBattleStage& stage,std::span<const PortableBattleSpawn> player,
                std::span<const PortableBattleSpawn> enemy)->PortableBattleCheck{
            ++stage_calls;
            CHECK(stage.id=="example:shared"&&player.size()==1&&enemy.size()==1);
            CHECK(player.front().id=="west"&&enemy.front().id=="east");
            return stage_check;
        },
        [&](const PortableBattleBorg& borg,PortableBattleSide side)->PortableBattleCheck{
            checked_borgs.emplace_back(side,borg);
            if(borg.variant_id=="missing"||(borg.variant_id=="gold"&&!supports_gold))
                return {Status::Missing,"exact variant not found"};
            return {Status::Ready,{}};
        },
        [&](PortableBattleSavedForce selection,PortableBattleUse,PortableBattleForceSnapshot* out)->PortableBattleCheck{
            ++force_calls;
            CHECK(selection.index==2);
            if(force_check)*out=force;
            return force_check;
        },
        [&](const PortableBattleRequest& request)->PortableBattleCheck{
            return !supports_timed&&request.tick_limit?PortableBattleCheck{Status::Unsupported,"host has no timer"}:
                PortableBattleCheck{Status::Ready,{}};
        }
    };
};

void preflight(){
    Fixture f;
    const auto data=manifest();
    CHECK(!portable_challenge_battle_request(data,"does-not-exist",{2}));
    const auto seed=portable_challenge_battle_request(data,"round-one",{2});
    CHECK(seed);
    auto request=*seed;
    PortableBattlePrepared out;
    CHECK(preflight_portable_battle_request(request,f.resolver,&out));
    CHECK(out.request==request&&out.players()==f.force.members);
    CHECK(f.checked_borgs.size()==2&&f.checked_borgs[0].first==PortableBattleSide::Player&&
        f.checked_borgs[1].first==PortableBattleSide::Enemy&&f.checked_borgs[1].second.variant_id=="gold");
    const auto accepted=out;
    f.checked_borgs.clear();
    f.force_calls=f.stage_calls=0;
    auto result=[&](Status expected){
        CHECK(preflight_portable_battle_request(request,f.resolver,&out).status==expected);
        CHECK(out==accepted);
    };
    request.enemies.push_back({"example:shared","normal"});
    result(Status::Unsupported);
    CHECK(f.force_calls==0&&f.stage_calls==0&&f.checked_borgs.empty());
    request=*seed;
    request.player_spawns.push_back({"back"});
    result(Status::Unsupported);
    request=*seed;
    request.enemy_spawns.front()=request.player_spawns.front();
    result(Status::Unsupported);
    request=*seed;
    request.ai.key="unimplemented";
    result(Status::Unsupported);
    request=*seed;
    request.rules.key="unimplemented";
    result(Status::Unsupported);
    CHECK(f.force_calls==0&&f.stage_calls==0&&f.checked_borgs.empty());
    request=*seed;
    request.player=PortableBattleSavedForce{};
    result(Status::Malformed);
    request=*seed;
    f.force_check={Status::Denied,"edited copy is not allowed in campaign"};
    result(Status::Denied);
    f.force_check={Status::Ready,{}};
    f.force.members.push_back({"example:player","normal"});
    result(Status::Unsupported);
    CHECK(f.stage_calls==0&&f.checked_borgs.empty());
    f.force.members.clear();
    result(Status::Denied);
    f.force.members.push_back({"example:player",""});
    result(Status::Malformed);
    f.force.members.front().variant_id="silver";
    f.stage_check={Status::Missing,"no selected stage or spawn marker"};
    result(Status::Missing);
    CHECK(f.checked_borgs.empty());
    f.stage_check={Status::Ready,{}};
    f.supports_gold=false;
    result(Status::Missing);
    CHECK(f.checked_borgs.size()==2&&f.checked_borgs.back().second.variant_id=="gold");
    f.supports_gold=true;
    f.supports_timed=false;
    result(Status::Unsupported);
    f.supports_timed=true;
    request.player=PortableBattleAuthoredRoster{{{"example:player","normal"}}};
    const auto force_calls=f.force_calls;
    CHECK(preflight_portable_battle_request(request,f.resolver,&out));
    CHECK(!out.saved_force&&out.players().front().variant_id=="normal"&&f.force_calls==force_calls);
}

void progress(){
    Fixture f;
    auto data=manifest();
    PortableChallengeRun run(data,{2});
    data.encounters[0].id="changed-after-start";
    PortableBattleReceipt receipt,foreign,old;
    CHECK(run.current()&&run.current()->id=="round-one"&&run.completed_count()==0);
    CHECK(!run.complete(receipt,PortableBattleOutcome::Win));
    CHECK(run.accept(f.resolver,&receipt).status==Status::Denied);
    CHECK(run.prepare(f.resolver));
    f.stage_check={Status::Missing,"missing stage"};
    CHECK(run.prepare(f.resolver).status==Status::Missing);
    CHECK(run.accept(f.resolver,&receipt).status==Status::Denied); // Failed prepare clears a prior offer.
    f.stage_check={Status::Ready,{}};
    CHECK(run.prepare(f.resolver));
    CHECK(!run.complete(receipt,PortableBattleOutcome::Win));
    ++f.force.revision; // Same visible variant, but a changed selected Force.
    CHECK(run.accept(f.resolver,&receipt).status==Status::Stale);
    CHECK(run.accept(f.resolver,&receipt).status==Status::Denied);
    CHECK(!receipt.accepted());
    CHECK(run.prepare(f.resolver)&&run.accept(f.resolver,&receipt));
    CHECK(receipt.accepted()&&receipt.accepted()->request.encounter_id=="round-one");
    CHECK(run.prepare(f.resolver).status==Status::Busy);
    CHECK(run.accept(f.resolver,&foreign).status==Status::Busy);
    old=receipt;
    CHECK(run.cancel(receipt));
    CHECK(!run.complete(old,PortableBattleOutcome::Win)&&run.completed_count()==0);
    PortableChallengeRun other(manifest(),{2});
    CHECK(other.prepare(f.resolver)&&other.accept(f.resolver,&foreign));
    CHECK(run.prepare(f.resolver)&&run.accept(f.resolver,&receipt));
    CHECK(!run.complete(old,PortableBattleOutcome::Win));
    CHECK(!run.cancel(old));
    CHECK(!run.cancel(foreign));
    CHECK(!run.complete(foreign,PortableBattleOutcome::Win));
    CHECK(!run.complete(receipt,static_cast<PortableBattleOutcome>(100)));
    CHECK(run.complete(receipt,PortableBattleOutcome::Loss)&&run.completed_count()==0);
    CHECK(!run.complete(receipt,PortableBattleOutcome::Win));
    CHECK(run.prepare(f.resolver)&&run.accept(f.resolver,&receipt));
    CHECK(run.complete(receipt,PortableBattleOutcome::Win)&&run.completed_count()==1);
    CHECK(run.current()&&run.current()->id=="alpha");
    CHECK(!run.complete(receipt,PortableBattleOutcome::Win));
    old=receipt;
    CHECK(run.prepare(f.resolver)&&run.accept(f.resolver,&receipt));
    CHECK(!run.complete(old,PortableBattleOutcome::Win));
    CHECK(run.complete(receipt,PortableBattleOutcome::Win)&&run.completed_count()==2);
    CHECK(run.finished()&&!run.current()&&run.prepare(f.resolver).status==Status::Denied);
    CHECK(!run.complete(receipt,PortableBattleOutcome::Win));
}

void noncampaign(){
    Fixture f;
    const auto data=manifest();
    PortableBattleSession session;
    for(const auto use:{PortableBattleUse::Preview,PortableBattleUse::Practice}){
        const auto request=portable_challenge_battle_request(data,"round-one",{2},use);
        CHECK(request&&session.prepare(*request,f.resolver));
        PortableBattleReceipt receipt;
        CHECK(session.accept(f.resolver,&receipt));
        CHECK(!session.complete(receipt,PortableBattleOutcome::Win));
        session.cancel();
    }
}

} // namespace

int main(){
    parse();
    discovery();
    preflight();
    progress();
    noncampaign();
}
