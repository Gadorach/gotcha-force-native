// A source-loaded Machine Red joint, its archived track, and original SDK sampling.
#include "retail_contact_effect_archive.hpp"
#include "retail_contact_clock_consumers.hpp"
#include <cmath>

namespace {
using namespace retail_contact_pose_clock;
namespace sdk=retail_contact_effect_archive;
constexpr u32 model_base=0x81400000,clip_base=0x81440000,reloc=0x8025B00C,joint_loader=0x8024F834;
constexpr u32 bind_track=0x8024DC04,request_frame=0x8024D838,set_rate=0x80243FC0,sample=0x8024EA64;
constexpr u32 publish=0x8004B76C,output_lane=0x81067000,translation=output_lane+0x100,quaternion=output_lane+0x120;
constexpr u32 isolated_descriptor=0x81066800;

template<size_t N>
std::array<u8,N> read_blob(){
    std::array<u8,N> bytes{};
    if(std::fread(bytes.data(),1,N,stdin)!=N)fail("joint clock archive input extent");
    return bytes;
}

struct Archive {u32 base,context,data;};
class Rig {
    const Module& mod;
    std::vector<u8> ram;
    sdk::Setup setup{mod,ram};
    u8* at(u32 a){return contact::at(ram,a);}
    u32 word(u32 a){return be32(at(a));}
    float scalar(u32 a){return std::bit_cast<float>(word(a));}

    void call(CPUState& state,u32 entry,CallObservation* observation=nullptr){
        if(mod.run_to_or_return(state,ram,entry,{0x80264290},observation,4096)){
            std::fprintf(stderr,"joint clock entry=%08X assertion=%08X line=%u\n",entry,state.lr-4,state.gpr[4]);
            fail("joint clock crossed an unsupported SDK boundary");
        }
    }
    u32 allocate_context(){
        CPUState request{};request.gpr[3]=68;
        call(request,0x8025AFA4);
        auto out=request.gpr[3];
        if(out<sdk::payload||out>sdk::arena_end-68||out%32)fail("joint clock archive context allocation");
        return out;
    }
    template<size_t N>
    Archive load(const std::array<u8,N>& blob,u32 target,unsigned size,unsigned relocs,unsigned publics){
        if(be32(blob.data())!=N||be32(blob.data()+4)!=size||be32(blob.data()+8)!=relocs||
           be32(blob.data()+12)!=publics||be32(blob.data()+16)||32+size+4*relocs+8*publics>N)
            fail("joint clock archive header changed");
        const auto* data=blob.data()+32;
        const auto* offsets=data+size;
        std::vector<bool> seen(size/4);
        for(unsigned i=0;i<relocs;++i){
            const auto offset=be32(offsets+4*i);
            if(offset%4||offset>size-4||seen[offset/4]||be32(data+offset)>=size)
                fail("joint clock archive relocation extent");
            seen[offset/4]=true;
        }
        std::memcpy(at(target),blob.data(),N);
        const auto context=allocate_context();
        CPUState boot{};boot.gpr[3]=context;boot.gpr[4]=target;boot.gpr[5]=N;
        call(boot,reloc);
        if(boot.gpr[3]||word(context+32)!=target+32||word(context+64)!=target)
            fail("joint clock failed to relocate its real archive");
        for(unsigned i=0;i<relocs;++i){
            const auto offset=be32(offsets+4*i);
            if(word(target+32+offset)!=target+32+be32(data+offset))
                fail("joint clock archive relocation did not round trip");
        }
        return {target,context,target+32};
    }
    u32 public_record(const Archive& archive,unsigned index,const char* name){
        CPUState lookup{};lookup.gpr[3]=archive.context;lookup.gpr[4]=index;
        call(lookup,0x8025B178);
        const auto length=std::strlen(name)+1;
        const auto extent=word(archive.base);
        if(lookup.gpr[3]<archive.data||lookup.gpr[3]>archive.base+extent-length||
           std::memcmp(at(lookup.gpr[3]),name,length))
            fail("joint clock public index changed");
        CPUState resolve{};resolve.gpr[3]=archive.context;resolve.gpr[4]=lookup.gpr[3];
        call(resolve,0x8025B1B0);
        return resolve.gpr[3];
    }
    u32 joint_for(u32 descriptor){
        CPUState lookup{};lookup.gpr[3]=0;lookup.gpr[4]=descriptor;
        call(lookup,0x8025983C);
        const auto joint=lookup.gpr[3];
        if(joint<sdk::payload||joint>sdk::arena_end-136)fail("original joint loader did not register the exact Red joint");
        return joint;
    }
public:
    Rig(const Module& module,const std::vector<u8>& original):mod(module),ram(original){}

