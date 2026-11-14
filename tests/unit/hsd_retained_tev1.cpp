#include <cstdio>
#include <cstdlib>
#include <limits>
#include "engine/gameplay/animated_model_scene.hpp"
#include "tests/unit/hsd_record_texture_fixture.hpp"
int main(){using namespace gf;unsigned checks=0;auto check=[&](bool x){++checks;if(!x)std::abort();};
    for(unsigned channel=20;channel<=23;++channel)for(float value:{-1.f,0.f,.1f,.5f,1.f,2.f}){
        HsdTextureState a;a.colour_lerp=true;a.tev1=0x12345678;const auto before=a;
        check(!hsd_texture_callback(channel,value,&a)&&a.tev1==before.tev1);
        check(hsd_texture_callback_retaining_tev1(channel,value,&a));
        const unsigned shift=(23-channel)*8;const u32 byte=u32(f32(std::clamp(value,0.f,1.f)*255.f));
        check(a.tev1==((before.tev1&~(0xffu<<shift))|(byte<<shift))&&a.low==before.low&&a.high==before.high);
    }
    HsdTextureState a;check(!hsd_texture_callback_retaining_tev1(20,0,&a));a.colour_lerp=true;
    check(!hsd_texture_callback_retaining_tev1(20,std::numeric_limits<f32>::quiet_NaN(),&a));
    check(!hsd_texture_callback_retaining_tev1(20,std::numeric_limits<f32>::infinity(),&a));
    check(!hsd_texture_callback_retaining_tev1(10,0,&a));
    for(unsigned channel=20;channel<=23;++channel){
        auto f=hsd_record_texture_fixture::fixture(channel,.5f,.5f,true);f.word(0xf18,0x12345678);auto bytes=f.archive();HsdRecordAnimationBank bank;std::string e;
        check(bank.load({bytes.data(),bytes.size()},&e));HsdRecordAnimationBinding rejected,binding;
        check(!bank.select_compact_textures(0,3,0,0,&rejected,&e)&&!rejected.scene());
        check(bank.select_compact_retained_tev1_textures(0,3,0,0,&binding,&e));
        HsdRecordAnimationPlayer player;check(player.bind(binding,&e)&&player.request(1,0,&e));
        const auto constants=player.texture_constants();check(constants.size()==1);
        const u32 expected=(0x12345678u&~(255u<<((23-channel)*8)))|(127u<<((23-channel)*8));
        check(constants[0][0]==0xffffffffu&&constants[0][1]==0&&constants[0][2]==expected);
        AnimatedModelScene model;check(model.load_compact_retained_tev1_textures(bank,0,3,0,0,&e)&&model.request(1,0,&e));
        check(model.ready());
        auto unsupported=hsd_record_texture_fixture::fixture(channel,.5f,.5f,false).archive();
        check(bank.load({unsupported.data(),unsupported.size()},&e));
        check(!bank.select_compact_retained_tev1_textures(0,3,0,0,&binding,&e));
    }
    std::printf("Retained TEV1 explicit path: %u checks PASS; old channel guard/expression guards retained\n",checks);
}
