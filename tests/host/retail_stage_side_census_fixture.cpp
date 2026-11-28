// Replay the original side-stage placement loop, not the enclosing scene loader.
#include "retail_contact_effect_archive.hpp"
#include "retail_declared_stage_grid.hpp"
#include <algorithm>
#include <cmath>

namespace {
using namespace retail_fixture;
namespace contact=retail_stationary_contact;
namespace art=retail_contact_effect_archive;
constexpr u32 stage_files=0x81070000,resources=0x81200000,common=0x81268000;
constexpr u32 pools=0x81069000,models=0x81069800,archives=0x8106A000;
constexpr u32 owners=0x81030000,owner_step=0x800,shapes=0x81060000,shape_step=0x200;
constexpr u32 group40=contact::active_head+40*4,group48=contact::active_head+48*4;
constexpr u32 flags=0x803B2908,common_roles=0x803C237C,common_role_count=0x8043B5A0-21340;
constexpr unsigned owner_limit=16,shape_limit=32,bank_limit=4;
constexpr u32 type_table=0x802D7990,heap_limit=art::arena+0x100000;
constexpr u32 stage=0x81070000,actor_body=0x81020000,actor_bank=0x81054000,actor_spec=actor_bank+0x2000;
struct ReceiverInput {contact::HitBlock hit;contact::ActorSpec spec;};

class Census {
    const Module& mod;
    std::vector<u8> ram;
    std::array<u32,8> gqr{};
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    float scalar(u32 a){return std::bit_cast<float>(word(a));}
    void store(u32 a,u32 v){put32(at(a),v);}
    contact::Point point(u32 a){return {scalar(a),scalar(a+4),scalar(a+8)};}
    void put_point(u32 a,const contact::Point& p){for(unsigned i=0;i<3;++i)put_float(at(a+4*i),p[i]);}
    void rig(u32 a,const contact::Point& p){
        for(unsigned i=0;i<3;++i){put_float(at(a+20*i),1.f);put_float(at(a+12+16*i),p[i]);}
    }
    void call(u32 fn,u32 arg=0,CallObservation* observation=nullptr){
        CPUState cpu{};cpu.gpr[3]=arg;std::copy(gqr.begin(),gqr.end(),std::begin(cpu.gqr));
        if(mod.run_to_or_return(cpu,ram,fn,{0x80264290},observation,1u<<20)){
            std::fprintf(stderr,"side census assertion: entry=%08X caller=%08X line=%u\n",fn,cpu.lr-4,cpu.gpr[4]);
            fail("original side census hit a guest assertion");
        }
    }
    void init_render_pool(unsigned type,u32 first,unsigned count,u32 recipe){
        const auto row=pools+28*type;
        store(row,first-word(art::source_bank_basis));
        store(row+4,count*half(recipe+16));
        std::memcpy(at(row+16),at(recipe+16),12);
        call(0x80197950,type);
        if(word(row+12)!=first||half(row+18)!=count||word(row+8))
            fail("side census input descriptor pool did not initialize");
    }
    void make_pool(unsigned type,const std::array<u32,shape_limit>& cells,unsigned count){
        if(!count)return;
        for(unsigned i=0;i<count;++i){
            at(cells[i]+2)[0]=u8(type);
            store(cells[i]+4,i+1<count?cells[i+1]:0);
        }
        store(contact::free_head+4*type,cells[0]);store(contact::free_tail+4*type,cells[count-1]);
        put16(at(contact::free_count+2*type),u16(count));
    }
    void mute(){
        constexpr u32 audio=0x8106B000;
        store(0x8043B5A0-31856,audio);at(audio+12)[0]=255;
        at(0x803D3AE8+852)[0]=1;
    }
    void side_visit(CallObservation* observation=nullptr){
        call(0x80089A94,40,observation);call(0x80089554);
        call(0x8008A28C);call(0x80089AEC);
    }
    void bind_receiver(const ReceiverInput& sasuke,const contact::Point& pose){
        std::memcpy(at(actor_bank),sasuke.hit.data(),sasuke.hit.size());
        at(actor_bank)[0]=0;at(actor_bank+1)[0]=11;at(actor_bank+2)[0]=255;
        std::memcpy(at(actor_spec),sasuke.spec.data(),sasuke.spec.size());store(0x803C4D94,actor_spec);
        at(actor_body)[0]=at(actor_body+0x18)[0]=at(actor_body+0x3E5)[0]=1;
        put16(at(actor_body+0x3E8),0x000A);put16(at(actor_body+0x1C4),180);put16(at(actor_body+0x1C6),180);
        store(actor_body+0x3FC,actor_bank);store(actor_body+0x90,actor_body);store(actor_body+0x76C,1|(1u<<6));
        for(u32 field:{0xB4u,0xB8u,0xC0u,0xC4u})put_float(at(actor_body+field),1.f);
        put_point(actor_body+0x20,pose);
        rig(actor_body+0x1AA4,pose);
        auto joint=pose;joint[0]+=12;
        rig(actor_body+0x8D4+6*0x30,joint);
        call(0x8006F4F8,actor_body);
        call(0x8008AB30,actor_body);
        for(unsigned i=0;i<2;++i){call(0x8008A28C);call(0x80089AEC);}
    }
    void bind_support_grid(){
        const auto retained_side=word(stage_files+24);
        retail_declared_stage_grid::bind(mod,ram,stage,false,100.f);
        store(stage+0x800+24,retained_side);
    }
    contact::Point support_top(u32 cell,unsigned& face){
        const auto kind=word(cell+0x58);
        auto top=point(cell+0x64);
        face=255;
        if(kind==4){
            if(scalar(cell+0x8C)<0.99f||scalar(cell+0xA0)!=140.f)
                fail("side support original horizontal plane shape changed");
            top[1]+=scalar(cell+0xA0);
        }else if(kind==1){
            // Select the upward face from the original allocated corners
            // and normals; the original pair decides whether it supports.
            constexpr u32 face_table=0x802CFFC8;
            for(unsigned f=0;f<6;++f){
                if(scalar(cell+0xEC+24*f)<0.99f)continue;
                if(face!=255)fail("side support expected one upward box face");
                face=f;
                top={};
                for(unsigned v=0;v<4;++v){
                    const auto corner=at(face_table+4*f+v)[0];
                    if(corner>=8)fail("side support original box face corner extent");
                    const auto p=point(cell+0x88+12*corner);
                    for(unsigned k=0;k<3;++k)top[k]+=p[k]*0.25f;
                }
            }
            if(face==255)fail("side support original box face was not horizontal");
        }else fail("side support original role-20 kind changed");
        return top;
    }
    std::pair<unsigned,unsigned> support_walk(u32 cell,u32 owner){
        CPUState walk{};std::copy(gqr.begin(),gqr.end(),std::begin(walk.gqr));
        CallObservation narrow{word(cell+0x58)==4?0x80035698u:0x800350F8u};
        narrow.sample=[](const CPUState& cpu)->u32{
            if(cpu.lr!=0x80030410||cpu.gpr[4]!=0x803B0368)
                fail("side support reached another original narrow caller");
            return be32(cpu.ram+(0x803B036C-GC_RAM_BASE));
        };
        const auto cut=mod.run_to_or_return(walk,ram,0x8002BB14,{0x8005A580,0x80264290,contact::hp_write},&narrow,65536);
        if(cut!=0x8005A580||walk.lr!=0x8002C870||walk.gpr[3]!=actor_body||half(actor_body+0x1C6)!=180||
           narrow.count>8||word(cell+0x20)!=owner)
            fail("side support collision crossed the bounded presentation cut");
        return {narrow.count,unsigned(std::count(narrow.samples,narrow.samples+narrow.count,cell))};
    }
    void require_support(u32 cell,u32 owner){
        if(at(actor_body+0x1DB)[0]!=1||word(actor_body+0x1E0)!=cell||word(actor_body+0x1E4)!=owner||
           word(cell+0x20)!=owner||word(cell+0x60)!=owner+0x114)
            fail("side support resolved to a different cell or owner");
    }
    u32 supported_height(){
        const auto root=point(actor_body+0x20);
        CPUState query{};std::copy(gqr.begin(),gqr.end(),std::begin(query.gqr));
        query.gpr[3]=actor_body;query.gpr[4]=actor_body+0x20;query.gpr[5]=actor_spec+0x400;
        mod.run(query,ram,0x8003E0A8,4096);
        if(point(actor_body+0x20)!=root)fail("side supported height query moved the root");
        return std::bit_cast<u32>(float(query.fpr[1]));
    }
    unsigned supported_landing(){
        CPUState landing{};std::copy(gqr.begin(),gqr.end(),std::begin(landing.gqr));
        landing.gpr[3]=actor_body;
        mod.run(landing,ram,0x800677B0,4096);
        return landing.gpr[3];
    }
    void support_census(unsigned member,const ReceiverInput& sasuke){
        bind_support_grid();
        const auto initial=ram;
        const auto count=half(common_role_count);
        if(!count||count>16)fail("side support original role-20 registry extent");
        for(unsigned j=0;j<count;++j){
            const auto cell=word(common_roles+4*j),owner=word(cell+0x20);
            const auto descriptor=(word(cell+0x28)-common-0x400)/80;
            unsigned face=255;
            const auto top=support_top(cell,face);
            for(unsigned far=0;far<2;++far){
                auto pose=top;
                pose[1]+=far?1024.f:4.f;
                bind_receiver(sasuke,pose);
                if(half(common_role_count)!=count||word(common_roles+4*j)!=cell||
                   word(cell+0x20)!=owner||half(contact::receiver_count)<2)
                    fail("side support actor bind changed the original side registry");
                const auto [narrow,visited]=support_walk(cell,owner);
                const auto selected=word(actor_body+0x1E0),selected_owner=word(actor_body+0x1E4);
                const bool held=bool(at(actor_body+0x1DB)[0]);
                const auto collision_root=point(actor_body+0x20);
                u32 height=0,landing_result=0,landing_y=0;
                if(held){
                    require_support(cell,owner);
                    height=supported_height();
                    landing_result=supported_landing();
                    landing_y=word(actor_body+0x24);
                }
                if(scalar(actor_body+0x20)!=pose[0]||scalar(actor_body+0x28)!=pose[2]||
                   half(actor_body+0x1C6)!=180||word(actor_body+0x1E0)!=selected||word(actor_body+0x1E4)!=selected_owner)
                    fail("side support query or landing changed lateral pose, support identity or HP");
                std::printf("side-support: %u %u %u %u %u %u %u %u %u %08X %08X %08X %08X %08X %u\n",member,
                            (owner-owners)/owner_step,descriptor,j,face,far,narrow,visited,
                            unsigned(held)+(selected==cell?2:0)+(selected_owner==owner?4:0),
                            std::bit_cast<u32>(top[1]),std::bit_cast<u32>(pose[1]),std::bit_cast<u32>(collision_root[1]),
                            height,landing_y,landing_result);
                ram=initial;
            }
        }
    }
    unsigned shape_ordinal(u32 cell){
        if(cell<shapes||cell>=shapes+shape_limit*shape_step||(cell-shapes)%shape_step)
            fail("side rise support cell escaped its declared pool");
        return (cell-shapes)/shape_step;
    }
    void carry_prefix(bool admits){
        CPUState cpu{};std::copy(gqr.begin(),gqr.end(),std::begin(cpu.gqr));cpu.gpr[3]=actor_body;
        CallObservation helper{0x80067A70};
        const auto cut=mod.run_to_or_return(cpu,ram,0x80059068,{0x80059E58,0x80264290},&helper,4096);
        if(cut!=0x80059E58||cpu.lr!=0x800590B0||cpu.gpr[3]!=actor_body||helper.count!=1||
           helper.caller!=0x800590A4||bool(word(actor_body+0x5E0)&0x100000)!=admits)
            fail("side recovery carry escaped the original actor prefix");
    }
    void rise_support(unsigned member,const ReceiverInput& sasuke,const std::array<u32,owner_limit>& retired){
        const auto rebinding=ram;
        bind_support_grid();
        const auto initial=ram;
        const auto count=half(common_role_count);
        if(!count||count>owner_limit)fail("side recovery has no bounded support cohort");
        for(unsigned j=0;j<count;++j){
            const auto cell=word(common_roles+4*j),owner=word(cell+0x20);
            const auto d=word(cell+0x28),descriptor=(d-common-0x400)/80;
            if(owner<owners||owner>=owners+owner_limit*owner_step||(owner-owners)%owner_step||word(owner+0x90)||
               at(owner+0x11)[0]>=8||at(owner+0x18)[0]!=1||half(owner+0x1C6)!=100||half(owner+0x1C))
                fail("side rise target is not a rebound unsponsored owner");
            const auto ordinal=(owner-owners)/owner_step,old_cell=retired[ordinal];
            const auto old_index=shape_ordinal(old_cell),new_index=shape_ordinal(cell);
            unsigned face=255;
            auto pose=support_top(cell,face);pose[1]+=4.f;
            bind_receiver(sasuke,pose);
            const auto [narrow,target_visits]=support_walk(cell,owner);
            require_support(cell,owner);
            if(target_visits!=1||half(common_role_count)!=count||word(common_roles+4*j)!=cell)
                fail("side rise actor did not acquire an exact rebound cell in the full registry");
            u32 capsule=0;
            for(unsigned i=0;i<half(contact::receiver_count);++i){
                const auto c=word(contact::receiver_registry+4*i);
                if(word(c+0x20)!=actor_body)continue;
                if(capsule||word(c+0x58)!=2||word(c+0x28)!=actor_bank+32+11*80)
                    fail("side rise actor receiving capsule changed");
                capsule=c;
            }
            if(!capsule)fail("side rise actor has no original receiving capsule");
            const auto anchor=std::array{word(actor_body+0x1F8),word(actor_body+0x1FC),word(actor_body+0x200)};
            const auto normal=point(actor_body+0x1EC);
            std::printf("side-ride: %u %u %u %u %u %u %u %08X %08X %08X %08X %08X %08X %08X\n",
                        member,ordinal,descriptor,old_index,new_index,face,narrow,word(actor_body+0x24),
                        supported_height(),anchor[1],word(owner+0x24),word(owner+0x130),word(cell+0x68),word(owner+0x184));
            for(unsigned step=1;step<=2;++step){
                const auto owner_before=point(owner+0x20);
                const auto actor_before=point(actor_body+0x20);
                const auto actor_yaw=half(actor_body+0x72);
                side_visit();
                const auto owner_after=point(owner+0x20),held_capsule=point(capsule+0x64);
                if(point(owner+0x2C)!=owner_before||owner_after[0]!=owner_before[0]||owner_after[2]!=owner_before[2]||
                   owner_after[1]<=owner_before[1]||scalar(owner+0x130)!=owner_after[1]||
                   half(common_role_count)!=count||word(common_roles+4*j)!=cell||word(cell+0x28)!=d)
                    fail("side rise did not advance the same original owner's world and history");
                require_support(cell,owner);
                const auto query=supported_height();
                u32 masked_y=0,masked_height=0,landing_y=0,landing=0;
                const auto verify_actor=[&](bool exact_lateral){
                    require_support(cell,owner);
                    const auto p=point(actor_body+0x20);
                    const auto tolerance=exact_lateral?0.f:0.001f;
                    if(half(actor_body+0x1C6)!=180||half(actor_body+0x72)!=actor_yaw||point(capsule+0x64)!=held_capsule||
                       point(actor_body+0x1EC)!=normal||word(actor_body+0x1F8)!=anchor[0]||
                       word(actor_body+0x1FC)!=anchor[1]||word(actor_body+0x200)!=anchor[2]||
                       !(std::abs(p[0]-actor_before[0])<=tolerance)||!(std::abs(p[2]-actor_before[2])<=tolerance))
                        fail("side rise carry/query changed receiving geometry, lateral pose, anchor or HP");
                };
                if(step==1){
                    const auto fork=ram;
                    at(contact::scene+235)[0]=1;
                    carry_prefix(false);
                    verify_actor(true);
                    masked_y=word(actor_body+0x24);
                    ram=fork;
                    at(contact::scene+235)[0]=1;
                    masked_height=supported_height();
                    landing=supported_landing();
                    landing_y=word(actor_body+0x24);
                    verify_actor(true);
                    ram=fork;
                }
                carry_prefix(true);
                verify_actor(false);
                const auto actor_after=point(actor_body+0x20);
                std::printf("side-rise-step: %u %u %u %08X %08X %08X %08X %08X %08X %08X %08X %08X %08X %u %08X %08X\n",
                            member,ordinal,step,word(owner+0x24),word(owner+0x30),word(owner+0x130),word(cell+0x68),
                            std::bit_cast<u32>(actor_before[1]),word(actor_body+0x24),query,masked_y,masked_height,landing_y,landing,
                            std::bit_cast<u32>(actor_after[0]-actor_before[0]),std::bit_cast<u32>(actor_after[2]-actor_before[2]));
            }
            ram=initial;
        }
        ram=rebinding;
    }
    void isolation(){
        // An independent fork of set0202: break row 0, not same-type row 1.
        const auto initial=ram;
        put16(at(owners+0x1C6),0);
        mute();
        for(unsigned visit=0;visit<2;++visit){
            side_visit();
            const auto target=at(owners+2*owner_step+0x82)[0],peer=at(owners+3*owner_step+0x82)[0];
            if(target!=1-visit||peer!=1||word(flags+4*4)!=1||half(contact::receiver_count)!=4||
               half(common_role_count)!=3||half(owners+owner_step+0x1C6)!=100||!at(shapes+2*shape_step)[0]||
               !at(shapes+3*shape_step)[0]||at(shapes)[0]||at(shapes+shape_step)[0])
                fail("side recovery single-break mask or same-type peer changed");
            std::printf("side-isolation: %u %u %u %u %u %u\n",visit,word(flags+4*4),target,peer,
                        half(owners+owner_step+0x1C6),half(contact::receiver_count));
        }
        ram=initial;
    }
    void recovery(unsigned member,const std::vector<u8>& side,unsigned n,unsigned n3,unsigned allocated,
                  const std::array<u8,21504>& collision,const ReceiverInput* rider){
        struct Life {
            unsigned companion=owner_limit,shape_count=0,hide=0,show=0,settle=0;
            float offset=0,rate=0;
            std::array<u32,2> models{};
        };
        std::array<Life,owner_limit> life{};
        std::array<u32,16> masks{};
        unsigned persistent=0,ordinary=0,common_n=0;
        const auto row=[&](unsigned i){return side.data()+2052+28*i;};
        const auto owner=[](unsigned i){return owners+i*owner_step;};
        std::array<u32,owner_limit> retired_roles{};
        if(rider){
            for(unsigned i=0;i<half(common_role_count);++i){
                const auto c=word(common_roles+4*i),o=word(c+0x20);
                if(o<owners||o>=owners+n*owner_step||(o-owners)%owner_step)fail("side recovery support owner extent");
                const auto j=(o-owners)/owner_step;
                if(row(j)[0]>=8)continue;
                if(retired_roles[j])fail("timed side recovery owner had duplicate support cells");
                retired_roles[j]=c;
            }
        }
        for(unsigned i=0;i<n;++i){
            const auto* src=row(i);
            if(src[2])continue;
            auto& l=life[i];
            const auto o=owner(i),t=src[0]+0u;
            if(src[1]>=32||masks[t]&(1u<<src[1]))fail("side recovery mask aliases distinct collision owners");
            masks[t]|=1u<<src[1];
            for(unsigned j=i+1;j<n;++j){
                const auto* other=row(j);
                if(other[2]&&other[0]==t&&other[1]==src[1]){
                    if(l.companion!=owner_limit)fail("side recovery duplicate mask companion");
                    l.companion=j;
                }
            }
            if(l.companion==owner_limit)fail("side recovery no later source companion for placement mask");
            l.models={word(o+0xE0),word(o+0xE4)};
            const auto key=4*at(o+0x1B)[0];
            for(unsigned j=0;j<4&&collision[key+j]!=255;++j){
                ++l.shape_count;
                if(t<8)common_n+=bool(be32(collision.data()+0x400+80*collision[key+j])&0x20);
            }
            if(t<8)ordinary+=l.shape_count;
            else ++persistent;
            put16(at(o+0x1C6),0); // Only the lethal inlet is declared.
        }
        mute();
        for(unsigned visit=0;visit<=664;++visit){
            CallObservation sound{0x800F0468};
            side_visit(visit==0?&sound:nullptr);
            if(visit==0){
                if(sound.count!=n3||sound.caller!=0x80081F48||half(contact::free_count+8)!=shape_limit||word(group48))
                    fail("side recovery break did not release the full collision roster");
                unsigned index=0;
                for(unsigned i=n;i--;){
                    if(row(i)[2])continue;
                    if(sound.first_arguments[index]!=owner(i)||sound.second_arguments[index++]!=115)
                        fail("side recovery positional cue lost its owner");
                }
                for(unsigned i=0;i<allocated;++i)
                    if(at(shapes+i*shape_step)[0])fail("side recovery retained a released shape cell");
            }
            if(half(contact::receiver_count)!=(visit<600?0:ordinary)||
               half(common_role_count)!=(visit<600?0:common_n)||
               half(contact::free_count+8)!=shape_limit-(visit<600?0:ordinary)||
               half(contact::free_count+6)!=(visit<2?0:persistent)||half(pools+18)!=(visit<2?0:2*persistent))
                fail("side recovery allocation/registry counts changed");
            std::array<u32,16> expected_masks{};
            for(unsigned i=0;i<n;++i){
                const auto* src=row(i);
                if(src[2])continue;
                auto& l=life[i];
                const auto o=owner(i),c=owner(l.companion),t=src[0]+0u;
                const bool timed=t<8,restored=timed&&visit>=600,retired=!timed&&visit>=2;
                const auto expected_phase=timed?1u:visit==0?2u:3u;
                const auto expected_timer=timed?(visit<600?600-visit:0):65535;
                if(bool(at(o)[0])==retired||at(o+0x18)[0]!=expected_phase||at(o+0x19)[0]!=unsigned(!restored)||
                   half(o+0x1C6)!=(restored?100:0)||half(o+0x1C)!=expected_timer||
                   at(o+0x82)[0]!=unsigned(restored)||bool(word(l.models[0]+12))==retired||
                   bool(word(l.models[1]+12))==retired||!at(c)[0]||at(c+0x18)[0]!=1)
                    fail("side recovery owner/companion phase or model lifetime changed");
                if(restored){
                    if(visit==600){
                        l.offset=-(100.f+scalar(type_table+20*t+8));
                        l.rate=-l.offset/60.f;
                    }else{
                        l.offset=std::min(0.f,l.offset+l.rate);
                        if(!l.offset&&!l.settle)l.settle=visit;
                    }
                    if(scalar(o+0x180)!=l.offset||scalar(o+0x184)!=(l.offset?l.rate:0.f))
                        fail("side recovery source rise recurrence changed");
                }
                if(l.settle&&std::memcmp(at(o+0x20),src+4,12))
                    fail("side recovery settled away from its authored position");
                const bool mask=word(flags+4*t)&(1u<<src[1]);
                const bool expected_mask=!l.settle;
                if(expected_mask)expected_masks[t]|=1u<<src[1];
                const bool show=visit==0||(l.settle&&visit>l.settle);
                if(mask!=expected_mask||bool(at(c+0x82)[0])!=show)
                    fail("side recovery mask or one-visit companion observation changed");
                if(!show&&!l.hide)l.hide=visit;
                if(show&&visit&&!l.show)l.show=visit;
            }
            for(unsigned i=0;i<16;++i)
                if(word(flags+4*i)!=expected_masks[i])fail("side recovery wrote outside the qualified mask bits");
            if(visit==2||visit==600||visit==664){
                // Retirement is an unlink, not merely a cleared active byte.
                auto c=word(group40);
                for(unsigned i=n;i--;){
                    if(visit>=2&&!row(i)[2]&&row(i)[0]>=8)continue;
                    if(c!=owner(i))fail("side recovery bucket lost live or retained retired owner");
                    c=word(c+8);
                }
                if(c)fail("side recovery bucket suffix");
                c=word(0x8043B5A0-21264+4);
                for(unsigned i=0;i<n;++i){
                    if(row(i)[2]||row(i)[0]>=8)continue;
                    if(c!=owner(i))fail("side recovery world list lost an owner or retained a retired one");
                    c=word(c+0x108);
                }
                if(c)fail("side recovery world list suffix");
                if(persistent){
                    auto o=word(contact::free_head+12), d=word(pools+12);
                    for(unsigned i=n;i--;){
                        if(row(i)[2]||row(i)[0]<8)continue;
                        if(o!=owner(i))fail("side recovery class-3 free list changed return identity");
                        o=word(o+4);
                    }
                    for(unsigned i=0;i<n;++i){
                        if(row(i)[2]||row(i)[0]<8)continue;
                        for(unsigned j=2;j--;){
                            if(d!=life[i].models[j])fail("side recovery model free list changed return identity");
                            d=word(d);
                        }
                    }
                    if(o||d)fail("side recovery retired-cell free list suffix");
                }
            }
            if(visit==600||visit==664){
                std::array<unsigned,owner_limit> seen{};
                u32 c=word(group48);
                for(unsigned i=0;i<ordinary;++i){
                    if(c<shapes||c>=shapes+shape_limit*shape_step||(c-shapes)%shape_step||!at(c)[0])
                        fail("side recovery rebound shape escaped original registry");
                    const auto o=word(c+0x20);
                    if(o<owners||o>=owners+n*owner_step||(o-owners)%owner_step||word(c+0x24)!=o||word(c+0x60)!=o+0x114)
                        fail("side recovery rebound shape escaped original owner");
                    const auto index=(o-owners)/owner_step;
                    auto& ordinal=seen[index];
                    const auto count=life[index].shape_count;
                    if(row(index)[0]>=8||ordinal>=count)fail("side recovery rebound wrong shape owner");
                    const auto key=collision[4*at(o+0x1B)[0]+count-1-ordinal++];
                    if(word(c+0x28)!=common+0x400+80*key)fail("side recovery rebound wrong descriptor");
                    c=word(c+8);
                }
                if(c)fail("side recovery rebound shape suffix");
                for(unsigned i=0;i<n;++i)
                    if(row(i)[0]<8&&seen[i]!=life[i].shape_count)fail("side recovery lost a repeated descriptor cell");
                if(rider&&visit==600&&common_n)rise_support(member,*rider,retired_roles);
            }
        }
        std::printf("side-recovery: %u %u %u %u %u\n",member,n3-persistent,persistent,ordinary,common_n);
        for(unsigned i=0;i<n;++i){
            if(row(i)[2])continue;
            const auto& l=life[i];
            const bool timed=row(i)[0]<8;
            std::printf("side-life: %u %u %u %u %u %u %u %u %08X %u\n",member,i,l.companion,l.shape_count,
                        timed?600:0,l.settle,l.hide,l.show,std::bit_cast<u32>(l.rate),timed?0:2);
        }
    }
public:
    Census(const Module& m,const std::vector<u8>& pristine):mod(m),ram(pristine){}
    void run(unsigned member,const std::vector<u8>& side,const std::array<u8,21504>& collision,bool recover,
             const ReceiverInput* sasuke){
        CPUState boot{};
        const auto boot_cut=mod.run_to_or_return(boot,ram,0x8002A3E4,{0x8002A51C,0x80200318},nullptr,4096);
        if(!boot_cut||boot.gqr[5]!=0x00070007)fail("side census source boot GQR");
        std::copy(std::begin(boot.gqr),std::end(boot.gqr),gqr.begin());
        art::Setup setup(mod,ram);
        setup.init_heap(heap_limit);
        call(0x80254288);
        store(0x80436380,pools);
        const unsigned sectors=be32(side.data()),n=be32(side.data()+2048);
        if(sectors<2||sectors>bank_limit+1||be32(side.data()+4)!=1||!n||n>owner_limit)
            fail("side census original header extent");
        unsigned total=2048;
        for(unsigned i=0;i<sectors;++i){
            const unsigned span=be32(side.data()+4+4*i);
            if(!span||span>(side.size()-total)/2048)fail("side census original sector extent");
            total+=span*2048;
        }
        if(total!=side.size())fail("side census unexpected side suffix");
        std::array<u32,shape_limit> class2{},class3{},class4{};
        unsigned n2=0,n3=0;
        for(unsigned i=0;i<n;++i){
            const auto* row=side.data()+2052+28*i;
            if(row[0]>=12||row[2]>1||row[3]>=sectors-1)fail("side census original row extent");
            if(row[2])class2[n2++]=owners+i*owner_step;
            else class3[n3++]=owners+i*owner_step;
        }
        for(unsigned i=0;i<shape_limit;++i)class4[i]=shapes+i*shape_step;
        make_pool(2,class2,n2);make_pool(3,class3,n3);make_pool(4,class4,shape_limit);
        init_render_pool(0,models,n2+2*n3,0x8036F5B8);
        init_render_pool(1,archives,sectors-1,0x8036F5D4);
        std::memcpy(at(resources),side.data(),side.size());
        std::memcpy(at(common),collision.data(),collision.size());
        store(0x8043B5A0-31820,stage_files);store(stage_files+24,resources);
        store(0x8043B5A0-31852,contact::scene);store(contact::scene+152,common);
        for(unsigned i=0;i<6;++i)store(0x803C4E84+4*i,0x81020000+0x1E00*i);
        CallObservation load{0x80040EB4};
        call(0x8004279C,0,&load);
        if(load.count!=sectors-1||load.caller!=0x80042840||word(stage_files+24)!=resources+2048||half(pools+28+18))
            fail("side census did not load all original banks");
        unsigned offset=4096;
        std::printf("side-banks: %u %u",member,sectors-1);
        std::array<unsigned,bank_limit> records{};
        for(unsigned i=0;i<sectors-1;++i){
            const auto a=archives+32*i;
            if(word(0x803BB38C+8*i)!=a||word(a+20)!=resources+offset||!at(a+10)[0])
                fail("side census archive descriptor lost its original bank identity");
            records[i]=half(a+26);
            std::printf(" %u",records[i]);
            offset+=2048*be32(side.data()+8+4*i);
        }
        std::printf("\n");

        for(unsigned i=0;i<16;++i)store(flags+4*i,0xA5A5A5A5);
        CallObservation dispatch{0x80081980};
        call(0x800818D0,0,&dispatch);
        if((dispatch.count&&(dispatch.count!=n||dispatch.caller!=0x80081904))||
           half(pools+18)||half(contact::free_count+4)||
           half(contact::free_count+6)||word(group40)!=owners+(n-1)*owner_step)
            fail("side census original placement walk did not construct every row");
        for(unsigned i=0;i<std::min(8u,dispatch.count);++i)
            if(dispatch.first_arguments[i]!=resources+2052+28*i)fail("side census original dispatch order");
        for(unsigned i=0;i<16;++i)
            if(word(flags+4*i))fail("side census placement walk did not reset all 16 masks");
        std::array<u32,owner_limit*2> roots{};
        unsigned root_count=0;
        for(unsigned i=0;i<n;++i){
            const u32 o=owners+i*owner_step;
            const auto* row=side.data()+2052+28*i;
            const bool model_only=row[2];
            const u32 row_type=type_table+20*row[0];
            if(!at(o)[0]||at(o+1)[0]!=40||at(o+2)[0]!=(model_only?2:3)||at(o+0x83)[0]!=22||
               word(o+8)!=(i?o-owner_step:0)||word(o+4)!=(i+1<n?o+owner_step:0)||word(o+0x0C)!=0x80081D98||
               at(o+0x11)[0]!=row[0]||at(o+0x12)[0]!=row[2]||at(o+0x13)[0]!=row[1]||at(o+0x18B)[0]!=row[3]||
               word(o+0x100)!=(model_only?0u:0x80082060u)||at(o+0x18)[0]||row[2]>=records[row[3]]||
               half(o+0x1C6)!=(model_only?0:half(row_type))||at(o+0x1B)[0]!=(model_only?0:half(row_type+2))||
               half(o+0x188)!=(model_only?0:half(row_type+16)))
                fail("side census original row-to-owner correspondence changed");
            if(!model_only&&word(o+0x8C)!=o)fail("side census collision owner does not own itself");
            for(unsigned j=0;j<(model_only?1u:2u);++j){
                const auto d=word(o+0xE0+4*j);
                if(d<models||d>=models+44*(n2+2*n3)||(d-models)%44)
                    fail("side census model descriptor outside pool");
                const auto r=word(d+12);
                if(r<art::payload||r>=heap_limit||
                   std::find(roots.begin(),roots.begin()+root_count,r)!=roots.begin()+root_count)
                    fail("side census model instance escaped ownership");
                roots[root_count++]=r;
            }
        }
        CallObservation first_update{0x80081DD4},world{0x80082060};
        call(0x80089A94,40,&first_update);
        call(0x80089554,0,&world);
        if(first_update.count!=n||first_update.caller!=0x80081DC0||world.count!=n3||
           (world.count&&world.caller!=0x80089590)){
            std::fprintf(stderr,"side census callbacks %u: bucket=%u/%08X world=%u/%08X\n",member,
                         first_update.count,first_update.caller,world.count,world.caller);
            fail("side census first-phase callback coverage changed");
        }
        for(unsigned i=0;i<std::min(8u,n);++i)
            if(first_update.first_arguments[i]!=owners+(n-1-i)*owner_step)fail("side census reverse bucket visit order");
        for(unsigned i=0;i<n3;++i)
            if(i>=8||world.first_arguments[i]!=class3[i])fail("side census registered world visit order");
        for(unsigned i=0;i<2;++i){call(0x8008A28C);call(0x80089AEC);}
        const unsigned allocated=shape_limit-half(contact::free_count+8);
        if(allocated>shape_limit||half(contact::receiver_count)>allocated||half(common_role_count)>allocated)
            fail("side census shape registry escaped declared pool");
        std::array<unsigned,shape_limit> roles{},descriptors{};
        auto cell_index=[&](u32 c){
            if(c<shapes||c>=shapes+allocated*shape_step||(c-shapes)%shape_step)
                fail("side census shape registry cell escaped original publication");
            return (c-shapes)/shape_step;
        };
        const auto mark=[&](u32 list,unsigned count,unsigned bit){
            for(unsigned i=0;i<count;++i){
                auto& role=roles[cell_index(word(list+4*i))];
                if(role&bit)fail("side census shape was duplicated in one registry");
                role|=bit;
            }
        };
        mark(contact::receiver_registry,half(contact::receiver_count),1);
        mark(common_roles,half(common_role_count),2);
        auto cell=word(group48);
        for(unsigned i=allocated;i;--i){
            if(cell_index(cell)!=i-1||!at(cell)[0])fail("side census allocated shape chain changed");
            const auto d=word(cell+0x28);
            if(d<common+0x400||d>common+collision.size()-80||(d-common-0x400)%80||
               word(cell+0x58)!=at(d)[0])
                fail("side census shape descriptor escaped common bank");
            descriptors[i-1]=(d-common-0x400)/80;
            cell=word(cell+8);
        }
        if(cell)fail("side census unexpected shape chain suffix");
        std::printf("side-census: %u %u %u %u %u %u %u %u\n",member,n,n2,n3,root_count,allocated,
                    half(contact::receiver_count),half(common_role_count));
        unsigned next=0;
        for(unsigned i=0;i<n;++i){
            const auto* row=side.data()+2052+28*i;
            const auto o=owners+i*owner_step;
            if(at(o+0x18)[0]!=1||at(o+0x82)[0]!=1||std::memcmp(at(o+0x20),row+4,12))
                fail("side census original first world did not resolve the authored position");
            std::printf("side-owner: %u %u %u %u %u %u %u %u %u %u",member,i,at(o+0x11)[0],at(o+0x13)[0],
                        at(o+0x12)[0],at(o+0x18B)[0],at(o+2)[0],half(o+0x1C6),half(o+0x188),at(o+0x1B)[0]);
            const auto key=4*at(o+0x1B)[0];
            if(!row[2]){
                for(unsigned j=0;j<4&&collision[key+j]!=255;++j){
                    const auto c=shapes+next*shape_step;
                    if(next>=allocated||word(c+0x20)!=o||word(c+0x24)!=o||word(c+0x60)!=o+0x114||
                       descriptors[next]!=collision[key+j])fail("side census collision owner or descriptor changed");
                    std::printf(" %u/%u",descriptors[next],roles[next]);
                    ++next;
                }
            }
            std::printf("\n");
        }
        if(next!=allocated)fail("side census left allocated collision cells unclaimed");
        if(recover){
            if(member==2429)isolation();
            recovery(member,side,n,n3,allocated,collision,sasuke);
        }
        if(sasuke&&!recover)support_census(member,*sasuke);
    }
};
} // namespace

