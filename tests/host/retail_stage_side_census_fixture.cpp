// Replay the original side-stage placement loop, not the enclosing scene loader.
#include "retail_contact_effect_archive.hpp"
#include <algorithm>

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

class Census {
    const Module& mod;
    std::vector<u8> ram;
    std::array<u32,8> gqr{};
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    u16 half(u32 a){return contact::half(ram,a);}
    void store(u32 a,u32 v){put32(at(a),v);}
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
public:
    Census(const Module& m,const std::vector<u8>& pristine):mod(m),ram(pristine){}
    void run(unsigned member,const std::vector<u8>& side,const std::array<u8,21504>& collision){
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
    }
};
} // namespace

int main(int argc,char** argv){
    if(argc!=3)fail("usage: side-census qualified.dol unmodified-recomp.so < qualified-inputs");
    std::array<u8,21504> collision{};
    if(std::fread(collision.data(),1,collision.size(),stdin)!=collision.size())fail("side census common collision input extent");
    const auto original=load_ram(argv[1]);const Module mod(argv[2],original);
    for(unsigned n=2419;n<=2479;++n){
        std::array<u8,8> header{};
        if(std::fread(header.data(),1,header.size(),stdin)!=header.size()||be32(header.data())!=n)
            fail("side census member order");
        const auto size=be32(header.data()+4);
        if(size<6144||size>(1u<<18))fail("side census archive input size");
        std::vector<u8> side(size);
        if(std::fread(side.data(),1,size,stdin)!=size)fail("side census archive input extent");
        Census(mod,original).run(n,side,collision);
    }
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("side census unexpected input suffix");
}
