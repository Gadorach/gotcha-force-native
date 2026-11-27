// Original allocator in declared RAM, followed by bounded contact-visual updates.
#include "retail_contact_effect_archive.hpp"
#include <cmath>

namespace {
using namespace retail_fixture;
using namespace retail_contact_effect_archive;
namespace contact=retail_stationary_contact;
constexpr u32 parent=0x81028000,effect=0x81030000,cells=0x81032000,pools=0x81033000;
constexpr u32 shape=0x81034000,origin=0x81035000,visual_object=0x81036000;
constexpr u32 visual=0x81037000,link=visual+0x40,table=visual+0x80,empty=visual+0x400;
constexpr u32 active_head=0x803C706C,scene=contact::scene,attack_count=0x8043B5A0-21328;
constexpr u32 row=pools+3*28,cell_stride=0x80;
constexpr u32 render_context=0x81038000,view_heads=0x803C6F1C,source_owner=0x81036800;
constexpr u32 second_parent=0x8102A000,contender=0x8102C000;
constexpr unsigned burst_masks[]{255,255,255,255,255,65535,65535,65535,65535,65535,
                                  65535,65535,65535,65535,65535,65535,65280,0,0,0};
constexpr unsigned piece_masks[]{3,7,15,31,63,63,57,49,33,1,0,0,0};
struct Case {unsigned selector,bursts,entries,count,total,retire;};
struct SourceCase {unsigned selector,parent_lane,credit_lane,scene_mode;};
struct SourceModels {
    struct Model {unsigned id,descriptor,root;};
    std::array<Model,2> models{};
    unsigned count=0;
};
constexpr SourceCase source_cases[]{
    {0,0,1,0},{1,0,1,0},{1,1,0,0},{1,1,0,3},{8,1,0,0},
};
constexpr Case cases[]{
    {1,0,0,16,928,20},
    {1,31,511,1,96,19},
    {8,0,0,8,288,13},
    {8,32,512,8,288,13},
};

class Check {
    const Module& mod;
    const Case& spec;
    std::vector<u8> ram;
    Setup setup{mod,ram};
    u8* at(u32 address){return contact::at(ram,address);}
    u32 word(u32 address){return be32(at(address));}
    u16 half(u32 address){return contact::half(ram,address);}
    void call(u32 entry,u32 arg=0){setup.call(entry,arg);}

    bool heap_owns_block(u32 address=block){
        auto cur=word(arena+8);
        for(unsigned i=0;i<64&&cur;++i){
            if(cur<block||cur>=arena_end||(cur&31))fail("contact visual heap list escaped the declared arena");
            if(cur==address)return true;
            cur=word(cur+4);
        }
        if(cur)fail("contact visual allocation list exceeded the walk bound");
        return false;
    }
    bool heap_free_covers(u32 address,u32 length){
        auto cur=word(arena+4);
        for(unsigned i=0;i<64&&cur;++i){
            if(cur<block||cur>arena_end-32||(cur&31))fail("contact visual free list escaped the declared arena");
            const auto size=word(cur+8);
            if(size<32||size>arena_end-cur||(size&31))fail("contact visual free block extent changed");
            if(address>=cur&&length<=size&&address-cur<=size-length)return true;
            cur=word(cur+4);
        }
        if(cur)fail("contact visual free list exceeded the walk bound");
        return false;
    }
    u32 request_effect(u32 attack){
        CPUState request{};request.gpr[3]=attack;request.gpr[4]=origin;request.gpr[5]=spec.selector;
        mod.run(request,ram,0x80019550);
        return request.gpr[3];
    }
    void allocation(bool at_frontier=false){
        if(word(effect+0xDC)!=visual_object||word(row+8)!=visual_object||word(row+12)||half(row+18)||
           at(visual_object)[8]!=3||at(visual_object)[9]!=1||word(visual_object+12)!=payload||
           !heap_owns_block()||word(block+8)!=spec.total||
           (at_frontier&&(word(arena+8)!=block||word(block)||word(block+4)||word(arena+4)!=block+spec.total||
            word(block+spec.total)||word(block+spec.total+4)||word(block+spec.total+8)!=arena_end-block-spec.total)))
            fail("original contact visual did not own the aligned heap block");
    }