int main(int argc,char** argv){
    const bool ride=argc==4&&!std::strcmp(argv[3],"--rise-support");
    const bool recovery=ride||(argc==4&&!std::strcmp(argv[3],"--recovery"));
    const bool support=ride||(argc==4&&!std::strcmp(argv[3],"--support"));
    if(argc!=3&&!recovery&&!support)
        fail("usage: side-census qualified.dol unmodified-recomp.so [--recovery|--support|--rise-support] < qualified-inputs");
    std::array<u8,21504> collision{};
    if(std::fread(collision.data(),1,collision.size(),stdin)!=collision.size())fail("side census common collision input extent");
    ReceiverInput sasuke{};
    if(support&&(std::fread(sasuke.hit.data(),1,sasuke.hit.size(),stdin)!=sasuke.hit.size()||
                 std::fread(sasuke.spec.data(),1,sasuke.spec.size(),stdin)!=sasuke.spec.size()))
        fail("side census support input extent");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned n=2419;n<=2479;++n){
        std::array<u8,8> header{};
        if(std::fread(header.data(),1,header.size(),stdin)!=header.size()||be32(header.data())!=n)
            fail("side census member order");
        const auto size=be32(header.data()+4);
        if(size<6144||size>(1u<<18))fail("side census archive input size");
        std::vector<u8> side(size);
        if(std::fread(side.data(),1,size,stdin)!=size)fail("side census archive input extent");
        Census(mod,original).run(n,side,collision,recovery,support?&sasuke:nullptr);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("side census unexpected input suffix");
}
