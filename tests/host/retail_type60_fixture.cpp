// Original type-60 release, shape bind and recipient-effect dispatch.
#include "retail_stationary_contact.hpp"
#include <cstdint>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
constexpr u32 carrier=0x81030000,effect=0x81030800,shape=0x81032000,descriptors=0x81033000,
              pool=0x81033800,hit_block=0x81034000,stored_target=0x8101A000;
constexpr u32 factory=0x8014452C,bind=0x8008AC80,effect_factory=0x80176EC0;
constexpr u32 kind_free_head=0x803C716C,kind_free_tail=0x803C718C,kind_free_count=0x803C71AC;
constexpr u32 attack_count=0x8043B5A0-21328,attack_list=0x803C477C;
struct Route {unsigned model,key,index,hp;u32 release,site,table;};
constexpr Route routes[]{
    {0x0902,0,5,150,0x8016027C,0x801602FC,0x80348910},
    {0x0909,2,6,180,0x8016027C,0x8016030C,0x80348910},
    {0x0907,1,5,150,0x801B6868,0x801B68E8,0x80381448},
    {0x090C,3,6,180,0x801B6868,0x801B68F8,0x80381448},
};
// Same-source controls for release, type-60 shape and type-33 resources.
struct Case {bool cue,stock,object,shape,overlap,ally,fx;unsigned descriptors;};
constexpr Case cases[]{
    {true,true,true,true,true,false,true,7},
    {true,true,true,true,true,true,true,7},
    {true,true,true,true,false,false,true,7},
    {true,true,true,true,true,false,false,7},
    {true,true,true,true,true,false,true,6},
    {true,true,true,false,true,false,true,7},
    {true,true,false,true,true,false,true,7},
    {true,true,true,true,true,false,true,3},
    {true,false,true,true,true,false,true,7},
    {false,true,true,true,true,false,true,7},
};
constexpr contact::Point origin{200.f,150.f,325.f};
constexpr contact::StationaryBody large_body{0x0602,1050,9,2,0x8030C570,{0,-40,20}};
constexpr u32 delta_bank=0x8043612C,scope_case_count=5;

