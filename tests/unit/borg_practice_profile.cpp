#include <cstdio>
#include <cstdlib>
#include "engine/assets/borg_practice_profile.hpp"

namespace {
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg practice profile: %s\n",why);std::abort();}
}
}
int main(){
    using namespace gf;
    using A=NativeGRedPracticeAssets;
    const std::string prefix="GFBORG-PRACTICE 1\r\ncontroller=normal-gred-gfmm9\r\nvariant=normal\r\n";
    const std::string suffix=
        "rig-prefix=visual/body.prefix\npose-profile=visual/body.pose\nlocomotion=rules/program.gfmp\n"
        "pose-height=rules/height.gfhc\nmovement-input=rules/input.gfmi\nmovement-motion=rules/motion.gfmm\n";
    BorgPracticeProfile profile;
    check(borg_read_normal_gred_practice_profile(prefix+suffix,&profile)&&profile.variant=="normal"&&
        profile.paths[A::RigPrefix]=="visual/body.prefix"&&profile.paths[A::MovementMotion]=="rules/motion.gfmm"&&
        profile.paths[A::Body].empty()&&profile.paths[A::Bank].empty(),"only the six file roles and two exact declarations");
    for(const auto& extra:{"model=body.hsd\n","rig-prefix=extra\n","variant=gold\n","fallback=normal\n",
                          "controller=normal-gred-gfmm9\n","source-stage=elsewhere.stih\n"}){
        check(!borg_read_normal_gred_practice_profile(prefix+suffix+extra,&profile)&&profile.paths[A::MovementMotion]=="rules/motion.gfmm",
            "no overrides, unknown keys, alternate stage reads, duplicates or partial output");
    }
    check(!borg_read_normal_gred_practice_profile(prefix+suffix.substr(0,suffix.find("movement-motion=")),&profile),
        "all component references are required");
    auto escaped=suffix;escaped.replace(escaped.find("visual/"),7,"../");
    check(!borg_read_normal_gred_practice_profile(prefix+escaped,&profile),"role paths cannot escape the package");
    check(!borg_read_normal_gred_practice_profile(std::string(kBorgPracticeProfileBytes+1,'x'),&profile),"bounded text");
    check(!A{}.qualified(),"parsed rules are not a qualified content snapshot");
    std::puts("Borg practice profile: exact normal variant, strict file roles, and fail-closed output PASS");
}
