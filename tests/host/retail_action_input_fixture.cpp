// Original processed pad packet -> conditioned requests -> action arbiter.
// The scene, actor flags, and missing services are fixture inlets, not a match.
#include "retail_recomp_fixture.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace {
using namespace retail_fixture;
constexpr u32 packet=0x803C727C, arbiter=0x800699D8;
constexpr u32 config=0x81014000, spec=0x81015000, contexts=0x81016000, rows=0x81016100,
              pairs=0x81016200, settings=0x81017000, mode_state=0x81019200, peer=0x8101A000;
constexpr u32 b=0x200, x=0x400, y=0x800;
constexpr u32 descriptor_row=0x81030000, descriptor_cell=0x81030100,
              object_cell=0x81032000, hurt_cell=0x81032800, hit_block=0x81036000, hurt_block=0x81038000;
constexpr u32 free_head=0x803C716C, free_tail=0x803C718C, free_count=0x803C71AC,
              group_head=0x803C6F6C, group_tail=0x803C706C;
constexpr u32 attack_registry=0x803C477C, receiver_registry=0x803C417C,
              attack_count=0x8043B5A0-21328, receiver_count=0x8043B5A0-21330;
using HitBlock=std::array<u8,7840>;
using ActorSpec=std::array<u8,432>;
struct HpInlet {
    const ActorSpec& actor_spec;
    u16 receiver_hp=180;
    bool protect_hp=false;
    u8 settings_index=0,receiver_source=0;
};
struct Profile {u32 cfg,context,selector;};
constexpr Profile synthetic{config,contexts,pairs};
constexpr Profile red{0x8030A130,0x8030A380,0x8030A248}; // Machine Red, p076
constexpr Profile sasuke{0x802D3DC8,0x802D4148,0x802D3FF8}; // Sasuke, p012
constexpr Profile black[]{
    {0x80365AC8,0x80365EA8,0x80365CF8}, // G-Black, p088
    {0x80365BE0,0x80365EA8,0x80365CF8}, // G-Black, p089
};