void scale_scope(const Module& mod,std::vector<u8>& ram,Route r,unsigned mode,bool have_fx,unsigned hp){
    constexpr u32 recipient=contact::receiver,slot_delta=delta_bank+1;
    constexpr u32 prelude=0x8002BB14,reaction=0x8005C290,frontier=0x8005A580,apply=0x800681E4,step=0x8005BAB8;
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto signed_byte=[](u8 byte){return int(std::bit_cast<std::int8_t>(byte));};
    constexpr u32 rank=recipient+0x74A,clamp=rank+1,stored=rank+4,timer=rank+6;
    const int expected_delta=mode==2?0:(r.key%2?-1:1)*(r.key>=2?2:1);
    const unsigned start_level=at(rank)[0];
    const auto start=word(recipient+0xB4);

    // Fork at the same published lists: first observe the pair-only writes,
    // then run the original scene prelude from before those writes.
    auto isolated=ram;
    const u32 factory_entry=mode==2?0x800DB250u:effect_factory;
    CPUState pair{};CallObservation request{factory_entry};
    mod.run_observed(pair,isolated,contact::pair_pass,request,4096);
    const auto one=[&](u32 a){return contact::at(isolated,a);};
    const unsigned flags=one(recipient+0x1DA)[0];
    const unsigned expected_fx=have_fx?(mode==2?15:33):0;
    const auto request_site=mode==2?0x8002F698u:(r.key%2?0x8002F798u:0x8002F848u);
    if(request.count!=1||request.arguments[0]!=recipient||request.caller!=request_site||
       bool(one(effect)[0])!=have_fx||one(effect+0x10)[0]!=expected_fx||
       signed_byte(one(slot_delta)[0])!=expected_delta||one(delta_bank)[0]||one(slot_delta+1)[0]||
       (flags&0x60)!=(expected_delta?(r.key%2?0x40u:0x20u):0)||
       be32(one(recipient+0xB8))!=start||be32(one(recipient+0xB4))!=start||
       contact::half(isolated,timer)||contact::half(isolated,recipient+0x1C6)!=hp){
        std::fprintf(stderr,"type60-scale pair %04X/%u calls=%u recipient=%08X effect=%u/%u delta=%d flags=%02X scale=%08X/%08X timer=%u hp=%u\n",
                     r.model,mode,request.count,request.arguments[0],unsigned(bool(one(effect)[0])),one(effect+0x10)[0],signed_byte(one(slot_delta)[0]),
                     flags,be32(one(recipient+0xB4)),be32(one(recipient+0xB8)),contact::half(isolated,timer),
                     contact::half(isolated,recipient+0x1C6));
        fail("type-60 pair failed to separate its queued scale delta from effect or HP");
    }

    // The receiver is the first active record in the scene scan. The caster
    // and the stored target remain outside that contiguous scan.
    for(unsigned i=0;i<6;++i)put32(at(0x803C4E84+4*i),recipient-0x1E00+i*0x1E00);
    CPUState scene{};
    CallObservation consume{apply};
    const auto cut=mod.run_to(scene,ram,prelude,{reaction,frontier},&consume);
    if((cut==frontier?scene.lr!=0x8002C870:scene.lr!=0x8002C6C4)||scene.gpr[3]!=recipient||
       consume.count!=unsigned(expected_delta!=0)||
       (consume.count&&(consume.count!=1||consume.caller!=0x8002C5D0||consume.arguments[0]!=recipient||
                       static_cast<int>(std::bit_cast<std::int32_t>(consume.arguments[1]))!=expected_delta))||
       signed_byte(at(slot_delta)[0])!=expected_delta||half(recipient+0x1C6)!=hp||
       bool(at(effect)[0])!=have_fx||at(effect+0x10)[0]!=expected_fx)
        fail("type-60 scene did not consume its own original contact");
    const int level=signed_byte(at(rank)[0]),effective=signed_byte(at(clamp)[0]),offset=signed_byte(at(stored)[0]);
    const unsigned visits=half(timer);
    const u32 after=word(recipient+0xB4),target=word(recipient+0xB8),motion=word(recipient+0x1DD0),derived=word(recipient+0x5F8);
    if(after!=start||offset!=expected_delta||visits!=(expected_delta?900u:0u))
        fail("type-60 scene changed applied scale before the scale callback");

    u32 first=after,settled=after,expired=after,restored=after;
    unsigned before_expiry=0;
    if(visits){
        for(unsigned n=1;n<=visits+40;++n){
            CPUState tick{};tick.gpr[3]=recipient;
            mod.run(tick,ram,step);
            if(n==1)first=word(recipient+0xB4);
            if(n==visits-1){before_expiry=half(timer);settled=word(recipient+0xB4);}
            if(n==visits)expired=word(recipient+0xB4);
        }
        restored=word(recipient+0xB4);
        if(before_expiry!=1||settled!=target||restored!=start||at(stored)[0]||half(timer)||at(rank)[0]!=start_level||
           word(recipient+0xB8)!=start||word(recipient+0x58)!=restored||
           half(recipient+0x1C6)!=hp||bool(at(effect)[0])!=have_fx)
            fail("type-60 scale expiry or ramp changed");
    }
    std::printf("type60-scale: %u %u %u %u %d %u %d %d %d %u %08X %08X %08X %08X %08X %08X %08X %08X\n",
                r.model,mode,hp,expected_fx,expected_delta,consume.count,level,effective,offset,visits,
                start,target,motion,derived,first,settled,expired,restored);
}

