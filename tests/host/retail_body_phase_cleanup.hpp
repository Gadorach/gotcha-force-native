// Continue the original phase-3 dispatcher without calling the disc backend.
#pragma once
#include "retail_body_entry_contact.hpp"
#include "retail_contact_effect_archive.hpp"
#include "retail_scene_entry.hpp"

namespace retail_body_phase_cleanup {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace archive=retail_contact_effect_archive;
inline constexpr u32 victim=contact::receiver,scene=contact::scene,seat=1,side=scene+60;
inline constexpr u32 wrapper=0x80054070,presentation=0x8005A580,loader=0x80041BF0,disc=0x8000B07C;
inline constexpr u32 phase3=0x8005514C,configure=0x80056CE0;
inline constexpr u32 view_count=3120,view=0x803BB4C0+view_count*seat,view_owner=view+0x600;
inline constexpr u32 pending_model=scene+16+2*seat,pending_color=scene+160+seat;
inline constexpr u32 ordinal=scene+84+seat,length=scene+90+seat,cursor=scene+5576+seat;
inline constexpr u32 queue=scene+488+840*seat,active_mask=scene+5593,active_count=scene+5594;
inline constexpr u32 resources=0x803C4D94+4*seat,slot_bank=0x803C4D7C+4*seat;
inline constexpr u32 repository=0x81060000,pools=0x81063000,header=0x81064000,pool=pools+28;
inline constexpr u32 source_basis=0x8043B5A0-21352,source_pool=0x8036F5D4;
inline constexpr u16 next=0x0106,terminal_model=0x0F07;
struct Replacement {
    std::array<u8,435104> motion{};
    std::array<u8,171236> view{};
};
enum class ReplacementProbe {constructor, entry, scene, contact};
struct Delivery {
    const Replacement& assets;
    const contact::ActorSpec& spec;
    const contact::HitBlock& hits;
    ReplacementProbe probe=ReplacementProbe::constructor;
    const retail_body_entry_contact::Inputs* contact_inputs=nullptr;
};

class Probe {
    const Module& mod;
    std::vector<u8> ram;
    const Delivery* delivery;
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    unsigned byte(u32 a){return at(a)[0];}
    unsigned body(unsigned off){return byte(victim+off);}
    void set(u32 a,unsigned x){at(a)[0]=u8(x);}
    void store(u32 a,u32 x){put32(at(a),x);}
    void short_store(u32 a,u16 x){put16(at(a),x);}
    u32 call(u32 entry,u32 arg=0){
        CPUState cpu{};cpu.gpr[3]=arg;
        mod.run(cpu,ram,entry,4096);
        return cpu.gpr[3];
    }
    u32 heap_limit()const{return delivery?archive::arena+0x100000:archive::arena_end;}
    void tick(){
        CPUState cpu{};cpu.gpr[3]=victim;
        CallObservation update{phase3};
        const auto cut=mod.run_to(cpu,ram,wrapper,{presentation,loader,disc},&update);
        if(cut!=presentation||cpu.gpr[3]!=victim||cpu.lr!=0x80054144||update.count!=1||update.arguments[0]!=victim||
           update.caller!=0x800540C8)
            fail("phase-3 actor did not use the original wrapper dispatch");
    }
    void prepare(unsigned kind){
        if(!body(0)||body(0x18)!=3||body(0x19)||body(0x3E4)!=seat||body(0x3E7)!=seat||body(0x3E6)||
           half(victim+0x1C6)||half(victim+0x20C)!=2||word(scene+0xB8)!=victim||word(side+0x118))
            fail("phase-3 inlet did not retain the accepted normal body");
        set(active_mask,3);set(active_count,2);
        set(length,2);set(cursor,1);set(ordinal,0);
        set(victim+0x491,kind?2:0);set(victim+0x492,0);
        short_store(queue,contact::sasuke.model);short_store(queue+28,next);
        short_store(pending_model,contact::sasuke.model);
        set(pending_color,0);
        set(scene+203+seat,1);
        if(kind==2){
            set(side+246,1);
            set(word(0x8043B5A0-31856)+50,1); // Declared sentinel branch after original mode-0 KO.
        }
    }
    void install_empty_owner(bool initialize){
        if(initialize){
            archive::Setup setup(mod,ram);
            setup.init_heap(heap_limit());
            store(source_basis,repository);store(0x8043B5A0-21024,pools);
            store(pool,header-repository);store(pool+4,32);
            std::memcpy(at(pool+16),at(source_pool+16),12);
            call(0x80197950,1);
        }
        if(call(0x80197A0C,1)!=header||word(pool+8)!=header||word(pool+12)||half(pool+18)||
           byte(header+9)!=1||!word(header+12)||half(header+26))
            fail("phase-3 source owner did not allocate its empty archive context");
        store(view_owner,header);
    }
    bool full_hp_accounting() {
        return !half(victim+0x1C6)&&half(victim+0x20C)==2&&word(victim+0x41C)==16&&
            word(side+0x118)==0&&body(0x435)==1;
    }
    void request(unsigned caster,unsigned kind){
        const auto prepared=ram;
        const auto wanted=kind==2?terminal_model:next;
        CPUState selected{};selected.gpr[3]=victim;
        mod.run_to(selected,ram,wrapper,{loader});
        if(selected.lr!=0x80055344||selected.gpr[3]!=victim||half(selected.gpr[4])!=wanted||
           selected.gpr[5]||half(pending_model)!=wanted||byte(pending_color)||body(0x19)!=1||
           half(victim+0x1C)!=60||!body(0)||word(view_owner)!=header)
            fail("phase-3 model request crossed clock, identity or resource ownership");
        std::memcpy(ram.data(),prepared.data(),ram.size());
        CPUState cut{};cut.gpr[3]=victim;
        mod.run_to(cut,ram,wrapper,{disc});
        if(cut.lr!=0x80041EE4||cut.gpr[4]!=0x803B07AC+56||word(view_owner)||word(resources)||word(slot_bank)||
           word(pool+8)||word(pool+12)!=header||half(pool+18)!=1||byte(header+9)||
           word(archive::arena+4)!=archive::block||word(archive::arena+8)||
           word(archive::block+8)!=heap_limit()-archive::block||
           half(pending_model)!=wanted||body(0x19)!=1||!body(0)||half(victim+0x1C)!=60||
           byte(cursor)!=(kind==2?1:0)||byte(ordinal)!=(kind==2?0:1)||
           byte(queue+22)!=unsigned(kind==0)||byte(queue+24)!=unsigned(kind==0)||
           byte(queue+28+23)!=unsigned(kind==0)||!full_hp_accounting())
            fail("phase-3 original empty resource release or queue preparation changed");
        if(!delivery)std::printf("body-cleanup-request: %u %u %04X %u %u %u %u %u %u %u %u %u %u\n",
                    caster,kind,half(pending_model),cut.gpr[3],byte(cursor),byte(ordinal),byte(queue+22),byte(queue+24),
                    byte(queue+28+23),half(pool+18),body(0),body(0x19),half(victim+0x1C));
    }
    void reentry(unsigned caster){
        if(word(view_owner))fail("phase-zero readiness control already had an owner");
        CPUState ready{};ready.gpr[3]=victim;
        mod.run_to(ready,ram,wrapper,{presentation});
        if(ready.lr!=0x80054144||body(0x18)||!body(0)||half(victim+0x1C6))
            fail("phase-zero actor admitted without its model resource");
        install_empty_owner(false);
        const auto generation=body(0x3F0);
        CPUState enter{};enter.gpr[3]=victim;
        mod.run_to(enter,ram,wrapper,{configure});
        if(enter.lr!=0x80054328||enter.gpr[3]!=victim||body(0x18)!=1||body(0x19)||
           body(0x3F0)!=generation+1||half(victim+0x3E8)!=next||half(victim+0x1C6)||word(view_owner)!=header)
            fail("phase-zero owner gate did not reach the original next-model setup cut");
        std::printf("body-cleanup-ready: %u %u %04X %u %u\n",caster,body(0x18),half(victim+0x3E8),
                    body(0x3F0)-generation,half(victim+0x1C6));
    }
    void entry_gates(unsigned caster,u32 fresh_shape,u32 stride){
        constexpr u32 pad=0x803C727C+32*seat,poll_input=0x80056900,bomb_request=0x800E058C;
        constexpr unsigned body_steps=0x3EF0;
        const auto start=ram;
        const bool contacts=delivery->probe==ReplacementProbe::contact;
        if(contacts&&!delivery->contact_inputs)fail("missing exact replacement contact inputs");
        const auto rebuild=[&](){call(0x8008A28C);call(0x80089AEC);};
        const auto registry=[&](std::vector<u8>& snapshot,unsigned expected){
            const auto pos=[&](u32 p){return be32(contact::at(snapshot,p));};
            const auto n=contact::half(snapshot,contact::receiver_count);
            if(n!=(expected?9:0))fail("replacement entry published the wrong receiving count");
            unsigned mask=0;
            for(unsigned i=0;i<n;++i){
                const auto cell=pos(contact::receiver_registry+4*i);
                if(cell<fresh_shape||cell>=fresh_shape+16*stride||(cell-fresh_shape)%stride||
                   pos(cell+0x20)!=victim)
                    fail("replacement entry published a foreign receiving cell");
                const auto step=(cell-fresh_shape)/stride;
                if((mask&(1u<<step))||pos(cell+0x28)!=pos(victim+0x3FC)+32+step*80)
                    fail("replacement entry changed a bound receiving descriptor");
                mask|=1u<<step;
            }
            if(mask!=expected)fail("replacement entry receiving steps changed");
        };
        const auto step=[&](unsigned phase,unsigned visit,bool expects_poll){
            CPUState cpu{};cpu.gpr[3]=victim;
            CallObservation poll{poll_input};
            const auto cut=mod.run_to_or_return(cpu,ram,wrapper,{presentation,0x80264290},&poll,65536);
            if(cut==0x80264290)
                std::fprintf(stderr,"entry assert caster=%u scene=%u visit=%u caller=%08X line=%u\n",
                             caster,phase,visit,cpu.lr-4,cpu.gpr[4]);
            if(cut!=presentation||cpu.lr!=0x80054144||cpu.gpr[3]!=victim||half(victim+0x1C6)!=220||
               half(victim+0x774)!=10||half(victim+0x784)!=3||half(victim+0x20C)||
               word(victim+0x41C)!=16||word(side+0x118)||body(0x18)!=1||
               poll.count!=unsigned(expects_poll)||(poll.count&&(poll.caller!=0x80054CD0||poll.arguments[0]!=victim)))
                fail("normal replacement entry changed the original actor, input, or resource frontier");
            return poll.count;
        };
        const auto pulse=[&](unsigned phase,unsigned visit,bool blocked){
            auto fork=ram;
            put32(contact::at(fork,pad),0x200);
            if(blocked)contact::at(fork,scene+0x1829)[0]=1;
            CPUState cpu{};cpu.gpr[3]=victim;
            CallObservation poll{poll_input};
            const auto cut=mod.run_to_or_return(cpu,fork,wrapper,{bomb_request,presentation,0x80264290},&poll,65536);
            const auto b=[&](u32 off){return be32(contact::at(fork,victim+off));};
            const auto stock=contact::half(fork,victim+0x774);
            const bool paid=cut==bomb_request;
            const bool polls=(phase==4&&visit==82)||(phase==5&&visit==162);
            if(paid!=(polls&&!blocked)||poll.count!=unsigned(polls)||cut!=(paid?bomb_request:presentation)||
               (paid&&(cpu.lr!=0x80130584||cpu.gpr[3]!=victim||cpu.gpr[4]!=2||cpu.gpr[5]!=10))||
               stock!=10-unsigned(paid)||b(0x5B4)!=(paid?0x200u:0)||b(0x5BC)!=(paid?0x200u:0)||
               b(0x5D4)!=(paid?0x60u:0))
                fail("original entry B press reached the wrong paid source request");
            std::printf("replacement-pulse: %u %u %u %u %u %08X %08X %08X %u %u %04X %08X\n",
                        caster,phase,visit,unsigned(blocked),poll.count,b(0x5B4),b(0x5BC),b(0x5D4),unsigned(paid),
                        stock,contact::half(fork,victim+0x272),cpu.lr);
        };
        for(unsigned phase:{4u,5u}){
            if(contacts&&phase==5)break;
            std::memcpy(ram.data(),start.data(),ram.size());
            set(scene+69,phase);
            set(scene+0x1829,0);
            store(pad,0);
            const auto observe=[&](unsigned visit,unsigned poll){
                std::printf("replacement-entry: %u %u %u %u %u %u %u %04X %08X %08X %u %u %u %u\n",
                            caster,phase,visit,body(0x18),body(0x19),body(0x1A),half(victim+0x1C),
                            half(victim+0x272),word(victim+0xD4),word(victim+0x720),poll,
                            half(contact::receiver_count),body(0x741),body(0x742));
            };
            if(!contacts)observe(0,0);
            unsigned previous=body(0x19)*256+body(0x1A),previous_mask=half(victim+0x272),previous_count=0;
            for(unsigned n=1;n<=(contacts?139u:160u);++n){
                if(!contacts&&(n==21||n==80||n==81||n==82)){
                    pulse(phase,n,false);
                    if(n==82&&phase==4)pulse(phase,n,true);
                }
                const auto polled=step(phase,n,phase==4&&n>=82);
                rebuild();
                registry(ram,phase==4&&n>=139?body_steps:0);
                if(contacts){
                    namespace incoming=retail_body_entry_contact;
                    if(incoming::receiving(ram,incoming::alternate_receive_count,incoming::alternate_receive_list,
                                           fresh_shape,stride)!=(n>=81?body_steps:0))
                        fail("original replacement alternate receiver chronology changed");
                    if(n==80||n==81||n==82||n==138||n==139)
                        for(unsigned r=0;r<incoming::routes.size();++r)
                            for(unsigned relation:{0u,1u})
                                incoming::observe(mod,ram,(*delivery->contact_inputs)[r],r,caster,n,relation,fresh_shape,stride);
                    continue;
                }
                const unsigned key=body(0x19)*256+body(0x1A),mask=half(victim+0x272),count=half(contact::receiver_count);
                if(n<=2||n==21||n==79||n==82||n==138||n==139||key!=previous||mask!=previous_mask||count!=previous_count||n==160)
                    observe(n,polled);
                previous=key;previous_mask=mask;previous_count=count;
                if(n==81&&phase==4){
                    auto control=ram;
                    put32(contact::at(control,victim+0x720),0);
                    for(u32 entry:{0x8008A28Cu,0x80089AECu}){
                        CPUState list{};
                        mod.run(list,control,entry,4096);
                    }
                    registry(control,body_steps);
                    std::printf("replacement-guard: %u %u %u %u\n",caster,phase,count,
                                contact::half(control,contact::receiver_count));
                }
            }
            if(phase==5){
                set(scene+69,4);
                const auto polled=step(phase,161,false);
                rebuild();
                registry(ram,body_steps);
                observe(161,polled);
                pulse(phase,162,false);
            }
        }
    }
    void scene_gates(unsigned caster,u32 fresh_shape,u32 stride){
        constexpr u32 pad=0x803C727C+32*seat,bomb=0x800E058C;
        namespace scheduler=retail_scene_entry;
        scheduler::prepare(ram);
        set(scene+0x1828,2); // No voiced-event allocation for this seat.
        set(scene+0x1829,0);
        store(pad,0);
        for(unsigned n=1;n<=174;++n){
            const auto marker=body(0x743);
            const auto before=byte(scene+1);
            if(before>=scheduler::handlers.size())fail("replacement scene substep left the tested startup");
            if(n==172||n==173||n==174){
                auto fork=ram;
                put32(contact::at(fork,pad),0x200);
                CPUState input{};
                const auto cut=mod.run_to_or_return(input,fork,scheduler::dispatch,{bomb,presentation,0x80264290},nullptr,65536);
                const bool fired=cut==bomb;
                const auto stock=contact::half(fork,victim+0x774);
                if(cut!=(fired?bomb:presentation)||fired!=(n==174)||(fired&&(input.lr!=0x80130584||
                   input.gpr[4]!=2||input.gpr[5]!=10))||(!fired&&input.lr!=0x80054144)||
                   input.gpr[3]!=victim||stock!=(fired?9:10))
                    fail("normal scene opening changed the actor's first paid B request");
                std::printf("scene-bridge-b: %u %u %u %u %04X %08X\n",caster,n,fired,stock,
                            contact::half(fork,victim+0x272),input.lr);
            }
            CPUState body_visit{};
            CallObservation pad_visit{0x80056900};
            const auto stop=mod.run_to_or_return(body_visit,ram,scheduler::dispatch,{presentation,0x80264290},&pad_visit,65536);
            if(stop==0x80264290)
                std::fprintf(stderr,"scene-actor assertion visit=%u caller=%08X line=%u\n",n,body_visit.lr-4,body_visit.gpr[4]);
            if(stop!=presentation||body_visit.lr!=0x80054144||body_visit.gpr[3]!=victim||
               half(victim+0x1C6)!=220||half(victim+0x774)!=10||word(victim+0x41C)!=16||
               half(victim+0x784)!=3||word(side+0x118)||half(victim+0x20C)||body(0x18)!=1||
               marker!=unsigned(n>80)||body(0x743)!=unsigned(n>=80)||
               byte(scene+1)!=(n<81?0u:n==81?1u:n<172?2u:3u)||byte(scene+69)!=(n<173?3u:4u)||
               body(0x19)!=(n<80?0u:n<173?1u:2u)||half(victim+0x272)!=(n<20?0xEF:n<173?0xC3:0)||
               word(victim+0x720)||pad_visit.count!=unsigned(n==174)||
               (pad_visit.count&&(pad_visit.caller!=0x80054CD0||pad_visit.arguments[0]!=victim)))
                fail("normal scene opening actor slice left its scoped path");
            call(0x8008A28C);call(0x80089AEC);
            if(half(contact::receiver_count)!=(n>=173?9:0))
                fail("normal scene opening changed the receiving shape publication");
            if(n>=173){
                unsigned steps=0;
                for(unsigned i=0;i<9;++i){
                    const auto cell=word(contact::receiver_registry+4*i);
                    if(cell<fresh_shape||cell>=fresh_shape+16*stride||(cell-fresh_shape)%stride||word(cell+0x20)!=victim)
                        fail("normal scene opening published a foreign receiving shape");
                    const auto index=(cell-fresh_shape)/stride;
                    if((steps&(1u<<index))||word(cell+0x28)!=word(victim+0x3FC)+32+80*index)
                        fail("normal scene opening changed a bound receiving descriptor");
                    steps|=1u<<index;
                }
                if(steps!=0x3EF0)fail("normal scene opening published different receiving steps");
            }
            if(n==1||n==20||n==79||(n>=80&&n<=83)||n>=171)
                std::printf("scene-bridge: %u %u %u %u %08X %u %u %d %u %04X %08X %u %u\n",
                            caster,n,marker,body(0x743),scheduler::handlers[before],byte(scene+69),byte(scene+1),
                            std::bit_cast<std::int16_t>(half(scene+8)),
                            body(0x19),half(victim+0x272),word(victim+0x720),pad_visit.count,
                            half(contact::receiver_count));
        }
    }
    void reentry_from_archive(unsigned caster){
        constexpr u32 ready=0x800412C4,dest=0x803B07AC+56;
        constexpr u32 role0=0x812C9400,role2=0x81765C00,role3=0x81547C00,role4=0x81323C00;
        if(word(dest)!=role0||word(dest+4)!=0xF0000000||word(dest+8)!=role2||word(dest+12)!=role3||
           word(dest+16)!=role4||word(dest+20)!=0xF0000000||word(view_owner)||half(pending_model)!=next){
            std::fprintf(stderr,"replacement roles %08X %08X %08X %08X %08X %08X owner=%08X model=%04X\n",
                         word(dest),word(dest+4),word(dest+8),word(dest+12),word(dest+16),word(dest+20),
                         word(view_owner),half(pending_model));
            fail("replacement archive destinations no longer name the requested normal model roles");
        }
        const auto free_before=half(contact::free_count+8);
        if(!body(0x1D8)||!byte(contact::body))fail("replacement did not retain the marked old receiving shape");
        call(0x8008A28C);call(0x80089AEC);
        if(byte(contact::body)||half(contact::free_count+8)!=free_before+1)
            fail("replacement original list walk did not return the marked old receiving shape");
        // The host supplies only the exact decompressed role bytes. The
        // original loader's disc call and async polling remain outside the run.
        std::memcpy(at(role0),delivery->spec.data(),delivery->spec.size());
        std::memcpy(at(role2),delivery->hits.data(),delivery->hits.size());
        std::memcpy(at(role3),delivery->assets.motion.data(),delivery->assets.motion.size());
        std::memcpy(at(role4),delivery->assets.view.data(),delivery->assets.view.size());
        const auto before=ram;
        const auto complete=[&](){
            CPUState callback{};callback.gpr[3]=seat;callback.gpr[4]=0;
            mod.run(callback,ram,ready,4096);
            if(word(resources)!=role0||word(scene+128+seat*4)!=role2||word(slot_bank)!=role3||
               word(0x803BB4A0+seat*4)!=role4)
                fail("replacement completion selected the wrong destination or seat");
        };
        short_store(pool+18,0); // No archive-owner allocation; raw publications alone are insufficient.
        complete();
        CPUState refused{};refused.gpr[3]=victim;
        mod.run_to(refused,ram,wrapper,{presentation,configure});
        if(refused.lr!=0x80054144||word(view_owner)||body(0x18)||!body(0)||!full_hp_accounting())
            fail("replacement admitted with raw roles but no archive owner");
        std::memcpy(ram.data(),before.data(),ram.size());
        complete();
        if(word(view_owner)!=header||byte(header+9)!=1||half(pool+18)||!word(header+12)||
           body(0x18)||half(victim+0x1C6)||half(victim+0x20C)!=2)
            fail("replacement completion did not publish only the new seat resource");

        // Preallocate the original view-wrapper class. The heap and the
        // SDK ID map are shared HSD machinery, not a reconstructed rig.
        constexpr u32 views=0x81066000,view_nodes=44*8;
        store(pools,views-repository);store(pools+4,view_nodes);
        std::memcpy(at(pools+16),at(source_pool-28+16),12);
        constexpr u32 clip_pool=pools+2*28,clips=0x81068000;
        store(clip_pool,clips-repository);store(clip_pool+4,16*8);
        std::memcpy(at(clip_pool+16),at(source_pool+28+16),12);
        call(0x80197950,0);call(0x80197950,2);call(0x80254288);
        const auto generation=body(0x3F0);
        at(word(0x8043B5A0-31856)+2)[0]=2; // Normal local-seat input from the mode descriptor.
        const auto start=ram;
        constexpr u32 shape=0x81070000,stride=0x200;
        for(const unsigned offered:{0u,16u}){
            if(delivery->probe!=ReplacementProbe::constructor&&!offered)continue;
            std::memcpy(ram.data(),start.data(),ram.size());
            // Hold the retired cells aside; each constructor gets only
            // the fresh, declared capacity of this fork.
            store(contact::free_head+16,0);store(contact::free_tail+16,0);short_store(contact::free_count+8,0);
            if(offered){
                for(unsigned i=0;i<offered;++i){
                    set(shape+i*stride+2,4);
                    store(shape+i*stride+4,i+1<offered?shape+(i+1)*stride:0);
                }
                store(contact::free_head+16,shape);store(contact::free_tail+16,shape+(offered-1)*stride);
                short_store(contact::free_count+8,u16(offered));
            }
            CPUState entry{};entry.gpr[3]=victim;
            CallObservation installer{0x801301F8};
            const auto cut=mod.run_to_or_return(entry,ram,wrapper,{0x80264290},&installer,1u<<20);
            if(cut)
                std::fprintf(stderr,"replacement SDK assertion from %08X at line %u\n",entry.lr-4,entry.gpr[4]);
            if(cut||body(0)!=1||body(0x18)!=1||body(0x3F0)!=generation+1||
               installer.count!=1||installer.caller!=0x80056D14||installer.arguments[0]!=victim||
               half(victim+0x3E8)!=next||word(victim+0x4AC)!=role0||word(victim+0x4C0)!=0x801304B8||
               word(victim+0x27C)!=0x80336340||half(victim+0x1C4)!=220||half(victim+0x1C6)!=220||
               half(victim+0x1C8)!=220||half(victim+0x774)!=10||half(victim+0x784)!=3||
               word(view_owner)!=header||word(header+20)!=role4+0x100||half(header+26)!=4||
               half(pools+18)||word(victim+0x3FC)!=role2)
                fail("replacement did not start its exact normal Time Bomber actor");
            for(unsigned i=0;i<48;++i)
                if(half(victim+0x20C+2*i))fail("replacement retained an old collision token");
            const auto a=word(view+1800),b=word(view+1928);
            if(a<views||b<views||a>=views+view_nodes||b>=views+view_nodes||a==b||
               !word(a+12)||!word(b+12)||word(a+12)==word(b+12))
                fail("replacement missing distinct source view graph allocations");
            for(unsigned i=0;i<offered;++i){
                const u32 cell=shape+i*stride;
                if(!byte(cell)||word(cell+0x20)!=victim||word(cell+0x24)!=victim||
                   word(cell+0x28)!=role2+32+i*80||byte(cell+0x12)!=i)
                    fail("replacement allocated the wrong receiving descriptors");
            }
            call(0x8008A28C);call(0x80089AEC);
            const auto count=half(contact::receiver_count);
            if(count>32)fail("replacement receiving registry extent");
            unsigned admitted=0,old=0;
            for(unsigned i=0;i<count;++i){
                const auto entry=word(contact::receiver_registry+4*i);
                admitted+=entry>=shape&&entry<shape+offered*stride;
                old+=entry==contact::body;
            }
            const auto mask=half(victim+0x272);
            if(mask!=0xEF||count||word(side+0x118)||word(victim+0x41C)!=16||half(contact::free_count+8))
                fail("replacement changed the initial receive mask, force, ledger or allocator boundary");
            if(delivery->probe==ReplacementProbe::entry||delivery->probe==ReplacementProbe::contact){
                entry_gates(caster,shape,stride);
                return;
            }
            if(delivery->probe==ReplacementProbe::scene){
                scene_gates(caster,shape,stride);
                return;
            }
            // Calculator-style negative control: remove only the receiving
            // role mask in this discarded state, not in the original actor visit.
            short_store(victim+0x272,mask&~2u);
            call(0x8008A28C);call(0x80089AEC);
            const auto unmasked_count=half(contact::receiver_count);
            unsigned admitted_steps=0;
            if(unmasked_count>offered)fail("replacement publishing crossed its declared cells");
            for(unsigned i=0;i<unmasked_count;++i){
                const auto ptr=word(contact::receiver_registry+4*i);
                if(ptr<shape||ptr>=shape+offered*stride||(ptr-shape)%stride)
                    fail("replacement published a receiving shape from outside its pool");
                const auto step=(ptr-shape)/stride;
                if(admitted_steps&(1u<<step)||word(ptr+0x28)!=role2+32+step*80)
                    fail("replacement receiving shape published twice or changed descriptors");
                admitted_steps|=1u<<step;
            }
            std::printf("body-replacement: %u %u %04X %u %u %u %u %u %04X %u %u %u %u %u %u %u %u %u %04X\n",
                        caster,offered,half(victim+0x3E8),body(0x18),body(0x3F0)-generation,half(victim+0x1C6),
                        half(victim+0x774),half(victim+0x784),mask,half(header+26),
                        view_nodes/44-half(pools+18),count,admitted,old,unsigned(!byte(contact::body)),
                        word(side+0x118),word(victim+0x41C),unmasked_count,admitted_steps);
        }
    }
public:
    Probe(const Module& m,const std::vector<u8>& data,unsigned caster,unsigned kind,const Delivery* input=nullptr):
        mod(m),ram(data),delivery(input){
        if(delivery&&kind!=0)fail("replacement only qualifies the normal queued request");
        prepare(kind);
        install_empty_owner(true);
        if(kind!=1)request(caster,kind);
        else tick();
        if(body(0x18)!=3||body(0x19)!=1||half(victim+0x1C)!=60||!body(0)||!full_hp_accounting())
            fail("phase-3 entry did not arm the 60-visit wait");
        // In the load cases these are declared separate callback visits
        // after the resource frontier. The unresolved disc proc is not resumed.
        for(unsigned n=0;n<2;++n){
            set(scene+31,1);
            tick();
            if(half(victim+0x1C)!=60||body(0x19)!=1||!body(0)||!full_hp_accounting())
                fail("phase-3 scene allow-mask advanced an excluded player");
        }
        for(unsigned n=1;n<=60;++n){
            set(scene+31,n==1?2:0);
            tick();
            if(half(victim+0x1C)!=60-n||!full_hp_accounting()||
               (n<60&&(!body(0)||body(0x18)!=3||body(0x19)!=1||byte(active_mask)!=3||byte(active_count)!=2)))
                fail("phase-3 wait did not count its allowed isolated visits");
        }
        const bool gone=kind==1;
        const auto wanted=kind==2?terminal_model:gone?contact::sasuke.model:next;
        if(body(0)!=unsigned(!gone)||body(0x18)!=(gone?3:0)||body(0x19)!=(gone?2:0)||
           byte(active_mask)!=(gone?1:3)||byte(active_count)!=(gone?1:2)||half(pending_model)!=wanted||
           word(view_owner)!=(gone?header:0)||word(pool+8)!=(gone?header:0)||half(pool+18)!=unsigned(!gone))
            fail("phase-3 expiry mixed active slot reuse and normal actor retirement");
        if(!delivery)std::printf("body-cleanup-expiry: %u %u %u %u %u %u %u %u %u %u %u\n",caster,kind,body(0),body(0x18),
                    body(0x19),byte(active_mask),byte(active_count),half(victim+0x20C),half(victim+0x1C),
                    unsigned(word(view_owner)!=0),half(pool+18));
        if(delivery)reentry_from_archive(caster);
        else if(!kind)reentry(caster);
    }
};
} // namespace retail_body_phase_cleanup