    void no_collider(){
        if(half(contact::free_count+8)!=1||word(contact::free_head+16)!=shape||word(active_head+49*4)||
           half(attack_count)||half(contact::receiver_count)||std::memcmp(at(effect+0x20),at(origin),12))
            fail("bounded contact visual published a collider or moved its origin");
    }
    void retirement(unsigned descriptors){
        if(word(effect+0xDC)||word(row+8)||word(row+12)!=visual_object||half(row+18)!=1||
           at(visual_object)[9]||word(visual_object+12)||heap_owns_block()||word(arena+4)!=block||
           word(block+8)!=(spec.selector==1?spec.total:arena_end-block)||
           word(active_head+4*bucket)||half(contact::free_count+4)!=1||word(contact::free_head+8)!=effect||
           half(pools+18)!=descriptors)
            fail("contact visual failed to return a heap block, its type-3 cell or a battle resource");
        returned_descriptors(descriptors);
    }
    void returned_descriptors(unsigned descriptors){
        auto cur=word(pools+12);
        unsigned seen=0;
        for(unsigned i=0;i<descriptors;++i){
            if(cur<cells||(cur-cells)%cell_stride||(cur-cells)/cell_stride>=descriptors||word(cur+12))
                fail("contact visual returned a descriptor with an unexpected address or joint");
            const auto bit=1u<<((cur-cells)/cell_stride);
            if(seen&bit)fail("contact visual returned a duplicate descriptor");
            seen|=bit;cur=word(cur);
        }
        if(cur)fail("contact visual returned a malformed descriptor list");
    }
    void check_burst_joints(){
        u32 joint_root=word(cells+12);
        if(joint_root<payload+spec.total||joint_root>=arena_end||word(cells+20)!=2)
            fail("original burst did not attach the record-2 joint group");
        u32 cur=word(joint_root+16);
        for(unsigned i=0;i<spec.count;++i){
            if(cur<payload+spec.total||cur>=arena_end||cur==joint_root){
                std::fprintf(stderr,"burst joint index=%u address=%08X root=%08X\n",i,cur,joint_root);
                fail("original burst joint group was shorter than the admitted burst");
            }
            cur=word(cur+8);
        }
        if(cur)fail("original burst joint group exceeded the admitted burst");
    }
    std::array<u32,16> draw_joints(bool burst,bool retired){
        std::array<u32,16> result{};
        if(retired)return result;
        if(burst){
            check_burst_joints();
            auto cur=word(word(cells+12)+16);
            for(unsigned i=0;i<spec.count;++i){
                result[i]=cur;cur=word(cur+8);
            }
        }else{
            for(unsigned i=0;i<8;++i)result[i]=word(cells+i*cell_stride+12);
        }
        return result;
    }
    bool finite(u32 address){return std::isfinite(std::bit_cast<float>(word(address)));}
    void check_piece_transform(u32 joint,unsigned i,unsigned visit){
        const auto particle=payload+32*i;
        if(!joint||joint<block||joint>=arena_end||word(cells+i*cell_stride+20)!=53+unsigned(half(particle+30)))
            fail("contact piece lost its model-record binding");
        for(unsigned j=0;j<12;++j)
            if(!finite(joint+68+4*j))fail("contact piece transform is not finite");
        if(visit==1&&(half(particle+24)||half(particle+26)||half(particle+28)))
            fail("contact piece first-visit scale check needs the declared zero-angle sample");
        for(unsigned axis=0;axis<3;++axis){
            if(word(effect+0x114+12+16*axis)!=word(origin+4*axis)||
               word(joint+68+12+16*axis)!=word(origin+4*axis))
                fail("piece transform did not preserve its world origin");
            if(visit==1&&word(joint+68+20*axis)!=word(particle+12+4*axis))
                fail("unrotated contact piece did not apply its local scale");
        }
    }
    void set_origin(const contact::Point& point){
        for(unsigned i=0;i<point.size();++i)put_float(at(origin+4*i),point[i]);
    }
    u32 source_cell(unsigned index){
        const auto cell=word(effect+0xE0+4*index);
        const unsigned admitted=spec.selector==8?8:1;
        if(cell<cells||(cell-cells)%cell_stride||(cell-cells)/cell_stride>=admitted)
            fail("source model escaped its admitted descriptor list");
        return cell;
    }
    unsigned source_view(unsigned mask){
        setup.prepare_view_zero();
        no_collider();
        auto queued=word(draw_heads+5*4);
        unsigned count=0;
        for(unsigned i=spec.selector==8?8:1;i>0;--i){
            if(!(mask&(1u<<(i-1))))continue;
            const auto expected=source_cell(i-1);
            if(queued!=expected||!word(expected+12))
                fail("contact visual source models did not reach the expected lane 5 queue");
            queued=word(expected+40);
            ++count;
        }
        if(queued)fail("contact visual source model queue did not terminate");
        return count;
    }
    unsigned source_tick(CallObservation& model){
        CPUState visit{};visit.gpr[3]=bucket;
        if(mod.run_to_or_return(visit,ram,0x80089A94,{0x80264290},&model,4096)){
            std::fprintf(stderr,"source model assertion %08X/%u\n",visit.lr-4,visit.gpr[4]);
            fail("original common effect model update");
        }
        return model.count;
    }
    SourceModels source_first_update(const Source& source,const SourceCase& route,u32 attack){
        constexpr u32 model_entry=0x8024F834;
        const auto scene_symbol=word(source_owner+16),records=word(scene_symbol);
        const auto extent=be32(source.bytes.data()+4),data=source_bank+32;
        const auto in_data=[&](u32 address,u32 length){return address>=data&&address<=data+extent-length;};
        if(at(effect+0x88)[0]!=route.parent_lane||at(actor+0x88)[0]!=route.credit_lane||
           word(effect+0x90)!=actor||word(effect+0x8C)!=attack)
            fail("contact visual did not copy the parent lane independently of its credited actor");
        CallObservation model{model_entry};
        source_tick(model);
        const bool pieces=route.selector==8,burst=route.selector==1;
        const unsigned count=pieces?2:1,first_record=pieces?53:!burst?21:2+(route.scene_mode==3?0:route.parent_lane);
        const auto caller=pieces?0x800072B0u:burst?0x800071F8u:0x80006FF8u;
        if(model.count!=count||model.caller!=caller||at(effect+0x18)[0]!=1||at(effect+0x82)[0]!=1||
           (pieces&&half(effect+0x1C)!=3)||(burst&&word(effect+0x158)!=1)||word(source_globals)!=source_owner)
            fail("contact visual did not initialize the expected first-visit models");
        if(burst){
            const auto group=word(source_cell(0)+12);
            if(group<block||group>arena_end-20)fail("contact visual lost its loaded joint group");
            const auto child=word(group+16);
            if(child<block||child>arena_end-12||child==group||word(child+8)||
               half(scene+484)!=32||half(scene+486)!=512)
                fail("one-entry burst did not return with one loaded joint child");
        }
        source_view(pieces?3:1);
        SourceModels receipt{};
        receipt.count=count;
        for(unsigned i=0;i<count;++i){
            const unsigned record=first_record+i;
            const auto descriptor=word(records+record*4),cell=source_cell(i);
            const auto root=model.first_arguments[i],joint=word(cell+12);
            if(!in_data(descriptor,16)||!in_data(root,64)||word(descriptor)!=root||
               word(cell+20)!=record||joint<block||joint>=arena_end||(i&&joint==word(source_cell(0)+12)))
                fail("contact visual bound a different source record or failed to attach its original joint");
            receipt.models[i]={record,descriptor-data,root-data};
        }
        return receipt;
    }
public:
    Check(const Module& m,const std::vector<u8>& original,const Case& c,bool render=false,
          const Source* source=nullptr,const SourceCase* source_case=nullptr):mod(m),spec(c),ram(original){
        setup.init_heap();
        at(parent)[0]=1;at(actor)[0]=1;at(effect)[2]=2;at(shape)[2]=4;
        put32(at(parent+0x90),actor);put_float(at(parent+0xB4),1.f);
        put_float(at(actor+0x1DC8),1.f);
        set_origin({260.f,120.f,880.f});
        put32(at(0x8043B5A0-31852),scene);
        put16(at(scene+484),u16(spec.bursts));put16(at(scene+486),u16(spec.entries));
        put32(at(0x803C4E84),actor);
        for(unsigned i=1;i<6;++i)put32(at(0x803C4E84+4*i),0x8101A000);
        put32(at(0x80436380),pools);put32(at(0x8043B5A0-21024),pools);
        const unsigned descriptors=spec.selector==8?8:1;
        put32(at(pools+12),cells);put16(at(pools+18),u16(descriptors));
        for(unsigned i=1;i<descriptors;++i)put32(at(cells+(i-1)*cell_stride),cells+i*cell_stride);
        setup.init_visual_header(pools,visual_object);
        put32(at(contact::free_head+8),effect);put32(at(contact::free_tail+8),effect);
        put16(at(contact::free_count+4),1);
        put32(at(contact::free_head+16),shape);put32(at(contact::free_tail+16),shape);
        put16(at(contact::free_count+8),1);
        if(source){
            if(!source_case)fail("missing contact visual archive route case");
            at(parent+0x88)[0]=u8(source_case->parent_lane);
            at(actor+0x88)[0]=u8(source_case->credit_lane);
            at(scene+81)[0]=u8(source_case->scene_mode);
            setup.publish(*source,pools,source_owner,render_context);
        }else{
            put32(at(source_globals),visual);put32(at(visual+16),link);put32(at(link),table);
            for(auto id:{53u,54u})put32(at(table+4*id),empty);
        }
        if(!source&&(spec.selector==1||render)){
            constexpr u32 record=empty+0x20,joint=empty+0x80;
            // Original HSD ID pool and declared single-joint template
            // (no children, animation or geometry).
            call(0x80259644);
            if(spec.selector==8){
                for(auto id:{53u,54u})put32(at(table+4*id),record);
            }else put32(at(table+4*2),record);
            put32(at(record),joint);
            for(unsigned i=0;i<3;++i)put_float(at(joint+32+4*i),1.f);
        }
        if(render)put32(at(0x8043B5A0-31856),render_context);

        if(request_effect(parent)!=effect||word(active_head+4*bucket)!=effect||half(pools+18)||
           half(contact::free_count+4)||at(effect+0x13)[0]!=descriptors)
            fail("fixed-bucket visual constructor did not claim its battle resources");
    }

