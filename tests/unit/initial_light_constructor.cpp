#include <cstdio>
#include <cstdlib>
#include "engine/assets/hsd.hpp"
#include "engine/assets/hsd_light_constructor.hpp"
namespace {
unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}
struct Fixture {
    std::vector<gf::u8> bytes=std::vector<gf::u8>(1024);gf::hsd_detail::Reader reader;
    void word(unsigned at,gf::u32 v){gf::store_be32(bytes.data()+at,v);}
    void pointer(unsigned at,unsigned to){word(at,to);if(std::find(reader.fields.begin(),reader.fields.end(),at)==reader.fields.end())reader.fields.push_back(at);reader.targets.push_back(to);}
    void light(unsigned at,gf::u16 flags,unsigned next){word(at+8,gf::u32(flags)<<16);word(at+12,0xabcdefFF);if(next)pointer(at+4,next);if((flags&3)==1)pointer(at+16,600);}
    Fixture(){
        pointer(24,64);pointer(64,100);pointer(68,108);pointer(72,116);
        pointer(100,200);pointer(104,900);pointer(108,300);pointer(116,340);
        light(200,13,232);light(232,13,0);light(300,4,0);light(340,4,380);light(380,13,0);
        word(604,gf::f32_to_bits(3));word(608,gf::f32_to_bits(4));word(612,0);
    }
    bool load(gf::HsdInitialLightConstructor& out){reader.block={bytes.data(),bytes.size()};std::sort(reader.fields.begin(),reader.fields.end());std::sort(reader.targets.begin(),reader.targets.end());std::string error;return gf::hsd_detail::read_initial_light_constructor(reader,16,&out,&error);}
};
}
int main(){
    using namespace gf;HsdInitialLightConstructor out;
    {Fixture f;check(f.load(out)&&out.generation==1&&out.nodes.size()==5&&out.loaded_heads==std::vector<u16>{0,2,3}&&out.head==0&&!out.primary_infinite);check(out.nodes[0].next==2&&out.nodes[2].next==3&&out.nodes[3].next==4&&!out.nodes[1].next);check(out.nodes[1].source_descriptor==232&&out.animation_descriptors_present&&!out.animation_requested);}
    {Fixture f;f.light(300,13,0);check(f.load(out)&&out.primary_infinite==2&&out.nodes[*out.primary_infinite].source_descriptor==300);check(out.nodes[4].source_descriptor==380&&out.primary_infinite!=4);}
    {Fixture f;f.light(300,13,0);f.pointer(116,300);check(f.load(out)&&out.nodes.size()==4&&out.primary_infinite==3&&out.nodes[2].source_descriptor==out.nodes[3].source_descriptor&&out.nodes[2].next==3);}
    {Fixture f;f.word(68,0);check(f.load(out)&&out.nodes.size()==2&&out.head==0&&!out.primary_infinite);}
    {Fixture f;f.word(64,0);check(f.load(out)&&out.nodes.empty()&&!out.head&&!out.primary_infinite);}
    {Fixture f;f.pointer(204,200);check(!f.load(out)&&out.nodes.empty());}
    {Fixture f;f.pointer(200,800);check(!f.load(out)&&out.nodes.empty());}
    {Fixture f;f.light(300,14,0);check(!f.load(out)&&out.nodes.empty());}
    {Fixture f;f.pointer(600,800);check(!f.load(out)&&out.nodes.empty());}
    {Fixture f;f.word(604,0x7fc00000);check(!f.load(out)&&out.nodes.empty());}
    {Fixture f;f.bytes.resize(610);check(!f.load(out)&&out.nodes.empty());}
    {
        Fixture f;f.pointer(80,124);f.pointer(124,420);f.light(420,14,0);f.pointer(436,600);f.pointer(444,700);
        f.word(700,f32_to_bits(.9f));f.word(704,f32_to_bits(10));f.word(708,3);
        f.reader.block={f.bytes.data(),f.bytes.size()};std::sort(f.reader.fields.begin(),f.reader.fields.end());std::sort(f.reader.targets.begin(),f.reader.targets.end());
        HsdInitialCockpitLightInputs cockpit;std::string error;
        check(hsd_detail::read_initial_cockpit_light_inputs(f.reader,16,&cockpit,&error)&&cockpit.generation==1&&!cockpit.point_light_rendering_supported);
        check(cockpit.groups[0].nodes.size()==3&&cockpit.groups[1].nodes.size()==4&&cockpit.groups[2].nodes.size()==2&&cockpit.groups[3].nodes.size()==3);
        for(const auto& group:cockpit.groups)check(group.head==0&&!group.primary_infinite&&!group.animation_requested);
        const auto& point=cockpit.groups[3].nodes[2];check(point.light.flags==14&&point.point_parameters==std::array<u32,3>{f32_to_bits(.9f),f32_to_bits(10),3});
        //Stage's older entry remains ambient/infinite only.
        f.pointer(76,124);check(!f.load(out)&&out.nodes.empty());
    }
    std::printf("source light-root constructor/alias/retention: %u checks PASS\n",checks);
}