class Check {
    const Module& mod;
    std::vector<u8> ram;
    u8* at(u32 va){
        if(va<GC_RAM_BASE||va-GC_RAM_BASE>ram.size()-4)fail("fixture address outside RAM");
        return ram.data()+va-GC_RAM_BASE;
    }
    u16 half(u32 va){const auto* p=at(va);return u16(u16(p[0])<<8|p[1]);}
    float fp(u32 va){return std::bit_cast<float>(be32(at(va)));}
    u32 call(u32 entry){
        CPUState cpu{};
        cpu.gpr[3]=actor;
        mod.run(cpu,ram,entry);
        return cpu.gpr[3];
    }
    u32 relative_table(u32 root,u32 index){
        if(root<GC_RAM_BASE||root>=0x80440000||index>=128)fail("unsupported program table operand");
        const auto* p=at(root+2*index);
        const auto relative=std::bit_cast<int16_t>(u16(u16(p[0])<<8|p[1]));
        return root+u32(relative);
    }
    void pool(u8 kind,bool object,bool descriptor){
        // Only the free-list inputs are supplied; original pop/clear/tag
        // and active-list publication must follow in the guest code.
        put32(at(0x80436380),descriptor_row);
        put16(at(descriptor_row+18),descriptor?1:0);
        put32(at(descriptor_row+12),descriptor?descriptor_cell:0);
        if(object){
            at(object_cell)[2]=kind;
            put32(at(free_head+4*kind),object_cell);
            put32(at(free_tail+4*kind),object_cell);
            put16(at(free_count+2*kind),1);
        }
    }
    bool first_bucket_cell(u8 kind,unsigned bucket,u8 tag){
        const auto* cell=at(object_cell);
        return be32(at(group_head+4*bucket))==object_cell&&be32(at(group_tail+4*bucket))==object_cell&&
            !be32(at(free_head+4*kind))&&!be32(at(free_tail+4*kind))&&!half(free_count+2*kind)&&
            cell[1]==bucket&&cell[2]==kind&&cell[3]==kind&&cell[0x83]==tag&&
            !be32(cell+4)&&!be32(cell+8);
    }
    void rigid(u32 address,float x,float y,float z){
        auto* p=at(address);
        std::memset(p,0,48);
        for(unsigned i=0;i<3;++i)put_float(p+4*(5*i),1.f);
        put_float(p+12,x);put_float(p+28,y);put_float(p+44,z);
    }
    u32 pair_helper(u32 entry,u32 left,u32 right,u32 mask=0){
        CPUState cpu{};
        cpu.gpr[3]=left;cpu.gpr[4]=right;cpu.gpr[5]=mask;
        mod.run(cpu,ram,entry);
        return cpu.gpr[3];
    }
    void damage_inlet(const HpInlet& inlet){
        auto* receiver=at(peer);
        constexpr u32 receiver_spec=0x81015400,difficulty=0x81019600;
        std::memcpy(at(spec),inlet.actor_spec.data(),inlet.actor_spec.size());
        std::memcpy(at(receiver_spec),inlet.actor_spec.data(),inlet.actor_spec.size());
        put32(receiver+0x4AC,receiver_spec);
        for(const u32 obj:{actor,peer}){
            auto* fighter=at(obj);
            fighter[0x18]=1; // Admit the active, ordinary damage branch.
            put16(fighter+0x3E8,0x000A);
            put32(fighter+0x90,obj);
            put_float(fighter+0xB4,1.f);put_float(fighter+0xC4,1.f);
            put16(fighter+0x1C4,180);
        }
        put16(a+0x1C6,180);put16(receiver+0x1C6,inlet.receiver_hp);
        receiver[0x3E6]=inlet.receiver_source;
        // Declared scene mirrors, team force caps, difficulty, and rank.
        // This fixture does not run the scene or level scheduler.
        put32(at(0x8043B5A0-31824),difficulty);
        at(difficulty)[0]=inlet.settings_index;at(difficulty)[1]=inlet.settings_index;
        put16(at(0x803B069C),180);put16(at(0x803B069C+2),inlet.receiver_hp);
        for(unsigned team=0;team<2;++team){
            put32(at(0x803B068C+4*team),180);
            put32(at(settings+team*60+0x114),180);
            put32(at(settings+team*60+0x118),180);
        }
        at(settings)[0x1F]=inlet.protect_hp;
    }
public:
    u8* const a;
    Check(const Module& module,const std::vector<u8>& original,Profile p=synthetic)
        :mod(module),ram(original),a(at(actor)){
        put32(a+0x4B0,p.cfg);put32(a+0x4AC,spec);put32(a+0x4EC,p.context);put32(a+0x4F0,p.selector);
        put32(at(0x8043B5A0-31852),settings);put32(at(0x8043B5A0-31856),mode_state);
        for(unsigned i=0;i<6;++i)put32(at(0x803C4E84)+4*i,i?peer+0x1E00*(i-1):actor);
        for(unsigned g=0;g<6;++g){
            put32(at(contexts)+4*g,rows+g*0x20);
            for(unsigned choice=0;choice<6;++choice){
                auto* row=at(rows)+g*0x20+4*choice;
                row[0]=1;row[1]=3;row[2]=u8(g);row[3]=u8(choice);
            }
        }
        at(pairs)[2]=61;
        // The original stock check is inert with a zero configured channel.
        // Scene+69 must stay below 5 while polling a human packet; the sound
        // refusal is supplied only at the later, separately invoked arbiter.
        a[0x584]=255;
        at(config)[0xBB]=255;at(config)[0xBC]=255;at(config)[0xBD]=255;
        put_float(a+0x768,1.f);
    }
    void disable(unsigned group){at(config)[0xB8+group]=255;}
    void refuse_first_row(unsigned group){at(rows)[group*0x20]=255;}
    void install(u32 entry,u16 model,Profile p,u32 main){
        put16(a+0x3E8,model);
        put32(at(0x803C4D94),spec); // Declared role-0 resource; no model/scene load.
        call(entry);
        if(be32(a+0x4B0)!=p.cfg||be32(a+0x4EC)!=p.context||be32(a+0x4F0)!=p.selector||
           be32(a+0x4B4)!=main||be32(a+0x4AC)!=spec)fail("unexpected original installer writes");
        // Fixture inlet from config+2, normally published by constructor 8004BB14.
        a[0x579]=at(p.cfg)[2];
    }
    void pre_callback(){call(be32(a+0x4BC));}
    u32 frontier(CPUState& cpu){
        if(a[0x5DA]!=61||a[0x5DB]!=0)fail("expected admitted first slot and no-op second slot");
        pre_callback();
        cpu.gpr[3]=actor;
        return mod.run_to(cpu,ram,0x80065DFC,{0x8004BEB8,0x80088AA0,0x8003E0A8});
    }
    void request(u32 site,u32 group,u32 choice,u32 effective_mask,u32 program){
        CPUState cpu{};
        const auto reached=frontier(cpu);
        if(reached!=0x8004BEB8||cpu.lr!=site+4||cpu.gpr[3]!=actor||cpu.gpr[4]!=15||
           cpu.gpr[5]!=group||cpu.gpr[6]!=choice||cpu.gpr[7]!=0xFFFFFFFFu||cpu.gpr[8]!=0xFFFFFFFFu||
           (cpu.gpr[4]&a[0x579])!=effective_mask){
            std::fprintf(stderr,"request site=%08X mask=%u group=%u choice=%u r7=%08X r8=%08X effective=%u\n",
                         cpu.lr-4,cpu.gpr[4],cpu.gpr[5],cpu.gpr[6],cpu.gpr[7],cpu.gpr[8],cpu.gpr[4]&a[0x579]);
            fail("first program request mismatch");
        }
        // Qualify the local bank address from the observed operands without
        // giving the cut-off original request a result or executing cues.
        const auto local=relative_table(be32(a+0x1D80),cpu.gpr[5]);
        if(relative_table(local,cpu.gpr[6])!=program)fail("first request's local program address mismatch");
    }
    void sasuke_opening_contact(){
        if(a[0x5DA]!=61||a[0x5DB]!=0||a[0x579]!=3)fail("wrong Sasuke handler or lane seed");
        pre_callback();
        CPUState cpu{};
        cpu.gpr[3]=actor;
        CallObservation request_entry{0x8004BEB8};
        mod.run_to(cpu,ram,0x80065DFC,{0x8008AC80},&request_entry);
        const auto& args=request_entry.arguments;
        if(request_entry.count!=1||request_entry.caller!=0x800FEEA4||args[0]!=actor||
           args[1]!=15||args[2]!=3||args[3]!=1||args[4]!=u32(-1)||args[5]!=u32(-1))
            fail("Sasuke contact witness used the wrong program request");
        if(cpu.lr!=0x8004C6F4||cpu.gpr[3]!=actor||cpu.gpr[4]!=2||
           be32(at(cpu.gpr[5]))!=0x802BE3BA||a[0x744])
            fail("Sasuke opening opcode did not request queue 2 before the queue-byte write");
    }
    void sasuke_without_upper_lane(){
        const u32 cursor=rows+0x100;
        put32(at(cursor),0x802BE3B6);
        a[0x744]=9;
        CPUState cpu{};
        cpu.gpr[3]=actor;cpu.gpr[4]=2;cpu.gpr[5]=cursor;
        mod.run(cpu,ram,0x8004C6B4);
        if(be32(at(cursor))!=0x802BE3BA||a[0x744]!=9)
            fail("Sasuke opcode-10 without lane zero did not just advance the cursor");
    }
    void sasuke_pair(const HitBlock& block,bool free_cell,bool report=true){
        if(a[0x5DA]!=61||a[0x579]!=3)fail("wrong installed Sasuke pair state");
        std::memcpy(at(hit_block),block.data(),block.size());
        put32(a+0x3FC,hit_block);put32(a+0x90,actor);
        pool(4,free_cell,false);
        pre_callback();
        CallObservation observation{0x8008AC80};
        CPUState cpu{};cpu.gpr[3]=actor;
        mod.run_observed(cpu,ram,0x80065DFC,observation);
        if(observation.count!=1||observation.caller!=0x8004C6F0||
           observation.arguments[0]!=actor||observation.arguments[1]!=2||a[0x744]!=2)
            fail("Sasuke queue request or post-attempt write mismatch");
        if(!free_cell){
            if(be32(at(group_head+49*4))||a[0x86])fail("empty pair pool produced a shape");
            if(report)std::printf("sasuke: %u %u 0 0 0 0 0\n",unsigned(free_cell),a[0x744]);
            return;
        }
        const auto* cell=at(object_cell);
        if(!first_bucket_cell(4,49,0x1A)||cell[0]!=1||be32(cell+0xC)!=0x8008A65C||
           be32(cell+0x20)!=actor||be32(cell+0x24)!=actor||
           be32(cell+0x28)!=hit_block+0xAA0+2*80||be32(cell+0x2C)!=0x802D4160||
           be32(cell+0x60)!=actor+0x8D4+7*0x30||cell[0x11]!=2||cell[0x12]!=2||cell[0x14]!=2||a[0x86]!=1){
            std::fprintf(stderr,"pair: head=%08X form=%u bucket=%u class=%u tag=%02X callback=%08X "
                         "carrier=%08X owner=%08X step=%08X tuning=%08X matrix=%08X indexes=%u/%u/%u active=%u\n",
                         be32(at(group_head+4*49)),cell[0],cell[1],cell[2],cell[0x83],be32(cell+0xC),
                         be32(cell+0x20),be32(cell+0x24),be32(cell+0x28),be32(cell+0x2C),be32(cell+0x60),
                         cell[0x11],cell[0x12],cell[0x14],a[0x86]);
            fail("original Sasuke class-4 pair publication mismatch");
        }
        if(report)std::printf("sasuke: %u %u 1 %u %u %u %u\n",unsigned(free_cell),a[0x744],
                              cell[1],cell[0x83],cell[0x12],cell[0x11]);
    }
    void sasuke_shape_sweep(const HitBlock& block,unsigned distance,bool suppress,bool immune,bool receiver_gate,
                            const HpInlet* hp=nullptr){
        if(!first_bucket_cell(4,49,0x1A))fail("shape sweep requires the original attack allocation");
        a[0]=1;
        put_float(a+0xC0,1.f);
        rigid(actor+0x8D4+7*0x30,100,200,300);
        put32(a+0x76C,suppress?0:1u<<7);

        // Restrict the receiver to one real body descriptor (step 9).
        // Only the scratch body's list is narrowed; the saved descriptor
        // is untouched and the original body bind creates its record.
        std::memcpy(at(hurt_block),block.data(),block.size());
        at(hurt_block)[0]=9;at(hurt_block)[1]=255;
        auto* target=at(peer);
        target[0]=1;target[0x88]=1;target[0x3E4]=1;
        put32(target+0x3FC,hurt_block);put32(target+0x27C,0x802D4160);
        put32(target+0x76C,1u<<2);
        put16(target+0x1C4,180);put16(target+0x1C6,180);
        put_float(target+0xC0,1.f);
        rigid(peer+0x8D4+2*0x30,130+float(distance),194,290);
        at(hurt_cell)[2]=4;
        put32(at(free_head+16),hurt_cell);put32(at(free_tail+16),hurt_cell);
        put16(at(free_count+8),1);
        CPUState bind{};bind.gpr[3]=peer;
        mod.run(bind,ram,0x8008AB30);
        if(be32(at(group_head+50*4))!=hurt_cell||be32(at(group_tail+50*4))!=hurt_cell||
           be32(at(hurt_cell+0xC))!=0x8008A65C||be32(at(hurt_cell+0x28))!=hurt_block+32+9*80)
            fail("receiver's original body allocation did not publish the singleton");

        const auto sweep=[&](float cursor){
            // This is an explicit scratch-frame inlet, not the result
            // of a render/pose update. Start each pass with empty registries.
            put_float(a+0x1CDC,cursor);
            put16(at(attack_count),0);put16(at(receiver_count),0);
            call(0x80089AEC);
        };
        sweep(18.f);
        if(at(object_cell)[0x10]||half(attack_count)||half(receiver_count)!=1||
           be32(at(receiver_registry))!=hurt_cell)
            fail("before the attack cursor the original pass should only publish the receiver");
        sweep(19.f);
        const bool listed=!suppress;
        if(at(object_cell)[0x10]!=1||half(attack_count)!=unsigned(listed)||
           (listed&&be32(at(attack_registry))!=object_cell)||half(receiver_count)!=1||
           be32(at(receiver_registry))!=hurt_cell||fp(object_cell+0x64)!=130||fp(object_cell+0x68)!=180||
           fp(object_cell+0x6C)!=290||fp(object_cell+0x7C)!=60||
           fp(hurt_cell+0x64)!=130+float(distance)||fp(hurt_cell+0x68)!=180||fp(hurt_cell+0x6C)!=290||
           fp(hurt_cell+0x7C)!=22)
            fail("ready-shape geometry or registry publication mismatch");
        const bool overlap=pair_helper(0x80039F6C,object_cell+0x58,hurt_cell+0x58)!=0;
        // Apply the relation control after publication so the pair loop
        // sees the original registry entry but must still check eligibility.
        put16(target+0x272,immune?2:0);
        target[0x1DA]=receiver_gate?2:0;
        if(hp)damage_inlet(*hp);
        const auto initial_hp=hp?hp->receiver_hp:180;
        const bool permitted=pair_helper(0x8002FD7C,object_cell,hurt_cell,2)!=0;
        if(overlap!=(distance<=82)||permitted==immune||half(peer+0x1C6)!=initial_hp)
            fail("original sphere/receiver filter comparison mismatch");
        const bool should_dispatch=listed&&overlap&&permitted&&!receiver_gate;
        const auto before_contact=ram;
        CallObservation classifier{0x80030500};
        CPUState collision{};
        const auto entry=mod.run_to_or_return(collision,ram,0x8002DB58,{0x80030500},&classifier);
        u32 accepted=0;
        if(should_dispatch){
            if(entry!=0x80030500||classifier.count!=1||
               classifier.arguments[0]!=0x803B04D0||classifier.arguments[1]!=0x803B0368){
                std::fprintf(stderr,"sphere boundary=%08X calls=%u caller=%08X args=%08X/%08X geom=%u/%u\n",
                             entry,classifier.count,classifier.caller,classifier.arguments[0],classifier.arguments[1],
                             be32(at(object_cell+0x58)),be32(at(hurt_cell+0x58)));
                fail("pair loop did not reach the sphere classifier with its original scratch shapes");
            }
            // Execute only the sphere test on the exact scratch operands.
            // Do not return its value into the stopped loop or run reaction.
            accepted=pair_helper(entry,classifier.arguments[0],classifier.arguments[1]);
            if(accepted!=1)fail("candidate pair failed the original sphere narrow phase");
        }else if(entry||classifier.count)fail("a refused candidate reached the narrow phase");
        if(half(peer+0x1C6)!=initial_hp)fail("scoped classifier changed HP");
        // A fresh replay from both original registries now lets the original
        // pair loop consume the classifier result. Stop before amount/HP.
        std::memcpy(ram.data(),before_contact.data(),ram.size());
        CPUState reaction{};
        const auto boundary=mod.run_to_or_return(reaction,ram,0x8002DB58,{0x8003CD5C,0x8003D344});
        if((should_dispatch?
                boundary!=0x8003CD5C||reaction.lr!=0x8002F10C||reaction.gpr[3]!=0x802D4190||
                reaction.gpr[4]!=actor||reaction.gpr[5]!=peer:
                boundary!=0)||half(peer+0x1C6)!=initial_hp)
            fail("original pair loop sent the wrong amount request");
        if(hp){
            std::memcpy(ram.data(),before_contact.data(),ram.size());
            CPUState writer{};
            const auto hp_cut=mod.run_to_or_return(writer,ram,0x8002DB58,{0x8003D344});
            if((should_dispatch?(hp_cut!=0x8003D344||writer.lr!=0x8002F1DC||writer.gpr[3]!=peer):hp_cut!=0)||
               half(peer+0x1C6)!=initial_hp)fail("fixture applied HP before the original writer");
            const u32 amount=hp_cut?writer.gpr[4]:0;
            if(hp_cut)mod.finish_stopped_leaf(writer,hp_cut,164);
            const unsigned expected=should_dispatch?(initial_hp==1?34:hp->receiver_source?40:35):0;
            const bool writes=should_dispatch&&!hp->protect_hp;
            const auto remaining=initial_hp-(writes?std::min<unsigned>(initial_hp,expected):0);
            const unsigned credit=writes?expected:0;
            if(amount!=expected||half(peer+0x1C6)!=remaining||half(peer+0x1C8)!=(writes?initial_hp:0)||
               be32(a+0x418)!=credit||be32(target+0x41C)!=credit||a[0x88]!=0||target[0x88]!=1||
               (hp_cut&&writer.gpr[3]!=unsigned(writes&&!remaining)))
                fail("original HP writer, controller-source bank, or contact ledger mismatch");
            std::printf("contact-hp: %u %u %u %u %u %d %u %u %u %u %u %u %u %u %u\n",distance,unsigned(suppress),
                        unsigned(immune),unsigned(receiver_gate),unsigned(hp->protect_hp),initial_hp,
                        hp->settings_index,hp->receiver_source,unsigned(hp_cut!=0),
                        amount,half(peer+0x1C6),half(peer+0x1C8),be32(a+0x418),be32(target+0x41C),
                        hp_cut?writer.gpr[3]:0);
            return;
        }
        std::printf("shape-sweep: %u %u %u %u %u %u %u %u %u %u\n",distance,unsigned(suppress),unsigned(immune),
                    unsigned(receiver_gate),unsigned(listed),unsigned(overlap),unsigned(permitted),
                    classifier.count,accepted,unsigned(boundary!=0));
    }
    void machine_red_cue(bool visit_upper){
        if(a[0x5DA]!=61||a[0x5DB]!=0||a[0x579]!=7)fail("wrong Machine Red opening slots");
        pre_callback();
        a[0x1CEF]=0x7E; // Nonzero old value to distinguish the real opening clear.
        CPUState cpu{};
        cpu.gpr[3]=actor;
        CallObservation observation{0x8004BEB8};
        const auto boundary=mod.run_to(cpu,ram,0x80065DFC,{0x8003E0A8},&observation);
        if(boundary!=0x8003E0A8||observation.count!=1||observation.caller!=0x800C9634||
           observation.arguments[3]!=0||a[0x1CEF]||be32(a+0x1CD4)!=0x80309056)
            fail("Machine Red opening did not reach the first wait before the world query");
        put_float(a+0x1DC8,1.f);
        // Continue only the VM on the scratch actor. Do not fabricate
        // the stopped world-query result or resume the action tick.
        unsigned visits=0;
        for(;visits<16&&!a[0x1CEF];++visits){
            CPUState step{};
            step.gpr[3]=actor;
            step.gpr[4]=visit_upper?15:6;
            mod.run(step,ram,0x8004CD24);
        }
        if(visit_upper){
            if(visits!=11||a[0x1CEF]!=1||be32(a+0x1CD4)!=0x8030907E)
                fail("rifle cue or cursor mismatched in the VM-only fixture");
        }else if(visits!=16||a[0x1CEF]||be32(a+0x1CD4)!=0x80309056||be32(a+0x1CF4)==0x80309056)
            fail("advancing only the other lanes must not set the rifle cue");
    }
    void machine_red_release(u16 initial,bool has_cue,bool free_cell,bool free_descriptor){
        if(a[0x540]!=1||a[0x1CEF]!=unsigned(has_cue))fail("rifle consumer has no matching VM inlet");
        put32(a+0x90,actor);
        put16(a+0x774,initial);put16(a+0x776,initial);
        put16(a+0x78E,5);put16(a+0x790,0);put16(a+0x792,300);
        pool(3,free_cell,free_descriptor);
        CallObservation allocation{0x80088AA0};
        CPUState cpu{};cpu.gpr[3]=actor;
        mod.run_to(cpu,ram,0x80065DFC,{0x8003E0A8},&allocation);
        const bool paid=has_cue&&initial;
        const auto after=u16(initial-paid);
        const auto observed_count=half(actor+0x774);
        const auto observed_clock=std::bit_cast<float>(be32(a+0x778));
        if(half(actor+0x774)!=after||half(actor+0x776)!=initial||
           be32(a+0x778)!=std::bit_cast<u32>(paid&&!after?300.f:0.f)||
           allocation.count!=unsigned(paid)||a[0x540]!=(has_cue?2:1))
            fail("rifle debit, phase, or refill clock mismatch");
        if(paid&&(allocation.caller!=0x800C3C10||allocation.arguments[0]!=actor||
           allocation.arguments[1]!=3||allocation.arguments[2]!=8||
           allocation.arguments[3]!=0||allocation.arguments[4]!=1))
            fail("rifle typed allocation inlet mismatch");
        const bool created=paid&&free_cell&&free_descriptor;
        if(!created){
            if(be32(at(group_head+4*9))||be32(at(group_tail+4*9))||
               half(descriptor_row+18)!=unsigned(free_descriptor)||
               half(free_count+2*3)!=unsigned(free_cell))
                fail("refused rifle transaction changed a pool");
        }else{
            const auto* cell=at(object_cell);
            if(!first_bucket_cell(3,9,0x10)||cell[0]!=1||cell[0x10]!=2||cell[0x11]!=21||
               be32(cell+0xC)!=0x800C3CDC||be32(cell+0x8C)!=actor||be32(cell+0x90)!=actor||
               cell[0x99]!=1||be32(cell+0xE0)!=descriptor_cell||half(descriptor_row+18)||
               be32(at(descriptor_row+8))!=descriptor_cell||be32(at(descriptor_row+12)))
                fail("original rifle carrier or descriptor publication mismatch");
        }
        unsigned restored=0;
        if(paid&&!after){
            // Observe refill on the depleted channel without resuming the
            // world query or claiming scheduler cadence.
            a[0x597]=1;
            call(0x8006DCC0);
            if(be32(a+0x778)!=std::bit_cast<u32>(300.f)||half(actor+0x774))
                fail("inhibited rifle refill consumed the clock");
            a[0x597]=0;
            for(unsigned n=1;n<300;++n){
                call(0x8006DCC0);
                if(half(actor+0x774)||half(actor+0x776)!=initial||
                   be32(a+0x778)!=std::bit_cast<u32>(float(300-n)))
                    fail("rifle refill restored early or advanced at the wrong rate");
            }
            call(0x8006DCC0);
            if(half(actor+0x774)!=5||half(actor+0x776)!=5||be32(a+0x778))
                fail("rifle refill did not restore both count mirrors");
            restored=half(actor+0x774);
        }
        const auto* cell=at(object_cell);
        std::printf("rifle: %u %u %u %u %u %.0f %u %u %u %u %u %u %u\n",initial,unsigned(has_cue),
                    unsigned(free_cell),unsigned(free_descriptor),observed_count,double(observed_clock),
                    allocation.count,unsigned(created),created?cell[1]:0,created?cell[0x83]:0,
                     created?cell[0x10]:0,created?cell[0x11]:0,restored);
    }
    void machine_red_bucket(const HitBlock& block,bool admit,bool shape_pool,bool suppress){
        if(!first_bucket_cell(3,9,0x10))fail("rifle bucket fixture requires a source-allocated carrier");
        auto* cell=at(object_cell);
        auto* shape=at(hurt_cell); // Separate class-4 free cell, never a body in this run.
        std::memcpy(at(hit_block),block.data(),block.size());
        put32(a+0x3FC,hit_block);
        a[0]=1;a[0x18]=1;
        at(settings)[209]=admit?0:1;
        rigid(actor+0x8D4,0,0,0);
        rigid(actor+0x8D4+7*0x30,100,200,300);
        put_float(a+0xB4,1.f);
        // No free class-2 effect cell and no visual resource payload. The
        // existing carrier descriptor remains owned by the original code.
        constexpr u32 visual=0x81030200,visual_link=visual+0x40,visual_table=visual+0x80,visual_row=visual+0xC0;
        put32(at(0x803BB374),visual);put32(at(visual+16),visual_link);
        put32(at(visual_link),visual_table);put32(at(visual_table),visual_row);
        if(shape_pool){
            shape[2]=4;
            put32(at(free_head+16),hurt_cell);put32(at(free_tail+16),hurt_cell);
            put16(at(free_count+8),1);
        }
        const auto before_phase=cell[0x18];
        CPUState pass{};pass.gpr[3]=8;
        const auto reached=mod.run_to_or_return(pass,ram,0x80089A94,{0x800F036C});
        const bool launch=admit&&shape_pool;
        if(before_phase||reached!=(launch?0x800F036Cu:0)||cell[0x18]!=(launch?1:admit?2:0)||
           half(object_cell+0x1C)!=(admit?300:0)||half(actor+0x774)!=1||
           be32(at(group_head+4*49))!=(launch?hurt_cell:0)||
           be32(at(group_tail+4*49))!=(launch?hurt_cell:0)||
           half(free_count+8)!=unsigned(!admit&&shape_pool)||half(attack_count)){
            std::fprintf(stderr,"admit=%u pool=%u stop=%08X lr=%08X phase=%u->%u timer=%u stock=%u head=%08X tail=%08X free=%u attack=%u\n",
                         unsigned(admit),unsigned(shape_pool),reached,pass.lr,before_phase,cell[0x18],half(object_cell+0x1C),
                         half(actor+0x774),be32(at(group_head+4*49)),be32(at(group_tail+4*49)),half(free_count+8),half(attack_count));
            fail("scheduled rifle initializer, pool or stock mismatch");
        }
        if(launch){
            if(pass.lr!=0x800C4068||pass.gpr[3]!=object_cell||pass.gpr[4]!=185||cell[0x82]!=1||
               be32(at(0x8043B5A0-21272+4))!=object_cell||be32(at(0x8043B5A0-21264+4))!=object_cell||
               be32(at(group_head+4*25))||be32(at(descriptor_cell+12))||
               shape[1]!=49||shape[0x83]!=0x1A||be32(shape+0xC)!=0x8008A65C||
               be32(shape+0x20)!=object_cell||be32(shape+0x24)!=actor||
               be32(shape+0x28)!=hit_block+0xAA0+15*80||be32(shape+0x60)!=object_cell+0x114)
                fail("rifle did not bind the source-linked projectile shape and register before audio");
        }
        // Do not resume the stopped carrier pass or supply an audio result.
        // Visit the independently callable class-4 sweep on the same lists.
        if(suppress)put16(cell+0x272,1);
        call(0x8008A28C);
        call(0x80089AEC);
        const bool listed=launch&&!suppress;
        if(shape[0x10]!=unsigned(launch)||half(attack_count)!=unsigned(listed)||
           be32(at(attack_registry))!=(listed?hurt_cell:0)||half(receiver_count)||half(actor+0x774)!=1)
            fail("rifle shape activation or publication mismatch");
        if(launch){
            constexpr std::array<float,3> center{-149.39999f,191.f,365.f};
            for(unsigned i=0;i<center.size();++i){
                const auto near=[&](u32 field){return std::abs(fp(hurt_cell+field+4*i)-center[i])<0.0001f;};
                if(!near(0x64)||!near(0x8C)||!near(0x98))fail("rifle shape center or initial endpoint mismatch");
            }
            if(!(std::abs(fp(hurt_cell+0x7C)-32.46753f)<0.0001f))fail("rifle shape radius mismatch");
        }
        std::printf("rifle-bucket: %u %u %u %u %u %u %u %u %u %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n",
                    unsigned(admit),unsigned(shape_pool),unsigned(suppress),cell[0x18],half(object_cell+0x1C),
                    unsigned(reached!=0),shape[1],shape[0x10],half(attack_count),
                    double(fp(hurt_cell+0x64)),double(fp(hurt_cell+0x68)),double(fp(hurt_cell+0x6C)),
                    double(fp(object_cell+0x38)),double(fp(object_cell+0x3C)),double(fp(object_cell+0x40)),
                    double(fp(hurt_cell+0x7C)));
    }
    void charge(unsigned channel,float limit=2.f){
        if(be32(a+0x4B0)==config)at(config)[0xBB+channel]=0;
        put_float(at(spec)+0x184+4*channel,limit);
        // Declare an already-bound ancillary descriptor, so the original
        // same-sample path updates its existing slot without starting audio.
        put32(at(0x803D3B80),be32(at(0x8031B698)));
    }
    void input(u32 buttons,u32 held,u32 pressed,u32 released,u32 flags,const char* name){
        at(settings)[69]=0;
        put32(at(packet),buttons);
        call(0x80056900);
        call(0x800685C0);
        if(be32(a+0x5B4)!=held||be32(a+0x5BC)!=pressed||be32(a+0x5C0)!=released||
           be32(a+0x5D4)!=flags){
            std::fprintf(stderr,"%s: held=%X pressed=%X released=%X request=%X\n",
                         name,be32(a+0x5B4),be32(a+0x5BC),be32(a+0x5C0),be32(a+0x5D4));
            fail("processed packet or conditioned request mismatch");
        }
    }
    void expect(int group,u8 state,const char* name){
        at(settings)[69]=5;
        const u32 result=call(arbiter);
        if(result!=u32(group>=0)||(group>=0&&(a[0x584]!=group||a[0x580]!=state||
                                             a[0x5DA]!=61||a[0x5DB]!=0))){
            std::fprintf(stderr,"%s: arbiter=%u group=%u state=%u slots=%u:%u\n",
                         name,result,a[0x584],a[0x580],a[0x5DA],a[0x5DB]);
            fail("action arbitration mismatch");
        }
    }
};
}