    void burst_frontier(unsigned scenario){
        CPUState visit{};visit.gpr[3]=bucket;
        mod.run_to(visit,ram,0x80089A94,{0x80007198});
        allocation(true);
        no_collider();
        if(visit.lr!=0x80019B90||visit.gpr[3]!=visual||visit.gpr[4]!=cells||visit.gpr[5]!=2||visit.gpr[6]!=spec.count||
           at(effect+0x18)[0]!=1||at(effect+0x82)[0]||half(scene+484)!=spec.bursts+1||half(scene+486)!=spec.entries+spec.count)
            fail("selector 1 changed its original scene charge or model preparation frontier");
        std::printf("effect-storage-frontier: %u %u %u %u %u %u\n",
                    scenario,word(block+8),half(scene+484),half(scene+486),visit.gpr[5],visit.gpr[6]);
    }

    void lifetime(unsigned scenario){
        // Only the fixed bucket update. Do not call the drawing services;
        // selector 8 deliberately resolves its model records to null.
        for(unsigned visit=1;visit<=spec.retire;++visit){
            call(0x80089A94,bucket);
            no_collider();
            const bool burst=spec.selector==1,retired=visit==spec.retire;
            const bool charged=burst&&visit<spec.retire-2;
            const unsigned phase=visit<spec.retire-2?1:visit==spec.retire-2?2:3;
            const auto mask=burst?word(effect+0x158):unsigned(half(effect+0x1C));
            const unsigned expected_mask=!burst?piece_masks[visit-1]:
                spec.count==1?unsigned(charged):burst_masks[visit-1];
            if(at(effect+0x18)[0]!=phase||bool(at(effect)[0])==retired||at(effect+0x82)[0]!=unsigned(phase!=3)||
               mask!=expected_mask||half(scene+484)!=spec.bursts+unsigned(charged)||
               half(scene+486)!=spec.entries+spec.count*unsigned(charged))
                fail("contact visual changed its phase, activity mask or scene charge");
            if(!retired){
                allocation(!burst);
                if(half(pools+18)||half(contact::free_count+4)||word(active_head+4*bucket)!=effect)
                    fail("contact visual returned battle resources before retirement");
                if(burst)check_burst_joints();
            }else{
                retirement(burst?1:8);
            }
            std::printf("effect-storage-visit: %u %u %u %u %u %u %u %u\n",scenario,visit,phase,mask,
                        at(effect+0x82)[0],half(scene+484),half(scene+486),retired?0:word(block+8));
        }
    }