void check(const Module& mod,std::vector<u8> ram,Route r,unsigned id,
           const contact::HitBlock& hurt,const contact::ActorSpec& receiver_spec,
           const contact::HitBlock& hit,const contact::ActorSpec& actor_spec,int mode=-1){
    const Case c=cases[id];
    const auto at=[&](u32 a){return contact::at(ram,a);};
    const auto word=[&](u32 a){return be32(at(a));};
    const auto half=[&](u32 a){return contact::half(ram,a);};
    const auto scalar=[&](u32 a){return std::bit_cast<float>(word(a));};
    const auto point=[&](u32 a){return contact::Point{scalar(a),scalar(a+4),scalar(a+8)};};
    const auto offer=[&](unsigned kind,u32 cell,bool present){
        at(cell)[2]=kind;
        put32(at(kind_free_head+4*kind),present?cell:0);
        put32(at(kind_free_tail+4*kind),present?cell:0);
        put16(at(kind_free_count+2*kind),u16(present));
    };
    put32(at(0x80436380),pool);
    put32(at(pool+12),descriptors);
    put16(at(pool+18),c.descriptors);
    for(unsigned i=0;i<c.descriptors;++i)
        put32(at(descriptors+i*0x80),i+1<c.descriptors?descriptors+(i+1)*0x80:0);
    auto target=origin;if(!c.overlap)target[0]+=300.f;
    const auto body=mode==2?large_body:contact::sasuke;
    contact::bind_stationary(mod,ram,target,hurt,actor_spec,receiver_spec,r.hp,body);
    if(mode>=0){
        put16(at(contact::receiver+0x59C),u16(u16(receiver_spec[0xA8])<<8|receiver_spec[0xA9]));
        put16(at(contact::receiver+0x59E),u16(u16(receiver_spec[0xAA])<<8|receiver_spec[0xAB]));
        at(contact::receiver+0x74A)[0]=mode==3?receiver_spec[0xA1]:16;
        at(contact::receiver+0x74C)[0]=receiver_spec[0xA2];
        at(contact::receiver+0x74D)[0]=receiver_spec[0xA1];
        CPUState select{};select.gpr[3]=contact::receiver;
        mod.run(select,ram,0x8006826C,1024);
        const auto initial=word(contact::receiver+0xB8);
        for(unsigned off:{0x58u,0x5Cu,0x60u,0xB4u,0xC0u,0xC4u})put32(at(contact::receiver+off),initial);
    }
    put32(at(0x803C4E84),actor);
    for(unsigned i=2;i<6;++i)put32(at(0x803C4E84+4*i),stored_target);
    at(actor)[0]=1;at(actor+0x18)[0]=1;
    at(contact::receiver+0x88)[0]=c.ally||mode==4?0:1;
    put16(at(actor+0x3E8),r.model);put16(at(actor+0x94),r.model);
    put32(at(actor+0xCC),stored_target);
    put32(at(actor+0x27C),r.table);put32(at(actor+0x3FC),hit_block);
    std::memcpy(at(hit_block),hit.data(),hit.size());
    constexpr u32 joint=actor+0x8D4+48*7;
    for(unsigned i=0;i<3;++i){
        put_float(at(joint+20*i),1.f);
        put_float(at(joint+12+16*i),origin[i]+(i==0?10.f:i==2?-90.f:0.f));
    }
    put_float(at(actor+0xB8),1.f);put_float(at(actor+0xC0),1.f);
    at(actor+0x540)[0]=2;
    at(actor+0x1CEF)[0]=1;at(actor+0x1CF0)[0]=c.cue?1:3;
    put16(at(actor+0x784),c.stock?1:0);put16(at(actor+0x786),7);
    put16(at(actor+0x7A2),120);put_float(at(actor+0x788),17.f);
    offer(2,effect,c.fx);offer(3,carrier,c.object);offer(4,shape,c.shape);

    const bool requested=c.cue&&c.stock,made=requested&&c.object&&c.descriptors>=4;
    CPUState release{};release.gpr[3]=actor;
    CallObservation cast{factory};
    const auto tail=mod.run_to_or_return(release,ram,r.release,{0x8006D144},&cast,4096);
    if(tail!=0x8006D144||cast.count!=unsigned(requested)||
       (requested&&(cast.caller!=r.site||cast.arguments[0]!=actor||cast.arguments[1]!=r.key))||
       at(actor+0x540)[0]!=(c.cue?3:2)||at(actor+0x1CEF)[0]!=(c.cue?0:1)||at(actor+0x1CF0)[0]!=(c.cue?0:3)||
       half(actor+0x784)!=unsigned(c.stock&&!requested)||half(actor+0x786)!=(requested?1:7)||
       scalar(actor+0x788)!=(requested?120.f:17.f)||bool(at(carrier)[0])!=made||
       half(kind_free_count+6)!=unsigned(c.object&&!made)||half(pool+18)!=c.descriptors-4*unsigned(made))
        fail("type-60 cue, debit or carrier allocation changed");
    if(made&&(at(carrier+0x10)[0]!=60||at(carrier+0x11)[0]!=r.key||at(carrier+0x18)[0]||
              word(carrier+0x8C)!=actor||word(carrier+0x90)!=actor||word(carrier+0xCC)!=stored_target))
        fail("type-60 carrier ownership changed");

    bool published=false;
    unsigned effect_calls=0,effect_key=0,effect_arg=0,amount_calls=0,hp_writes=0,hp=180,link=0,repeat=0;
    if(made){
        CPUState init{};init.gpr[3]=carrier;
        CallObservation group{bind};
        const auto cut=mod.run_to_or_return(init,ram,0x80144634,{0x80089100},&group,4096);
        if(cut!=(c.shape?0x80089100:0)||group.count!=1||group.caller!=0x801446C4||
           group.arguments[0]!=carrier||group.arguments[1]!=6||at(carrier+0x18)[0]!=(c.shape?1:2)||
           (c.shape&&(point(carrier+0x20)!=origin||half(carrier+0x1C)!=480||
                      word(shape+0x20)!=carrier||word(shape+0x24)!=actor||
                      word(shape+0x28)!=hit_block+0xAA0+80*7||word(shape+0x2C)!=r.table||at(shape+0x11)[0]!=r.index))){
            std::fprintf(stderr,"model=%04X case=%u cut=%08X bind=%u/%08X/%08X/%u phase=%u life=%u root=%g/%g/%g shape=%08X/%08X/%08X/%08X/%u\n",
                         r.model,id,cut,group.count,group.caller,group.arguments[0],group.arguments[1],at(carrier+0x18)[0],
                         half(carrier+0x1C),double(scalar(carrier+0x20)),double(scalar(carrier+0x24)),double(scalar(carrier+0x28)),
                         word(shape+0x20),word(shape+0x24),word(shape+0x28),word(shape+0x2C),at(shape+0x11)[0]);
            fail("type-60 opening bind or provenance changed");
        }
        if(c.shape){
            CPUState matrix{};matrix.gpr[3]=carrier;
            mod.run(matrix,ram,0x80047AA4,1024);
        }
        for(u32 entry:{0x8008A28Cu,0x80089AECu}){
            CPUState publish{};mod.run(publish,ram,entry,1024);
        }
        published=half(attack_count)!=0;
        if(half(attack_count)!=unsigned(c.shape)||half(contact::receiver_count)!=1||
           (published&&(word(attack_list)!=shape||point(shape+0x64)!=origin||scalar(shape+0x7C)!=50.f)))
            fail("type-60 shape did not publish in the ordinary list");
        constexpr u32 audio=0x8103E000;
        put32(at(0x8043B5A0-31856),audio);at(audio+12)[0]=1;
        at(0x803D0000+15080+852)[0]=1;
        if(mode>=0){
            scale_scope(mod,ram,r,unsigned(mode),c.fx,body.hp);
            return;
        }
        auto audit=ram;
        CPUState hp_probe{};
        CallObservation amount{contact::amount_entry};
        hp_writes=mod.run_to_or_return(hp_probe,audit,contact::pair_pass,{contact::hp_write},&amount,4096)!=0;
        if(hp_writes&&(hp_probe.lr!=0x8002F1DC||hp_probe.gpr[3]!=contact::receiver||hp_probe.gpr[4]))
            fail("type-60 checked HP writer request was not zero");
        if(amount.count&&(amount.count!=1||amount.caller!=0x8002F108||
                         amount.arguments[0]!=r.table+24*r.index||amount.arguments[1]!=carrier||
                         amount.arguments[2]!=contact::receiver))
            fail("type-60 pair selected an unexpected amount record");
        amount_calls=amount.count;
        CPUState pair{};CallObservation fx{effect_factory};
        mod.run_observed(pair,ram,contact::pair_pass,fx,4096);
        if(fx.count&&(fx.count!=1||fx.arguments[0]!=contact::receiver||
                      fx.caller!=(r.key%2?0x8002F798u:0x8002F848u)))
            fail("type-60 contact did not give its effect to the struck actor");
        effect_calls=fx.count;effect_arg=fx.arguments[1];effect_key=fx.arguments[2];
        hp=half(contact::receiver+0x1C6);
        link=word(carrier+0x290)==contact::receiver;
        if(word(contact::receiver+0x41C)||word(actor+0x418)||word(actor+0x42C)||half(actor+0x1C6)!=r.hp)
            fail("type-60 contact changed the tested fighter ledgers");
    }
    const bool spawned=at(effect)[0]!=0;
    const unsigned remaining=half(pool+18);
    if(spawned&&(at(effect+0x10)[0]!=33||at(effect+0x11)[0]!=effect_key||at(effect+0x12)[0]!=effect_arg||
                 word(effect+0x8C)!=contact::receiver||word(effect+0x90)!=contact::receiver))
        fail("type-60 contact effect header changed");
    if(effect_calls){
        if(!spawned){
            // Offer the initially unavailable effect resources, without a
            // fresh carrier or token. No queued retry is assumed.
            offer(2,effect,true);
            put32(at(pool+12),descriptors+4*0x80);put16(at(pool+18),3);
            for(unsigned i=4;i<7;++i)put32(at(descriptors+i*0x80),i<6?descriptors+(i+1)*0x80:0);
        }
        CPUState again{};
        CallObservation retry{effect_factory};
        mod.run_observed(again,ram,contact::pair_pass,retry,4096);
        if(retry.count||bool(at(effect)[0])!=spawned||half(contact::receiver+0x1C6)!=hp)
            fail("type-60 retained pair repeated its effect request");
        repeat=1;
    }
    std::printf("type60: %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u %u\n",r.model,id,
                unsigned(requested),unsigned(made),unsigned(published),effect_calls,effect_arg,effect_key,unsigned(spawned),
                hp,link,amount_calls,hp_writes,remaining,half(contact::receiver+0x20C),repeat);
}
}

int main(int argc,char** argv){
    const bool scale=argc==4&&!std::strcmp(argv[3],"--scale");
    if(argc!=3&&!scale)fail("usage: type60-fixture qualified.dol unmodified-recomp.so [--scale]");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    const auto read=[](auto& row){
        if(std::fread(row.data(),1,row.size(),stdin)!=row.size())fail("type-60 input extent");
    };
    contact::HitBlock hurt{},hit{};contact::ActorSpec receiver{},owner{};
    contact::HitBlock large_hurt{};contact::ActorSpec large_spec{};
    read(hurt);read(receiver);
    if(scale){read(large_hurt);read(large_spec);}
    for(const auto r:routes){
        read(hit);read(owner);
        if(scale){
            for(unsigned i=0;i<scope_case_count;++i)
                check(mod,original,r,i==1?3:0,i==2?large_hurt:hurt,i==2?large_spec:receiver,hit,owner,int(i));
        }else{
            for(unsigned i=0;i<std::size(cases);++i)check(mod,original,r,i,hurt,receiver,hit,owner);
        }
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected type-60 payload");
}
