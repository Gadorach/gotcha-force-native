#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include "engine/gameplay/borgs/normal_titan_robot_form.hpp"

namespace {
unsigned checks=0;
void check(bool ok,const char* why){
    ++checks;
    if(!ok){std::fprintf(stderr,"normal Titan Robot: %s\n",why);std::abort();}
}
}

int main(){
    using namespace gf;
    using S=NormalTitanRobotSource;
    using F=TitanTransformFloorReturn;
    constexpr S robot{S::robot,0},form{S::tank_form,0},retail_tank{0x0619,0};
    static_assert(robot.reviewed()&&form.reviewed()&&!retail_tank.reviewed()&&!retail_tank.cycle_destination());
    TitanRobotFormTraits traits;
    check(normal_titan_robot_form_traits(robot,&traits)&&traits==TitanRobotFormTraits{0,0,1,true},"robot movement and original data block");
    check(normal_titan_robot_form_traits(form,&traits)&&traits==TitanRobotFormTraits{432,1,1,false},"tank movement replaces robot-only selectors");
    check(!normal_titan_robot_form_traits(retail_tank,&traits)&&traits==TitanRobotFormTraits{432,1,1,false},
        "matching movement type cannot substitute retail Tank");

    for(auto source:{robot,form}){
        TitanRobotCycleRequest request;
        check(normal_titan_robot_cycle_request(source,0,0,true,&request)&&!request.requested,"no conditioned pulse");
        check(normal_titan_robot_cycle_request(source,0x80,4,true,&request)&&!request.requested,"exact group lock");
        check(normal_titan_robot_cycle_request(source,0x80,2,true,&request)&&request.requested&&request.action_group==2,
            "conditioned pulse proposes group 2 without claiming class acceptance");
        check(!normal_titan_robot_cycle_request(source,0x80,0,false,&request)&&request.requested,
            "unreached request leaf cannot publish");
        for(auto contact:{F::NoContact,F::Contact}){
            TitanRobotCyclePreparation prepared;
            check(normal_titan_robot_cycle_prepare(source,2,0,contact,&prepared)&&
                prepared==TitanRobotCyclePreparation{source.cycle_destination(),4,0},
                "both exact floor-return branches choose the same program and opposite model");
            ModelFormRebindPrefix binding;
            check(normal_titan_robot_cycle_rebind(source,prepared.target_model548,0,7,&binding)&&!binding.requested,
                "prepared program still needs actual lower-lane completion");
            check(normal_titan_robot_cycle_rebind(source,prepared.target_model548,128,255,&binding)&&
                binding==ModelFormRebindPrefix{true,source.cycle_destination(),0,0},
                "lower completion gates both directions and wraps only the revision byte");
            const auto retained=binding;
            check(!normal_titan_robot_cycle_rebind(source,source.model,1,7,&binding)&&binding==retained,
                "same or stale staged model is not the opposing form");
            check(!normal_titan_robot_cycle_rebind(source,0x0619,1,7,&binding)&&binding==retained,
                "separately ownable retail Tank never replaces the form");
            check(!normal_titan_robot_cycle_prepare(source,2,0,F::Unresolved,&prepared)&&
                !normal_titan_robot_cycle_prepare(source,3,0,contact,&prepared)&&
                !normal_titan_robot_cycle_prepare(source,2,1,contact,&prepared)&&prepared.program_group==4,
                "unresolved floor and wrong class entry cannot request the pose");
        }
    }
    ModelFormRebindPrefix binding;
    check(normal_titan_robot_flagged_return(form,0x1000,1,&binding)&&!binding.requested,
        "arbitrary contact/action flag isn't this interrupt");
    check(normal_titan_robot_flagged_return(form,0x02000000,1,&binding)&&binding==ModelFormRebindPrefix{true,S::robot,0,2},
        "flagged return has no X-cycle completion input");
    check(!normal_titan_robot_flagged_return(retail_tank,0x02000000,1,&binding)&&binding.target_model==S::robot&&
        !normal_titan_robot_flagged_return(robot,0x02000000,1,&binding)&&binding.target_model==S::robot,
        "only the exact internal tank form accepts that prefix");
    TitanRobotCycleRequest request;
    check(!normal_titan_robot_cycle_request(retail_tank,0x80,0,true,&request)&&
        !normal_titan_robot_cycle_request({S::robot,1},0x80,0,true,&request),
        "retail Tank's weapon and alternate variants cannot borrow the form path");
    std::printf("normal Titan Robot cyclic form and exact flag return: %u checks PASS\n",checks);
}
