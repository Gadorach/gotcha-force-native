#include <cstdio>
#include <cstdlib>
#include "engine/assets/hsd.hpp"
#include "engine/assets/hsd_fog.hpp"
namespace {unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"authored fog: %s\n",why);std::abort();}}}
int main(){using namespace gf;std::vector<u8> raw(128);store_be32(raw.data()+28,48);store_be32(raw.data()+48,64);store_be32(raw.data()+52,100);store_be32(raw.data()+64,2);store_be32(raw.data()+72,f32_to_bits(2000));store_be32(raw.data()+76,f32_to_bits(40000));store_be32(raw.data()+80,0xb8cff0ff);hsd_detail::Reader reader;reader.block={raw.data(),raw.size()};reader.fields={28,48,52};reader.targets={48,64,100};LinearFog fog;std::string e;
    check(!hsd_detail::read_scene_linear_fog(reader,16,&fog,&e)&&fog==LinearFog{},"legacy static guard still refuses published animation metadata");
    check(hsd_detail::read_scene_authored_linear_fog(reader,16,&fog,&e)&&fog==LinearFog{true,2000,40000,{184,207,240}},"authored initializer loads only source FogDesc values");
    for(u32 unread:{0u,0xffffffffu,0x12345678u}){store_be32(raw.data()+52,unread);check(hsd_detail::read_scene_authored_linear_fog(reader,16,&fog,&e)&&fog.start==2000,"unread animation word not fabricated/consumed by constructor");}
    store_be32(raw.data()+68,96);reader.fields={28,48,52,68};reader.targets={48,64,96,100};check(!hsd_detail::read_scene_authored_linear_fog(reader,16,&fog,&e)&&fog==LinearFog{},"reached range adjustment still unsupported");
    store_be32(raw.data()+68,0);reader.fields={28,48,52};store_be32(raw.data()+64,10);check(!hsd_detail::read_scene_authored_linear_fog(reader,16,&fog,&e),"unsupported source type still refuses");
    store_be32(raw.data()+64,2);store_be32(raw.data()+76,0x7fc00000);check(!hsd_detail::read_scene_authored_linear_fog(reader,16,&fog,&e),"nonfinite required scalar refuses");
    std::printf("authored initial fog: %u owned checks PASS; animation unrequested\n",checks);
}