    void render_prepare(){
        for(unsigned visit=1;visit<=spec.retire;++visit){
            call(0x80089A94,bucket);
            const bool burst=spec.selector==1,retired=visit==spec.retire;
            const auto mask=burst?word(effect+0x158):unsigned(half(effect+0x1C));
            if(mask!=(burst?burst_masks[visit-1]:piece_masks[visit-1]))
                fail("contact visual render records changed the update mask");
            const auto phase=at(effect+0x18)[0],enabled=at(effect+0x82)[0];
            if(bool(at(effect)[0])==retired||enabled!=unsigned(visit<spec.retire-1))
                fail("contact visual render preparation needs the verified update lifecycle");
            if(!retired)allocation();
            const auto joints=draw_joints(burst,retired);
            if(!retired&&word(effect+0x100)!=(burst?0u:0x800197E8))
                fail("contact visual changed its early transform callback");
            for(unsigned view:{1u,0u}){
                at(render_context+52)[0]=u8(view);
                call(0x80009790);
                call(0x80089554);
                call(0x8008A16C);
                call(0x80089CD0,24);
                std::array<std::array<u8,48>,16> before{};
                unsigned prepared=0;
                for(unsigned i=0;i<joints.size();++i){
                    if(joints[i])std::memcpy(before[i].data(),at(joints[i]+68),48);
                    if(!burst&&(mask&(1u<<i))){
                        check_piece_transform(joints[i],i,visit);
                        prepared|=1u<<i;
                    }
                }
                const bool collected=visit<spec.retire-1;
                if(word(view_heads)!=(collected?effect:0)||word(view_heads+4)||word(effect+0x110))
                    fail("contact visual was collected under the wrong view or after disabling");
                for(unsigned lane=0;lane<6;++lane)
                    if(word(draw_heads+4*lane))fail("contact visual transform queued before its view callback");
                if(!retired&&word(effect+0x10C)!=0x80019824)
                    fail("contact visual lost its original family draw callback");
                const auto scene_bursts=half(scene+484),scene_entries=half(scene+486);
                call(0x80089EB4);
                for(unsigned i=0;i<joints.size();++i){
                    if(!joints[i])continue;
                    if(burst&&!view&&(mask&(1u<<i))){
                        prepared|=1u<<i;
                        for(unsigned axis=0;axis<3;++axis)
                            if(!finite(joints[i]+68+12+axis*16)||
                               word(joints[i]+68+12+axis*16)!=word(payload+56*i+4*axis))
                                fail("burst child translation did not follow its active entry position");
                    }else if(std::memcmp(before[i].data(),at(joints[i]+68),48))
                        fail("contact visual registration rewrote an unrelated joint matrix");
                }
                const unsigned expected=view?0u:burst?unsigned(mask!=0):std::popcount(mask);
                unsigned queued=0,queued_mask=0,limit=burst?1:8;
                u32 head=word(draw_heads+5*4);
                for(u32 cur=head;cur;cur=word(cur+40)){
                    if(cur<cells||(cur-cells)%cell_stride||(cur-cells)/cell_stride>=(burst?1:8)||++queued>8)
                        fail("contact visual draw queue escaped the admitted descriptors");
                    const unsigned i=(cur-cells)/cell_stride,bit=1u<<i;
                    if(queued_mask&bit||i>=limit||!word(cur+12)||at(cur+16)[0]!=1)
                        fail("contact visual draw list changed descriptor order or lacked a joint");
                    limit=i;
                    queued_mask|=bit;
                }
                for(unsigned lane=0;lane<5;++lane)
                    if(word(draw_heads+4*lane)){
                        std::fprintf(stderr,"selector=%u visit=%u view=%u lane=%u head=%08X\n",
                                     spec.selector,visit,view,lane,word(draw_heads+4*lane));
                        fail("contact visual used an unexpected draw lane");
                    }
                if(queued!=expected||queued_mask!=(expected?(burst?1:mask):0)||
                   at(effect+0x18)[0]!=phase||at(effect+0x82)[0]!=enabled||
                   half(scene+484)!=scene_bursts||half(scene+486)!=scene_entries||
                   mask!=(burst?word(effect+0x158):unsigned(half(effect+0x1C))))
                    fail("contact visual preparation changed the wrong mask or budget");
                no_collider();
                if(head){
                    // The consumer configures light/fog before walking its
                    // lane. Do not touch the GX FIFO in the host fixture.
                    CPUState consume{};consume.gpr[3]=5;
                    mod.run_to(consume,ram,0x80009838,{0x8025A210});
                    if(consume.lr!=0x80009880||consume.gpr[3]||word(draw_heads+5*4)!=head)
                        fail("contact visual did not reach the original render-state frontier");
                }
                std::printf("effect-render: %u %u %u %u %u %u %u %u\n",spec.selector,visit,view,mask,
                            prepared,unsigned(collected),queued,queued_mask);
            }
        }
    }