    void observe(const std::array<u8,180391>& body,const std::array<u8,7656>& tracks){
        setup.init_heap();setup.call(0x80254288);
        const auto model=load(body,model_base,177788,638,1);
        const auto clip=load(tracks,clip_base,5612,271,32);
        const auto scene=public_record(model,0,"scene_data");
        const auto anim=public_record(clip,2,"pl0600_b02_animjoint");
        if(scene!=model.data+0x2B66C||anim!=clip.data+0x136C||
           word(anim+8)!=clip.data+0x1A8||word(anim+12)||word(anim+16))
            fail("joint clock selected a different model scene or animation joint");
        const auto descriptor=word(word(word(scene))),part=model.data+0x13A88;
        if(descriptor!=model.data+0x13A08||word(word(descriptor+8)+8)!=part||
           word(part+16)||word(part+56)||word(part+60))
            fail("joint clock could not isolate the exact geometry-free key-2 descriptor");
        // Isolate the local key-2 record. Keep the original model archive
        // intact and do not claim this local instance owns its retail tree.
        std::memcpy(at(isolated_descriptor),at(part),64);
        put32(at(isolated_descriptor+8),0);put32(at(isolated_descriptor+12),0);
        CPUState construct{};construct.gpr[3]=isolated_descriptor;
        call(construct,joint_loader);
        const auto joint=joint_for(isolated_descriptor);
        if(joint!=construct.gpr[3]||word(joint+8)||word(joint+12)||word(joint+16)||
           word(joint+20)!=0x10000040||word(joint+0x7C)||word(joint+0x80)){
            std::fprintf(stderr,"joint %08X/%08X links=%08X/%08X/%08X flags=%08X anim=%08X/%08X\n",
                         joint,construct.gpr[3],word(joint+8),word(joint+12),word(joint+16),
                         word(joint+20),word(joint+0x7C),word(joint+0x80));
            fail("isolated key-2 joint flags or initial ownership changed");
        }
        CPUState attach{};attach.gpr[3]=joint;attach.gpr[4]=anim;
        call(attach,bind_track);
        const auto aobj=word(joint+0x7C);
        if(aobj<sdk::payload||aobj>sdk::arena_end-28||word(joint+0x80)||word(aobj+24)||
           word(aobj+12)!=word(clip.data+0x1AC))
            fail("original track did not attach its source AObj");
        std::array<u32,3> fobjs{};
        constexpr unsigned source_tracks[]{0x16C,0x180,0x194};
        auto link=word(aobj+20);
        for(unsigned i=0;i<fobjs.size();++i){
            const auto track=clip.data+source_tracks[i];
            if(link<sdk::payload||link>sdk::arena_end-64)fail("original track did not attach three FObjs");
            if(at(link+19)[0]!=i+1||at(link+19)[0]!=at(track+12)[0]||
               word(link+8)!=word(track+16)||word(link+12)!=word(track+4))
                fail("live FObj channel or archived stream identity changed");
            fobjs[i]=link;link=word(link);
        }
        if(link)fail("unexpected continuation after the three authored FObjs");

        const auto attached=ram;
        for(unsigned scenario=0;scenario<4;++scenario){
            std::memcpy(ram.data(),attached.data(),ram.size());
            const bool slowed=scenario&1,explicit_frame=scenario&2;
            const unsigned seed=slowed?4:0;
            at(actor+timer)[0]=u8(seed);
            factors(ram,actor);put_float(at(actor+retail_contact_clock_consumers::lane_clock),6.f);
            float time=0.f;
            for(unsigned visit=1;visit<=4;++visit){
                tick(mod,ram,actor);
                const unsigned left=visit<seed?seed-visit:0;
                check_clocks(ram,actor,left,unit,left?slow:unit,left?slow:neutral_secondary);
                const auto rate=scalar(actor+secondary);
                if(visit==1){
                    time=retail_contact_clock_consumers::clip_frame(mod,ram,actor,explicit_frame?2.5f:-1.f);
                    if(time!=(explicit_frame?2.5f:6.f+rate))fail("joint clock clip request changed");
                    CPUState request{};request.gpr[3]=joint;request.gpr[4]=1;request.fpr[1]=time;
                    call(request,request_frame);
                    if(scalar(aobj+4)!=time)fail("original sample request did not adopt the exact frame");
                }
                CPUState assign{};assign.gpr[3]=aobj;assign.fpr[1]=rate;
                call(assign,set_rate);
                if(scalar(aobj+4)!=time||scalar(aobj+16)!=rate)fail("rate assignment advanced the track prematurely");
                CPUState visit_joint{};visit_joint.gpr[3]=joint;
                CallObservation decode{0x80243BD0};call(visit_joint,sample,&decode);
                if(visit!=1)time+=rate;
                if(scalar(aobj+4)!=time||word(joint+0x7C)!=aobj||word(joint+0x80)||word(aobj+20)!=fobjs.front()||
                   decode.count!=1||decode.caller!=0x8024EB88||decode.arguments[0]!=aobj||
                   decode.arguments[1]!=joint||decode.arguments[2]!=word(word(joint)+80)){
                    std::fprintf(stderr,"joint case=%u visit=%u time=%g/%g hooks=%u at=%08X axis=%g/%g/%g\n",scenario,
                                 visit,double(scalar(aobj+4)),double(time),decode.count,decode.caller,
                                 double(scalar(joint+28)),double(scalar(joint+32)),double(scalar(joint+36)));
                    fail("original pose sample did not follow the retained joint and its three tracks");
                }
                for(unsigned i=0;i<fobjs.size();++i){
                    const auto stream=word(fobjs[i]+8),cursor=word(fobjs[i]+4);
                    if(word(fobjs[i])!=(i+1<fobjs.size()?fobjs[i+1]:0)||cursor<stream||cursor-stream>word(fobjs[i]+12))
                        fail("original sample changed its attached FObj sequence");
                }
                CPUState copy{};copy.gpr[3]=actor;copy.gpr[4]=output_lane;copy.gpr[5]=joint;
                copy.gpr[6]=translation;copy.gpr[7]=quaternion;
                call(copy,publish);
                if(scalar(output_lane+12)!=time||std::memcmp(at(translation),at(joint+56),12)||at(actor+timer)[0]!=left)
                    fail("original local pose copy did not retain the sample frame or translation");
                float norm=0;
                for(unsigned i=0;i<4;++i){
                    const auto q=scalar(quaternion+4*i);
                    norm+=q*q;
                }
                if(!(norm>0.9999f&&norm<1.0001f))fail("original pose copy produced an invalid quaternion");
                std::printf("joint-clock: %u %u %u %08X %08X %08X %08X %08X %08X %08X %08X %08X\n",
                            scenario,visit,left,word(aobj+16),word(aobj+4),word(joint+28),word(joint+32),word(joint+36),
                            word(quaternion),word(quaternion+4),word(quaternion+8),word(quaternion+12));
            }
        }
    }
};
}

int main(int argc,char** argv){
    if(argc!=3)fail("usage: joint-clock-sample-fixture qualified.dol unmodified-recomp.so");
    const auto original=load_ram(argv[1]);
    const Module mod(argv[2],original);
    const auto body=read_blob<180391>();
    const auto clip=read_blob<7656>();
    if(std::getc(stdin)!=EOF||std::ferror(stdin))fail("unexpected joint clock payload");
    Rig(mod,original).observe(body,clip);
}
