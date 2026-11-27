// Private original module, synthetic actor/table/scene records.
#include "retail_recomp_fixture.hpp"

namespace {
using namespace retail_fixture;
constexpr u32 select_entry=0x80069DD0, scripted_entry=0x8006A3D0;
constexpr u32 config=0x81014000, spec=0x81015000, contexts=0x81016000, rows=0x81016100,
              pairs=0x81016200, settings=0x81017000, peer=0x8101A000;
constexpr unsigned group=2;
struct Case {
    const char* name;
    int mode, choice, sector;
    u32 flags=0;
    u8 stance=0, lower=0, charge=0;
    float charge_counter=0, flat_counter=0, x=0, z=0;
    u16 upper=0x6000;
    u8 mask=1;
    bool refused=false, explicit_refusal=false, script=false, first_disabled=false;
};
constexpr Case cases[]{
    {.name="disabled config", .mode=-1, .choice=0, .sector=0, .refused=true},
    {.name="invalid normal config mode", .mode=2, .choice=0, .sector=0, .refused=true},
    {"flat default", 0, 0, 0},
    {.name="flat leading counter write does not choose row five", .mode=0, .choice=0, .sector=0, .flat_counter=1},
    {.name="flat flag 0x80 chooses four", .mode=0, .choice=4, .sector=0, .flags=0x80},
    {.name="flat flag 0x20 and stance choose one", .mode=0, .choice=1, .sector=0, .flags=0x20, .stance=1},
    {.name="flat flag 0x20 then 0x40 chooses two", .mode=0, .choice=2, .sector=0, .flags=0x60},
    {.name="flat flag 0x200 chooses three", .mode=0, .choice=3, .sector=0, .flags=0x200},
    {.name="flat lower lane 4 selects the same row as 0x40", .mode=0, .choice=2, .sector=0, .lower=4},
    {.name="flat charge refuses unless row three", .mode=0, .choice=0, .sector=0,
     .charge_counter=1, .refused=true},
    {.name="flat charge exception", .mode=0, .choice=3, .sector=0, .flags=0x200, .charge_counter=1},
    {.name="flat bypass byte skips positive counter guard", .mode=0, .choice=0, .sector=0,
     .charge=1, .charge_counter=1},
    {"directional defaults to zero without a target", 1, 0, 0},
    {.name="directional flag 0x100 takes priority over 0x80", .mode=1, .choice=5, .sector=0, .flags=0x180},
    {.name="directional flag 0x80 chooses four", .mode=1, .choice=4, .sector=0, .flags=0x80},
    {.name="directional stance chooses two", .mode=1, .choice=2, .sector=0, .flags=0x20, .stance=1},
    {.name="directional 0x40 chooses three", .mode=1, .choice=3, .sector=0, .flags=0x60},
    {.name="directional lower lane 4 selects three before 0x10", .mode=1, .choice=3, .sector=0,
     .flags=0x10, .lower=4},
    {.name="directional 0x10 chooses one", .mode=1, .choice=1, .sector=0, .flags=0x10},
    {.name="directional charge refuses regardless of flags", .mode=1, .choice=0, .sector=0, .flags=0x180,
     .charge_counter=1, .refused=true},
    {.name="directional bypass byte skips positive counter guard", .mode=1, .choice=5, .sector=0, .flags=0x180,
     .charge=1, .charge_counter=1},
    {.name="directional signed positive angle middle band", .mode=1, .choice=4, .sector=1, .flags=0x80, .x=10},
    {.name="directional angle above upper threshold", .mode=1, .choice=4, .sector=2, .flags=0x80,
     .x=1, .z=-10, .upper=0x4001},
    {.name="directional signed negative angle middle band", .mode=1, .choice=4, .sector=3, .flags=0x80, .x=-10},
    {.name="scripted uses explicit choice and sector", .mode=1, .choice=5, .sector=3, .script=true},
    {.name="scripted flat ignores sector", .mode=0, .choice=4, .sector=3, .script=true},
    {.name="scripted entry does not test the normal disabled mode", .mode=-1, .choice=4, .sector=3, .script=true},
    {.name="explicit row refusal", .mode=0, .choice=4, .sector=0, .flags=0x80, .refused=true, .explicit_refusal=true},
    {"lane one leaves second handler alone", 0, 0, 0},
    {.name="lane three writes both handlers", .mode=0, .choice=0, .sector=0, .mask=3},
    {.name="lane two still writes first, not second", .mode=0, .choice=0, .sector=0, .mask=2},
    {.name="all-zero row still commits and attempts first handler", .mode=0, .choice=0, .sector=0, .mask=0},
    {.name="disabled first handler does not turn row into refusal", .mode=0, .choice=0, .sector=0, .first_disabled=true},
};

void prepare_scene(std::vector<u8>& ram){
    put32(ram.data()+0x8043B5A0-31852-GC_RAM_BASE,settings);
    ram[settings-GC_RAM_BASE+69]=5; // Early return from the unrelated sample/sound selector.
    for(unsigned i=0;i<6;++i)put32(ram.data()+0x803C4E84-GC_RAM_BASE+4*i,i?peer+0x1E00*(i-1):actor);
}

void check_retail_zero_rows(const Module& mod,const std::vector<u8>& original){
    // The checked atlas names both exact action profiles. Use their actual
    // DOL tables with synthetic actor state, not a rebuilt four-byte row.
    struct Profile {u32 cfg,context,selector;u8 group,choice;bool use_script;};
    constexpr Profile profiles[]{
        {0x80342AD8,0x80342D28,0x80342BF0,0,3,false}, // 0205 B-shot 3:0
        {0x802FFED8,0x80300128,0x802FFFF0,1,5,true},  // 0207 B-close 5:0
    };
    for(const auto& p:profiles){
        auto ram=original;prepare_scene(ram);
        auto* obj=ram.data()+actor-GC_RAM_BASE;
        put32(obj+0x4B0,p.cfg);put32(obj+0x4AC,spec);put32(obj+0x4EC,p.context);put32(obj+0x4F0,p.selector);
        put32(obj+0x5E0,0x40);obj[0x585]=p.group;
        obj[0x580]=99;obj[0x581]=98;obj[0x5DA]=97;obj[0x5DB]=96;
        CPUState cpu{};
        cpu.gpr[3]=actor;cpu.gpr[4]=p.group;cpu.gpr[5]=p.choice;
        mod.run(cpu,ram,p.use_script?scripted_entry:select_entry);
        if(cpu.gpr[3]!=1||obj[0x584]!=p.group||obj[0x586]!=p.choice||
           obj[0x580]||obj[0x581]||obj[0x5DA]!=1||obj[0x5DB]!=96||(be32(obj+0x5E0)&3))
            fail("retail zero-filled cell did not schedule first handler 1");
    }
}

void check_main_relay(const Module& mod,const std::vector<u8>& original){
    auto ram=original;
    auto* obj=ram.data()+actor-GC_RAM_BASE;
    // A bounded stand-in for an installed handler: the already-tested
    // phase gate returns -1 for a synthetic actor's zero counter.
    put32(obj+0x4B4,0x80066838);
    CPUState cpu{};
    cpu.gpr[3]=actor;
    mod.run(cpu,ram,0x80065DFC);
    if(cpu.gpr[3]!=0xFFFFFFFFu)fail("slot 61 did not invoke the installed handler");
}
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: action-selector-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    for(const auto& test:cases){
        auto ram=original;prepare_scene(ram);
        const auto at=[&](u32 va){return ram.data()+va-GC_RAM_BASE;};
        auto* obj=at(actor);
        put32(obj+0x4B0,config);put32(obj+0x4AC,spec);
        put32(obj+0x4EC,contexts);put32(obj+0x4F0,pairs);
        put32(at(contexts)+4*group,rows);
        at(config)[0xB8+group]=u8(test.mode);
        for(unsigned i=0;i<24;++i){
            at(rows)[4*i]=u8(i+1);at(rows)[4*i+1]=1;
            at(rows)[4*i+2]=u8(i+41);at(rows)[4*i+3]=u8(i+71);
            at(pairs)[2*i+2]=61;at(pairs)[2*i+3]=0;
        }
        at(pairs)[0]=1;at(pairs)[1]=1;
        const unsigned index=test.mode==1?unsigned(4*test.choice+test.sector):unsigned(test.choice);
        auto* chosen=at(rows)+4*index;
        chosen[1]=test.mask;
        if(test.explicit_refusal)chosen[0]=255;
        else if(!test.mask)put32(chosen,0);
        else if(test.first_disabled)at(pairs)[2*chosen[0]]=255;

        put32(obj+0x5E0,test.flags);
        obj[0x585]=group;obj[0x584]=77;obj[0x586]=66;obj[0x587]=55;obj[0x595]=test.charge;
        obj[0x7CC]=test.stance;obj[0x5E4]=test.lower;
        obj[0x580]=99;obj[0x581]=98;obj[0x5DA]=97;obj[0x5DB]=96;
        put_float(obj+0x694,test.charge_counter);put_float(obj+0x6A4,test.flat_counter);
        if(test.x||test.z){
            put32(obj+0x508,peer);put32(obj+0x504,peer);
            put_float(obj+0x50C,test.x);put_float(obj+0x514,test.z);
            for(unsigned i=0;i<3;++i)put_float(obj+0x8D4+4*(5*i),1.f);
            put16(at(spec)+0x158,0x2000);put16(at(spec)+0x15A,0xE000);
            put16(at(spec)+0x15C,test.upper);put16(at(spec)+0x15E,0xA000);
        }
        CPUState cpu{};
        cpu.gpr[3]=actor;cpu.gpr[4]=group;
        cpu.gpr[5]=test.choice;cpu.gpr[6]=test.sector;
        mod.run(cpu,ram,test.script?scripted_entry:select_entry);
        const auto state=test.refused?99:chosen[2],substate=test.refused?98:chosen[3];
        const auto first=test.refused||test.first_disabled?97:at(pairs)[2*chosen[0]];
        const auto second=test.refused||test.mask!=3?96:at(pairs)[2*chosen[0]+1];
        if(cpu.gpr[3]!=(!test.refused)||obj[0x584]!=(test.refused?77:group)||
           obj[0x580]!=state||obj[0x581]!=substate||obj[0x5DA]!=first||obj[0x5DB]!=second||
           (!test.refused&&obj[0x586]!=test.choice)||
           (!test.refused&&obj[0x587]!=((test.mode==1)?test.sector:55))||
           (!test.refused&&(be32(obj+0x5E0)&3)!=test.mask)){
            std::fprintf(stderr,"%s: ret=%u group=%u choice=%u sector=%u state=%u:%u slots=%u:%u mask=%u\n",
                         test.name,cpu.gpr[3],obj[0x584],obj[0x586],obj[0x587],obj[0x580],obj[0x581],
                         obj[0x5DA],obj[0x5DB],be32(obj+0x5E0)&3);
            return 1;
        }
    }
    check_retail_zero_rows(mod,original);
    check_main_relay(mod,original);
    std::printf("action selector: %zu synthetic lookup cases, 2 original-table cases and slot 61 passed\n",
                sizeof(cases)/sizeof(cases[0]));
}
