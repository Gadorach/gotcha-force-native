#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_controller_add_on.hpp"
#include "platform/pc/borg_controller_load.hpp"

namespace {
using namespace gf;
using Kind=BorgControllerAddOn;
using Status=BorgControllerAddOnStatus;
namespace fs=std::filesystem;
const BorgIdentity identity{"test:tranche","Editable display name"};
constexpr std::string_view exchange="normal-switching-ninja-exchange";
constexpr std::string_view teleport="normal-teleport-ninja-history";
constexpr std::string_view icbm="normal-death-icbm-impact";

void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg controller add-on: %s\n",why);std::abort();}
}
std::string controller(std::string_view source,std::string_view add_on,std::string_view scope="normal|none"){
    std::string row="controller=";row+=scope;row+="\nsource=";row+=source;
    row+=R"(
source-movement-type=unresolved
selector-family=unresolved
movement=unresolved
contact=unresolved
air-resource=unresolved
recovery=unresolved
motion-bank=unresolved
pose=unresolved
actions=unresolved
evidence=source-audit
)";
    if(!add_on.empty()){row+="add-on=";row+=add_on;row+='\n';}
    return row;
}
std::string profile_text(std::string_view source,std::string_view add_on,std::string_view scope="normal|none"){
    return "GFBORG-CONTROLLERS 2\n"+controller(source,add_on,scope);
}
BorgControllerProfile read(std::string_view source,std::string_view add_on){
    BorgControllerProfile out;
    check(borg_read_controller_profile(profile_text(source,add_on),&out),"accept valid named add-on");
    return out;
}
BorgControllerSourceObservation observed(u16 model,u8 slot=3){
    return {identity.id,"normal","none",{"GG4E08",model,0},slot};
}
BorgDetachedControllerAddOn bind(const BorgControllerProfile& profile,const BorgControllerSourceObservation* in=nullptr){
    return BorgDetachedControllerAddOn::bind(identity,profile,"normal","none",in);
}

void format_checks(){
    BorgControllerProfile out;
    const auto known=profile_text("GG4E08:0009/0",exchange);
    check(borg_read_controller_profile(known,&out)&&out.find("normal","none")->add_on==Kind::SwitchingNinjaExchange&&
        borg_controller_add_on_name(out.find("normal","none")->add_on)==exchange,"round-trip stable add-on label");
    for(const auto& invalid:{
        profile_text("GG4E08:0005/0",exchange),
        profile_text("GG4E08:0009/1",exchange),
        profile_text("GG4P08:0009/0",exchange),
        profile_text("GG4E08:0009/0",exchange,"bronze|none"),
        profile_text("GG4E08:0009/0",exchange,"normal|base"),
        profile_text("GG4E08:0009/0",exchange,"normal|unresolved"),
        profile_text("GG4E08:0D07/0",icbm),
        profile_text("GG4E08:0009/0",teleport),
        profile_text("GG4E08:0009/0","dll:arbitrary"),
        profile_text("GG4E08:0009/0","percent-100"),
        profile_text("GG4E08:0009/0",""),
        known+"add-on=none\n",
        known+"verified=yes\n",
        "GFBORG-CONTROLLERS 1\n"+controller("GG4E08:0009/0",exchange)
    })check(!borg_read_controller_profile(invalid,&out)&&out.find("normal","none")->add_on==Kind::SwitchingNinjaExchange,
            "refuse unknown, incomplete or wrongly scoped declarations without replacing output");
    check(borg_read_controller_profile("GFBORG-CONTROLLERS 1\n"+controller("GG4E08:0009/0",""),&out)&&
        bind(out).status()==Status::Unresolved,"v1 does not implicitly opt into an add-on");
    check(bind(read("GG4E08:0009/0","none")).status()==Status::None&&
        bind(read("GG4E08:0009/0","unresolved")).status()==Status::Unresolved&&
        bind({}).status()==Status::NoController,"explicit absence, unresolved and missing row remain distinct");
    const auto declared=bind(read("GG4E08:0009/0",exchange));
    SwitchingNinjaStagedExchange staged;
    check(declared.status()==Status::NeedsSourceObservation&&!declared.stage_exchange({},&staged),
        "source audit field and exact-looking source do not authorize a planner");
}