    void source_route(const Source& source,const SourceCase& route,unsigned scenario){
        call(0x80254288);
        const auto first=source_first_update(source,route,parent);
        for(unsigned i=0;i<first.count;++i){
            const auto& m=first.models[i];
            std::printf("effect-source: %u %u %u %u %u %u %u %u %u\n",source.slot,scenario,route.selector,
                        route.parent_lane,route.credit_lane,route.scene_mode,m.id,m.descriptor,m.root);
        }
    }

    void source_lifetime(const Source& source,SourceCase route){
        call(0x80254288);
        const bool burst=route.selector==1,pieces=route.selector==8;
        const unsigned retire=burst?19:pieces?13:23,descriptors=pieces?8:1;
        const auto archive_context=word(source_owner+12),archive_scene=word(source_owner+16);
        const std::vector<u8> archive_after_relocation(at(source_bank),at(source_bank)+source.bytes.size());
        for(u32 source_parent:{second_parent,contender}){
            at(source_parent)[0]=1;
            put32(at(source_parent+0x90),actor);put_float(at(source_parent+0xB4),1.f);
        }
        const auto intact_archive=[&](){
            return word(source_globals)==source_owner&&word(source_owner+12)==archive_context&&
                   word(source_owner+16)==archive_scene&&at(source_owner)[9]==1&&
                   word(pools+28+8)==source_owner&&heap_owns_block();
        };
        for(unsigned epoch=1;epoch<=2;++epoch){
            const auto attack=epoch==1?parent:second_parent;
            if(epoch==2){
                route.parent_lane^=1;route.credit_lane^=1;
                at(second_parent+0x88)[0]=u8(route.parent_lane);at(actor+0x88)[0]=u8(route.credit_lane);
                set_origin({440.f,-20.f,1200.f});
                if(request_effect(attack)!=effect||word(active_head+4*bucket)!=effect||
                   half(pools+18)||half(contact::free_count+4)||at(effect+0x18)[0]||
                   !(word(effect+0x8C)==attack&&word(effect+0x90)==actor))
                    fail("contact visual did not reuse the original battle cell for its new parent");
            }
            const auto first=source_first_update(source,route,attack);
            const auto storage=word(visual_object+12);
            const unsigned block_size=burst?96:pieces?288:0;
            if(storage&&((storage&31)||storage<=archive_context||storage>=arena_end))
                fail("source effect heap payload escaped its declared area");
            const auto storage_block=storage?storage-32:0;
            if(bool(storage)!=bool(block_size)||(storage&&(!heap_owns_block(storage_block)||word(storage_block+8)!=block_size)))
                fail("source effect did not claim its separate visual payload");

            // Keep the borrowed parent backing but make its live state and
            // position disagree with the effect's saved origin.
            at(attack)[0]=0;put_float(at(attack+0x20),-900.f);put_float(at(attack+0xB4),2.f);
            unsigned model_loads=first.count,peak=first.count,last_queued=1,refused=0;
            for(unsigned visit=2;visit<=retire;++visit){
                CallObservation model{0x8024F834};
                const auto loads=source_tick(model);
                const bool more=pieces&&visit<=5;
                const auto scene_records=word(archive_scene);
                if(loads!=unsigned(more)||(more&&(model.caller!=0x800072B0||
                   model.arguments[0]!=word(word(scene_records+54*4)))))
                    fail("source visual did not install only the later selector-8 record-54 pieces");
                model_loads+=loads;
                const bool retired=visit==retire,enabled=visit<retire-1,charged=burst&&visit<retire-2;
                const unsigned phase=visit<retire-2?1:visit==retire-2?2:3;
                const auto mask=pieces?piece_masks[visit-1]:burst?unsigned(charged):unsigned(enabled);
                if(at(effect+0x18)[0]!=phase||bool(at(effect)[0])==retired||at(effect+0x82)[0]!=unsigned(enabled)||
                   half(scene+484)!=spec.bursts+unsigned(charged)||half(scene+486)!=spec.entries+unsigned(charged)||
                   (burst&&word(effect+0x158)!=mask)||(pieces&&half(effect+0x1C)!=mask)||
                   !intact_archive())
                    fail("source visual changed its lifetime, scene charge, or borrowed archive owner");
                const auto queued=source_view(mask);
                if(queued){last_queued=visit;peak=std::max(peak,queued);}
                if(!retired){
                    if(word(effect+0x8C)!=attack||word(effect+0x90)!=actor||word(active_head+4*bucket)!=effect||
                       half(contact::free_count+4)||half(pools+18)||
                       (storage&&(!heap_owns_block(storage_block)||word(visual_object+12)!=storage||
                                  word(row+8)!=visual_object||half(row+18)!=0||!at(visual_object)[9]))||
                       (!storage&&(word(visual_object+12)||word(row+8)||half(row+18)!=1)))
                        fail("source visual retired or changed ownership early");
                    if(phase!=1){
                        if(request_effect(contender)||word(active_head+4*bucket)!=effect||
                           half(contact::free_count+4)||half(pools+18))
                            fail("source visual reused a cell before retirement");
                        ++refused;
                    }
                }
            }
            if(refused!=2||word(active_head+4*bucket)||half(contact::free_count+4)!=1||
               word(contact::free_head+8)!=effect||half(pools+18)!=descriptors||word(effect+0xDC)||
               word(row+8)||word(row+12)!=visual_object||half(row+18)!=1||at(visual_object)[9]||word(visual_object+12)||
               (storage&&(heap_owns_block(storage_block)||!heap_free_covers(storage_block,block_size)))||
               std::memcmp(archive_after_relocation.data(),at(source_bank),source.bytes.size()))
                fail("source visual failed to return detached resources while retaining its original archive");
            returned_descriptors(descriptors);
            std::printf("effect-source-lifetime: %u %u %u %u %u %u %u %u %u %u %u %u\n",
                        source.slot,route.selector,epoch,first.models[0].id,first.count==2?first.models[1].id:0u,
                        model_loads,peak,last_queued,retire,descriptors,block_size,refused);
        }
    }
};
}