int main(int argc,char** argv){
    const bool rifle_bucket=argc==4&&std::strcmp(argv[3],"--rifle-bucket")==0;
    const bool hp=argc==4&&std::strcmp(argv[3],"--sasuke-contact-hp")==0;
    const bool sweep=hp||(argc==4&&std::strcmp(argv[3],"--sasuke-shape-sweep")==0);
    if(argc!=3&&!sweep&&!rifle_bucket&&!(argc==4&&std::strcmp(argv[3],"--sasuke-hit-block")==0))
        fail("usage: action-input-fixture qualified.dol unmodified-recomp.so [--rifle-bucket|--sasuke-hit-block|--sasuke-shape-sweep|--sasuke-contact-hp]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(rifle_bucket){
        HitBlock block{};
        if(std::fread(block.data(),1,block.size(),stdin)!=block.size()||std::fgetc(stdin)!=EOF)
            fail("rifle hit-block inlet extent");
        struct BucketCase {bool admit,shape_pool,suppress;};
        for(const auto test:{BucketCase{false,true,false},BucketCase{true,false,false},
                            BucketCase{true,true,true},BucketCase{true,true,false}}){
            Check c(mod,original);
            c.install(0x800C91BC,0x0600,red,0x800C94D4);
            c.pre_callback();
            c.input(b,b,b,0,0x60,"Machine Red bounded bucket update");
            c.expect(0,0,"Machine Red bucket update ranged B");
            c.machine_red_cue(true);
            put16(c.a+0x94,0x0600); // Declared ordinary owner model mirror, copied at allocation.
            c.machine_red_release(2,true,true,true);
            c.machine_red_bucket(block,test.admit,test.shape_pool,test.suppress);
        }
        return 0;
    }
    {
        Check c(mod,original,red);
        c.input(0,0,0,0,0,"neutral");
        c.expect(-1,0,"neutral Machine Red");
        c.input(b,b,b,0,0x60,"fresh B");
        c.expect(0,0,"Machine Red actual ranged B row");
        c.input(b,b,0,0,0,"held B");
        c.expect(-1,0,"held B does not start another ordinary move");
        c.input(0,0,0,b,0,"ordinary B release");
        c.expect(-1,0,"ordinary B release does not start an action");
    }
    {
        Check c(mod,original,sasuke);
        c.a[0x7D2]=8;
        c.input(b,b,b,0,0x60,"fresh B with close-admit flag");
        c.expect(1,1,"Sasuke actual close-B row");
    }
    {
        Check c(mod,original,red);
        c.input(b|x,b|x,b|x,0,0xE0,"fresh B plus X");
        c.expect(2,2,"Machine Red selects X over eligible B");
    }
    {
        Check c(mod,original);
        c.refuse_first_row(2);
        c.input(b|x,b|x,b|x,0,0xE0,"same press with refused X row");
        c.expect(0,0,"refused X falls through to B");
    }
    {
        Check c(mod,original);
        c.a[0x58E]=4;
        c.input(b|x,b|x,b|x,0,0xE0,"same press with actor X inhibit");
        c.expect(0,0,"actor-inhibited X falls through to B");
    }
    {
        Check c(mod,original);
        c.a[0x7D2]=8;
        c.disable(1);
        c.input(b,b,b,0,0x60,"same B with close mode refused");
        c.expect(0,0,"refused close branch falls through to ranged B");
    }
    {
        Check c(mod,original);
        c.input(y,y,y,0,0x200,"fresh Y");
        c.expect(-1,0,"Y flag is not a row request in this arbiter");
    }
    {
        Check c(mod,original);
        c.a[0x488]=3;
        c.input(b|x,b|x,0,0,0,"source switch suppresses edges");
        c.expect(-1,0,"source switch cannot manufacture a fresh attack");
    }
    for(auto channel:{0u,2u}){
        Check c(mod,original);
        c.charge(channel);
        const u32 button=channel==0?b:x;
        const u32 fresh=channel==0?0x60:0x80;
        const u32 release=channel==0?0x400:0x1000;
        c.input(button,button,button,0,fresh,"short charge press");
        c.input(0,0,0,button,0,"release before charge threshold");
        c.expect(-1,0,"short release cannot start charged action");
        c.input(button,button,button,0,fresh,"start charge hold");
        for(unsigned i=0;i<14;++i)c.input(button,button,0,0,0,"continue charge hold");
        // Combine a fresh opposing button with release on the same update.
        const u32 other=channel==0?x:b;
        c.input(other,other,other,button,release|(channel==0?0x80:0x60),"charged release plus fresh press");
        c.expect(channel==0?3:5,channel==0?3:5,"mature charged release wins over plain attack");
    }
    for(unsigned profile=0;profile<2;++profile){
        Check c(mod,original,black[profile]);
        // Limits extracted from the installer-selected role-0 record.
        // Keep the actor idle deliberately; this is not a live cancel path.
        c.charge(0,60.f);c.charge(2,120.f);
        c.input(b|x,b|x,b|x,0,0xE0,"G-Black fresh B and X");
        for(unsigned i=0;i<140;++i)c.input(b|x,b|x,0,0,0,"G-Black simultaneous hold");
        c.input(0,0,0,b|x,profile?0x1400:0x400,"G-Black two-profile release");
        c.expect(profile?5:3,profile?4:3,"G-Black uses each profile's original charged row");
        c.input(0,0,0,0,0,"no repeated charge pulse after release");
    }
    {
        Check c(mod,original);
        c.install(0x800C91BC,0x0600,red,0x800C94D4);
        c.a[0x6EE]=5;
        c.pre_callback();
        if(c.a[0x6EE])fail("Machine Red neutral pre-callback did not seed choice");
        c.input(b,b,b,0,0x60,"installed Machine Red B");
        c.expect(0,0,"installed Machine Red B row");
        c.request(0x800C9634,2,0,7,0x8030903E);
    }
    {
        constexpr Profile installed_sasuke{0x802D3EE0,0x802D4148,0x802D3FF8};
        Check c(mod,original);
        c.install(0x8006F4F8,0x000A,installed_sasuke,0x8006FACC);
        c.a[0x7D2]=8;
        c.input(b,b,b,0,0x60,"installed Sasuke close B");
        c.expect(1,1,"installed Sasuke close row");
        c.request(0x800FEEA4,3,1,3,0x802BE3B6);
    }
    {
        constexpr Profile installed_sasuke{0x802D3EE0,0x802D4148,0x802D3FF8};
        Check c(mod,original);
        c.install(0x8006F4F8,0x000A,installed_sasuke,0x8006FACC);
        c.a[0x7D2]=8;
        c.input(b,b,b,0,0x60,"Sasuke through opening melee opcode");
        c.expect(1,1,"Sasuke contact row");
        c.sasuke_opening_contact();
        c.sasuke_without_upper_lane();
    }
    for(const bool visit_upper:{true,false}){
        Check c(mod,original);
        c.install(0x800C91BC,0x0600,red,0x800C94D4);
        c.pre_callback();
        c.input(b,b,b,0,0x60,"Machine Red through first program wait");
        c.expect(0,0,"Machine Red opening row");
        c.machine_red_cue(visit_upper);
    }
    struct RifleCase {u16 initial;bool cue,cell,descriptor;};
    for(const auto test:{RifleCase{2,true,true,true},RifleCase{1,true,false,true},
                        RifleCase{1,true,true,false},RifleCase{0,true,true,true},RifleCase{2,false,true,true}}){
        Check c(mod,original);
        c.install(0x800C91BC,0x0600,red,0x800C94D4);
        c.pre_callback();
        c.input(b,b,b,0,0x60,"Machine Red bounded release");
        c.expect(0,0,"Machine Red release row");
        c.machine_red_cue(test.cue);
        c.machine_red_release(test.initial,test.cue,test.cell,test.descriptor);
    }
    if(argc==4){
        HitBlock block{};
        ActorSpec actor_spec{};
        if(std::fread(block.data(),1,block.size(),stdin)!=block.size()||
           (hp&&std::fread(actor_spec.data(),1,actor_spec.size(),stdin)!=actor_spec.size())||std::fgetc(stdin)!=EOF)
            fail("Sasuke hit-block inlet extent");
        for(const bool free_cell:{true,false}){
            constexpr Profile installed_sasuke{0x802D3EE0,0x802D4148,0x802D3FF8};
            Check c(mod,original);
            c.install(0x8006F4F8,0x000A,installed_sasuke,0x8006FACC);
            c.a[0x7D2]=8;
            c.input(b,b,b,0,0x60,"Sasuke bounded shape allocation");
            c.expect(1,1,"Sasuke pair row");
            c.sasuke_pair(block,free_cell);
        }
        if(sweep){
            struct Case {
                unsigned distance;bool suppress=false,immune=false,receiver_gate=false,protect=false;
                u16 health=180;u8 settings_index=0,receiver_source=0;
            };
            for(const auto test:{Case{81},Case{82},Case{83},Case{81,true},Case{81,false,true},Case{81,false,false,true},
                                 Case{81,false,false,false,true},Case{81,false,false,false,false,1},
                                 Case{81,false,false,false,false,180,7,0},Case{81,false,false,false,false,180,7,1}}){
                if(!hp&&(test.protect||test.health!=180||test.settings_index))continue;
                constexpr Profile installed_sasuke{0x802D3EE0,0x802D4148,0x802D3FF8};
                Check c(mod,original);
                c.install(0x8006F4F8,0x000A,installed_sasuke,0x8006FACC);
                c.a[0x7D2]=8;
                c.input(b,b,b,0,0x60,"Sasuke bounded shape sweep");
                c.expect(1,1,"Sasuke sweep row");
                c.sasuke_pair(block,true,false);
                const HpInlet damage{actor_spec,test.health,test.protect,test.settings_index,test.receiver_source};
                c.sasuke_shape_sweep(block,test.distance,test.suppress,test.immune,test.receiver_gate,hp?&damage:nullptr);
            }
        }
        std::puts("Sasuke: exact queue-2 pair and empty pool passed");
    }
    for(const bool use_x:{false,true}){
        Check c(mod,original);
        // The real model-gated installer selects p089 for model 062A.
        c.install(0x8018CCFC,0x062A,black[1],0x8018D24C);
        c.pre_callback();
        c.charge(0,60.f);c.charge(2,120.f);
        const u32 button=use_x?b|x:b;
        c.input(button,button,button,0,use_x?0xE0:0x60,"installed G-Black fresh press");
        for(unsigned i=0;i<140;++i)c.input(button,button,0,0,0,"installed G-Black hold");
        c.input(0,0,0,button,use_x?0x1400:0x400,"installed G-Black release");
        c.expect(use_x?5:3,use_x?4:3,"installed G-Black charged row");
        CPUState cpu{};
        const auto reached=c.frontier(cpu);
        if(cpu.gpr[3]!=actor||
           (use_x?(reached!=0x8003E0A8||cpu.lr!=0x800677F4||cpu.gpr[4]!=actor+0x20):
                  (reached!=0x80088AA0||cpu.lr!=0x8016CC54||cpu.gpr[4]!=3||cpu.gpr[5]!=8||
                   cpu.gpr[6]!=0||cpu.gpr[7]!=1)))fail("G-Black did not reach its expected pre-request service");
    }
    std::puts("action input: lookup, request, Sasuke queue 2 and Machine Red cue, paid release and refill passed");
}