void exchange_checks(){
    auto profile=read("GG4E08:0009/0",exchange);auto source=observed(9);
    profile.definitions[0].evidence=BorgControllerEvidence::Authored;
    const auto exact=bind(profile,&source);
    check(exact.status()==Status::DetachedPlanner&&exact.identity()==identity&&exact.variant()=="normal"&&
        exact.form()=="none"&&exact.kind()==Kind::SwitchingNinjaExchange,"bind independently of the metadata evidence label");
    for(const auto& denied:{
        BorgControllerSourceObservation{"test:other","normal","none",{"GG4E08",9,0},3},
        BorgControllerSourceObservation{identity.id,"bronze","none",{"GG4E08",9,0},3},
        BorgControllerSourceObservation{identity.id,"normal","base",{"GG4E08",9,0},3},
        BorgControllerSourceObservation{identity.id,"normal","none",{"GG4E08",5,0},3},
        BorgControllerSourceObservation{identity.id,"normal","none",{"GG4E08",9,1},3},
        BorgControllerSourceObservation{identity.id,"normal","none",{"GG4P08",9,0},3},
        observed(9,6)
    })check(bind(profile,&denied).status()==Status::Refused,"refuse mismatched host observations");
    check(BorgDetachedControllerAddOn::bind(identity,profile,"bronze","none",&source).status()==Status::NoController&&
        BorgDetachedControllerAddOn::bind(identity,profile,"normal","base",&source).status()==Status::NoController,
        "no variant or form fallback");
    profile.definitions[0].source="GG4E08:0005/0";
    check(bind(profile,&source).status()==Status::Refused,"binding checks invalid in-memory metadata too");
    profile={};source={};
    SwitchingNinjaContact contact{9,0,20,12,true,{3,0,0,0,0,{11,20,30},3},{1,0,0,0,0,{-700,90,800},10}};
    SwitchingNinjaStagedExchange result;
    check(exact.stage_exchange(contact,&result)&&result.caster==SwitchingNinjaLeg{true,3,{-700,97,800}}&&
        result.recipient==SwitchingNinjaLeg{true,1,{11,13,30}},"bound plan stages two actual contact legs after source data is gone");
    const auto accepted=result;
    contact.caster.slot=2;
    check(!exact.stage_exchange(contact,&result)&&result==accepted,"contact caster must own the bound slot");
    contact.caster.slot=3;contact.hit_row=11;
    check(!exact.stage_exchange(contact,&result)&&result==accepted,"wrong contact cannot satisfy the planner");
    check(!exact.teleport_source(3),"a contact exchange does not expose caster teleport");
}

void teleport_checks(){
    const auto source=observed(5);
    const auto exact=bind(read("GG4E08:0005/0",teleport),&source);
    TeleportSetupPlan result;
    check(exact.status()==Status::DetachedPlanner&&exact.plan_teleport_history(3,{3,4,5},{3,2,8},0x40,0,true,&result)&&
        result.goal==TeleportPosition{3,104,-145},"caster history proposes only the caster goal");
    const auto accepted=result;
    check(!exact.plan_teleport_history(3,{3,4,5},{3,2,8},0x40,1,true,&result)&&result==accepted&&
        !exact.plan_teleport_history(1,{3,4,5},{3,2,8},0x40,0,true,&result)&&result==accepted,
        "directed case and off-slot samples refuse unchanged");
    SwitchingNinjaStagedExchange staged;
    check(!exact.stage_exchange({},&staged),"caster movement cannot dispatch exchange");
}