int main(int argc,char** argv){
    const bool render=argc==4&&!std::strcmp(argv[3],"--render-prep");
    const bool source_mode=argc==4&&!std::strcmp(argv[3],"--source-art");
    const bool source_lifetime_mode=argc==4&&!std::strcmp(argv[3],"--source-lifetime");
    if(argc!=3&&!render&&!source_mode&&!source_lifetime_mode)
        fail("usage: contact-effect-storage-fixture qualified.dol unmodified-recomp.so [--render-prep|--source-art|--source-lifetime]");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    if(source_mode||source_lifetime_mode){
        const auto source=read_source();
        if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected source art payload");
        for(unsigned i=0;i<std::size(source_cases);++i){
            const auto& route=source_cases[i];
            const bool burst=route.selector==1;
            const Case c{route.selector,burst?31u:0u,burst?511u:0u,0,0,0};
            if(source_mode)Check(mod,original,c,false,&source,&route).source_route(source,route,i);
            else if(i==0||i==1||i==4)Check(mod,original,c,false,&source,&route).source_lifetime(source,route);
        }
        return 0;
    }
    if(render){
        for(unsigned i:{0u,2u})Check(mod,original,cases[i],true).render_prepare();
        return 0;
    }
    for(unsigned i=0;i<std::size(cases);++i){
        if(cases[i].selector==1)Check(mod,original,cases[i]).burst_frontier(i);
        Check(mod,original,cases[i]).lifetime(i);
    }
}
