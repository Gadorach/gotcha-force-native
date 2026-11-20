#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "platform/pc/borg_controller_load.hpp"

namespace {
namespace fs=std::filesystem;
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg controllers: %s\n",why);std::abort();}
}
std::string replace(std::string text,std::string_view old,std::string_view next){
    const auto at=text.find(old);check(at!=std::string::npos,"replacement fixture");
    text.replace(at,old.size(),next);return text;
}
struct Temp {
    fs::path path=fs::temp_directory_path()/("gf-borg-controller-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp(){check(fs::create_directory(path),"create owned temporary folder");}
    ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
};
void write(const fs::path& path,std::string_view text){
    std::ofstream file(path,std::ios::binary);
    check(bool(file.write(text.data(),std::streamsize(text.size()))),"write test fixture");
}
constexpr std::string_view manifest=R"(GFBORG 1
id=test:controller
name=Test Controller
model=model.dat
animations=motions.dat
rules=rules.txt
catalog=catalog.txt
)";
constexpr std::string_view catalogue=R"(GFBORG-CATALOG 2
description=Test controller declarations.
tribe=test:machine
default-variant=normal
variant=normal|Normal|100|placeholder
variant=bronze|Bronze|100|placeholder
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
moves=unresolved
traits=unresolved
forms=forms.txt
fusions=unresolved
acquisition=unresolved
effects=unresolved
variant-overrides=unresolved
controllers=controller.txt
)";
constexpr std::string_view forms=R"(GFBORG-FORMS 1
default-form=base
form=base|Base
model=unresolved
rig=unresolved
animations=unresolved
animation-map=unresolved
movement=unresolved
moves=unresolved
collision=unresolved
effects=unresolved
form=flight|Flight
model=unresolved
rig=unresolved
animations=none
animation-map=unresolved
movement=unresolved
moves=unresolved
collision=unresolved
effects=unresolved
)";
const std::string source=R"(GFBORG-CONTROLLERS 1
controller=normal|base
source=GG4E08:000A/0
source-movement-type=0
selector-family=standard-count
movement=grounded
contact=solid-floor
air-resource=air-jumps:3
recovery=grounded
motion-bank=local
pose=clip-based
actions=class-owned
evidence=source-audit
controller=normal|flight
source=none
source-movement-type=unresolved
selector-family=none
movement=continuous-flight
contact=unresolved
air-resource=boost-meter
recovery=not-boosting
motion-bank=none
pose=unresolved
actions=class-owned
evidence=authored
)";
}

int main(){
    using namespace gf;
    BorgControllerProfile profile;
    check(borg_read_controller_profile(source,&profile),"parse distinct controllers for named forms");
    const auto* base=profile.find("normal","base");
    const auto* flight=profile.find("normal","flight");
    check(base&&base->source_movement_type==0&&base->air_jump_points==3&&
        base->recovery==BorgControllerRecovery::Grounded&&!profile.find("bronze","base")&&
        !profile.find("normal","none"),"zero source type is known; no form or variant fallback");
    check(flight&&flight->motion_bank==BorgControllerBank::None&&flight->pose==BorgControllerPose::Unresolved&&
        flight->actions==BorgControllerActions::ClassOwned&&flight->recovery==BorgControllerRecovery::NotBoosting&&
        flight->contact==BorgControllerContact::Unresolved,"motion-bank absence does not erase pose or actions");
    for(const auto& bad:{
        replace(source,"normal|flight","normal|base"),
        replace(source,"normal|flight","normal|*"),
        replace(source,"GG4E08:000A/0","unresolved"),
        replace(source,"GG4E08:000A/0","GG4E08:GGGG/0"),
        replace(source,"GG4E08:000A/0","GG4E08:000A/256"),
        replace(source,"source-movement-type=0","source-movement-type=256"),
        replace(source,"selector-family=standard-count","selector-family=../standard-count"),
        replace(source,"air-jumps:3","air-jumps:0"),
        replace(source,"boost-meter","none"),
        replace(source,"motion-bank=local\n",""),
        source+"actions=none\n",
        source+"callback=0x8006FACC\n",
        source+std::string(kBorgControllerProfileBytes,'\n')
    })check(!borg_read_controller_profile(bad,&profile)&&profile.find("normal","base")->air_jump_points==3,
        "invalid scope, executable fields and unknown format cannot replace accepted declarations");

    Temp tmp;const auto folder=tmp.path/"Renamable folder";
    check(fs::create_directory(folder),"create test package folder");
    write(folder/"borg.manifest",manifest);write(folder/"catalog.txt",catalogue);
    for(const auto* file:{"model.dat","motions.dat","rules.txt"})write(folder/file,"Not loadable. Inspection does not open this asset.");
    write(folder/"controller.txt",source);write(folder/"forms.txt",forms);
    const auto scanned=pc::scan_borg_packages(tmp.path);
    pc::BorgControllerInspection inspected;std::string err;
    const auto load=[&](){return pc::load_borg_controller_inspection(tmp.path,scanned,"test:controller",&inspected,&err);};
    check(load()&&inspected.identity.id=="test:controller"&&inspected.profile.definitions.size()==2,
        "discover by stable ID and inspect without loading model, icons or gameplay rules");
    write(folder/"controller.txt",replace(source,"normal|base","clear|base"));
    check(!load()&&err.find("undeclared variant")!=std::string::npos&&inspected.profile.find("normal","base"),
        "undeclared variant cannot replace accepted inspection");
    write(folder/"controller.txt",replace(source,"normal|base","normal|bronze"));
    check(!load()&&err.find("undeclared form")!=std::string::npos,
        "catalogue variant is not a gameplay form");
    const auto without_forms=replace(replace(source,"normal|base","normal|none"),"normal|flight","normal|unresolved");
    write(folder/"controller.txt",without_forms);
    check(!load()&&err.find("requires forms=none")!=std::string::npos,"cannot use none when gameplay forms are declared");
    write(folder/"catalog.txt",replace(std::string(catalogue),"forms=forms.txt","forms=none"));
    check(load()&&inspected.profile.find("normal","none")&&inspected.profile.find("normal","unresolved"),
        "explicitly absent form and unresolved form are distinct inspection scopes");
    write(folder/"catalog.txt",replace(std::string(catalogue),"controllers=controller.txt","controllers=none"));
    check(!load()&&err.find("declares no")!=std::string::npos,"explicitly absent facet");
    write(folder/"catalog.txt",replace(std::string(catalogue),"controllers=controller.txt\n",""));
    check(!load()&&err.find("unresolved")!=std::string::npos,"legacy omitted facet stays unresolved");
    write(folder/"catalog.txt",catalogue);
    fs::remove(folder/"controller.txt");
    write(tmp.path/"external.txt",source);
    std::error_code ec;
    fs::create_symlink(tmp.path/"external.txt",folder/"controller.txt",ec);
    check(!load()&&err.find("invalid controller profile")!=std::string::npos,"missing or linked sidecar refuses");
    std::puts("Borg controllers: exact variant/form scope and independently declared mechanisms PASS");
}