void impact_checks(){
    const auto source=observed(0x0d06,4);
    const auto exact=bind(read("GG4E08:0d06/00",icbm),&source);
    IcbmGameImpactInput in{true,0,1,2,0,0,0,IcbmImpactProbeResult::Unresolved,IcbmImpactProbeResult::Unresolved,true,{9,18,-5}};
    IcbmGameImpactPlan result;
    check(exact.status()==Status::DetachedPlanner&&exact.plan_impact(4,in,&result)&&
        result.next_query==IcbmImpactQuery::Floor&&!result.request_area_object,"contact begins with the shared floor query");
    in.floor=IcbmImpactProbeResult::Clear;
    check(exact.plan_impact(4,in,&result)&&result.next_query==IcbmImpactQuery::Scene&&!result.write_pending_terminal,
        "clear floor requires the separate scene query");
    in.scene=IcbmImpactProbeResult::Clear;
    check(exact.plan_impact(4,in,&result)&&result==IcbmGameImpactPlan{},"no contact means no impact or terminal write");
    in.scene=IcbmImpactProbeResult::Contact;in.role493=1;
    check(exact.plan_impact(4,in,&result)&&result==IcbmGameImpactPlan{},"alternate role suppresses impact despite contact");
    in.role493=0;in.lifecycle18=2;
    check(exact.plan_impact(4,in,&result)&&result==IcbmGameImpactPlan{},"committed defeat suppresses impact");
    in.lifecycle18=1;
    check(exact.plan_impact(4,in,&result)&&result.request_area_object&&result.owner_slot==4&&result.object_type==73&&
        !result.argument&&result.request_position==in.matrix_translation&&result.write_pending_terminal&&result.pending663==1,
        "only qualified contact requests the caster-owned area object and separate terminal write");
    const auto accepted=result;
    check(!exact.plan_impact(0,in,&result)&&result==accepted,"another slot cannot supply impact samples");
    IcbmGameDefeatPlan defeat;
    check(exact.consume_terminal(4,1,result.pending663,0x80,true,&defeat)&&defeat==IcbmGameDefeatPlan{true,4,0x81,0},
        "self-defeat is bound to the caster and does not await an area allocation receipt");
    check(!exact.consume_terminal(0,1,result.pending663,0x80,true,&defeat)&&defeat.slot==4,
        "terminal request cannot be redirected to another actor");
}

struct Temp {
    fs::path path=fs::temp_directory_path()/("gf-borg-addon-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp(){check(fs::create_directory(path),"create owned test folder");}
    ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
};
void write(const fs::path& path,std::string_view txt){
    std::ofstream file(path,std::ios::binary);
    check(bool(file.write(txt.data(),std::streamsize(txt.size()))),"write fixture file");
}
void package_checks(){
    Temp tmp;const auto folder=tmp.path/"independent display name";
    check(fs::create_directory(folder),"create discovered package");
    write(folder/"borg.manifest",R"(GFBORG 1
id=test:tranche
name=Editable display name
model=model.dat
animations=motions.dat
rules=rules.txt
catalog=catalog.txt
)");
    const auto catalogue=R"(GFBORG-CATALOG 2
description=Detached controller test.
tribe=none
default-variant=normal
variant=normal|Normal|100|placeholder
collection-number=unresolved
combat-role=unresolved
rarity-grade=unresolved
crystals=unresolved
alternate=unresolved
level-schedule=unresolved
experience=unresolved
display-jump=unresolved
hp=unresolved|unresolved
ratings=unresolved|unresolved|unresolved|unresolved
moves=none
traits=unresolved
fusions=unresolved
acquisition=unresolved
effects=unresolved
variant-overrides=unresolved
controllers=controllers.txt
)";
    write(folder/"catalog.txt",std::string(catalogue)+"forms=none\n");
    write(folder/"controllers.txt",profile_text("GG4E08:0009/0",exchange));
    for(const auto* file:{"model.dat","motions.dat","rules.txt"})write(folder/file,"Opaque placeholders; this path loads no executable or visual assets.");
    const auto packages=pc::scan_borg_packages(tmp.path);
    pc::BorgControllerInspection inspected;std::string error;
    const auto load=[&](){return pc::load_borg_controller_inspection(tmp.path,packages,identity.id,&inspected,&error);};
    check(load()&&inspected.identity==identity&&bind(inspected.profile).status()==Status::NeedsSourceObservation,
        "existing catalogue path loads the declaration but grants no executable capability on its own");
    write(folder/"catalog.txt",std::string(catalogue)+"forms=unresolved\n");
    check(!load()&&error.find("requires forms=none")!=std::string::npos&&
        inspected.profile.find("normal","none")->add_on==Kind::SwitchingNinjaExchange,
        "package loading independently validates the exact form scope");
}
}

int main(){
    format_checks();
    exchange_checks();
    teleport_checks();
    impact_checks();
    package_checks();
    std::puts("Borg controller add-ons: versioned format, exact source/slot binding and detached plans PASS");
}
