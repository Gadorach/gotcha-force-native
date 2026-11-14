#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "engine/gameplay/character_pose.hpp"
namespace {
unsigned checks=0;
void check(bool value,const char* why){++checks;if(!value){std::fprintf(stderr,"character pose: %s\n",why);std::abort();}}
bool near(float a,float b,float tolerance=2e-4f){return std::fabs(a-b)<tolerance;}
bool same(const gf::Mat4& a,const gf::Mat4& b){for(unsigned i=0;i<16;++i)if(!near(a.m[i],b.m[i]))return false;return true;}
void word(std::vector<gf::u8>& b,std::size_t at,gf::u32 v){gf::store_be32(b.data()+at,v);}
gf::HsdModelBones bones(){
    gf::HsdModelBones b;b.loaded=true;b.root_offset=0x100;b.preorder_slots={0,1,2,3};b.bones.resize(4);
    for(unsigned i=0;i<4;++i)b.bones[i].joint_offset=0x100+i*0x100;
    b.bones[0].child=1;b.bones[1].parent=0;b.bones[1].child=2;b.bones[2].parent=1;b.bones[2].next=3;b.bones[3].parent=1;
    b.bones[0].translation[0]=100;b.bones[1].translation[1]=2;b.bones[2].translation[0]=1;b.bones[3].translation[0]=-1;return b;
}
std::vector<gf::u8> profile(){
    std::vector<gf::u8> p(56,0);word(p,0,0x47464350);gf::store_be16(p.data()+4,1);gf::store_be16(p.data()+6,4);p[8]=3;p[9]=2;p[10]=4;p[11]=4;
    word(p,12,32);word(p,16,36);word(p,20,40);word(p,24,p.size());word(p,28,0x100);
    const gf::u8 order[]={2,3,0,1},mirror[]={0,1,3,2};
    for(unsigned i=0;i<4;++i){p[32+i]=order[i];p[36+i]=mirror[i];word(p,40+i*4,0x100+i*0x100);}return p;
}
gf::HsdBoneMotionSample sample(){gf::HsdBoneMotionSample s;s.bones.resize(4);for(unsigned i=0;i<4;++i)s.bones[i].joint_offset=0x100+i*0x100;return s;}
std::vector<gf::u8> read(const std::string& path){std::ifstream f(path,std::ios::binary);return {(std::istreambuf_iterator<char>(f)),{}};}
}
int main(){
    std::string error;auto p=profile();auto binding=bones();gf::CharacterPoseRig rig;
    auto load=[&](const auto& bytes,const auto& b){return gf::character_pose_rig_load({bytes.data(),bytes.size()},b,&rig,&error);};
    check(load(p,binding),"owned logical rig/lane/mirror profile loads");
    auto a=sample(),b=sample();std::array<gf::CharacterPoseLane,4> lanes;lanes[0].current=&a;lanes[1].current=&b;
    gf::CharacterLocalPose pose;gf::CharacterWorldPose world;gf::CharacterPoseFrame frame;gf::Mat4 anchor;anchor.at(0,3)=10;
    check(gf::character_pose_sample(rig,lanes,&pose,&error)&&pose.bones[0].translation[0]==100&&pose.bones[2].translation[0]==1&&pose.bones[2].quaternion[3]==1,"missing channels reset to wrapper rest and identity quaternion, not authored file rotation");
    check(gf::character_pose_initial_frame(rig,anchor,&frame,&error),"source initial zero extra angles/unit bone scales with explicit actor anchor");
    check(gf::character_pose_direct_world(rig,pose,frame,&world,&error)&&world.matrices[0].at(0,3)==10,"bone0 is actor anchor, clip root translation is NOT implicitly integrated");
    b.bones[1].present=1u<<3;b.bones[1].value[3]=1.5707963267948966f;
    check(gf::character_pose_sample(rig,lanes,&pose,&error)&&gf::character_pose_direct_world(rig,pose,frame,&world,&error)&&near(world.matrices[1].at(0,3),10)&&near(world.matrices[1].at(1,3),2)&&near(world.matrices[2].at(0,3),10)&&near(world.matrices[2].at(1,3),3),"source parent*T*Q propagation rotates child translations");
    check(world.joint_offsets==std::vector<gf::u32>{0x100,0x200,0x300,0x400},"world output retains logical joint identities");
    frame.bone1_translation_adjust[0]=3;
    check(gf::character_pose_direct_world(rig,pose,frame,&world,&error)&&near(world.matrices[1].at(0,3),13),"bone1 uses local translation plus source root adjustment");
    frame.bone1_translation_adjust[0]=0;
    a.bones[2].present=(1u<<5)|(1u<<2)|(1u<<3);a.bones[2].value[5]=4;a.bones[2].value[2]=.4f;a.bones[2].value[3]=.2f;
    a.bones[3].present=(1u<<5)|(1u<<2);a.bones[3].value[5]=-7;a.bones[3].value[2]=.1f;lanes[0].mirrored=true;
    check(gf::character_pose_sample(rig,lanes,&pose,&error)&&pose.bones[3].translation[0]==-4&&pose.bones[2].translation[0]==7,"mirror remaps destination slots and negates translationX");
    float expected[4],euler[3]={0,-.4f,-.2f};check(gf::character_pose_detail::euler(euler,expected)&&near(pose.bones[3].quaternion[1],expected[1])&&near(pose.bones[3].quaternion[2],expected[2]),"EulerY/Z mirror precedes quaternion conversion");
    auto previous=sample();previous.bones[2].present=1u<<5;previous.bones[2].value[5]=8;lanes[0].mirrored=false;lanes[0].previous=&previous;lanes[0].previous_weight=.25f;
    check(gf::character_pose_sample(rig,lanes,&pose,&error)&&pose.bones[2].translation[0]==5,"previous weight lerps translation (current*.75 +old*.25)");
    const float identity[4]={0,0,0,1},opposite[4]={0,0,0,-1};float q[4];
    check(gf::character_pose_detail::blend(identity,opposite,.5f,q)&&near(q[3],1),"antipodal quaternions choose shortest representation, not a zero vector");
    euler[0]=euler[1]=0;euler[2]=1.5707963267948966f;gf::character_pose_detail::euler(euler,expected);
    check(gf::character_pose_detail::blend(identity,expected,.5f,q)&&near(q[2],.382683432f)&&near(q[3],.923879532f),"non-close quaternion interpolation uses spherical weights");
    euler[2]=.008f;gf::character_pose_detail::euler(euler,expected);gf::character_pose_detail::blend(identity,expected,.5f,q);
    float norm=0;for(auto v:q)norm+=v*v;
    check(norm<.9999999f&&norm>.9999f&&near(q[3],(1+expected[3])*.5f,1e-7f),"source close-dot branch stays linear, not gratuitously normalized");
    euler[0]=.2f;euler[1]=.3f;euler[2]=.4f;gf::character_pose_detail::euler(euler,q);float matrix[12];gf_transform_quaternion(q,matrix);gf::Mat4 quat;for(unsigned i=0;i<12;++i)quat.m[i]=matrix[i];
    const float one[3]={1,1,1},zero[3]={};check(same(quat,gf::mat4_from_srt(euler,one,zero)),"Euler-to-quaternion basis agrees with independently composed Rz*Ry*Rx");
    lanes[0].previous_weight=0;lanes[0].previous=nullptr;lanes[0].mirrored=false;
    check(gf::character_pose_sample(rig,lanes,&pose,&error),"restore unblended local pose");
    frame.extra_angles[2]={4096,8192,16384};frame.scales[2]={2,3,4};
    check(gf::character_pose_direct_world(rig,pose,frame,&world,&error),"supplied source secondary rotation and post scale");
    gf::Mat4 local;gf_transform_quaternion(pose.bones[2].quaternion,matrix);for(unsigned i=0;i<12;++i)local.m[i]=matrix[i];for(unsigned k=0;k<3;++k)local.at(k,3)=pose.bones[2].translation[k];
    const float unit=gf::bits_to_f32(0x38c90fdbu),sizes[3]={2,3,4};
    auto extra=gf::mat4_multiply(gf::mat4_rotation_z(16384*unit),gf::mat4_multiply(gf::mat4_rotation_y(8192*unit),gf::mat4_rotation_x(4096*unit)));
    auto wanted=gf::mat4_multiply(world.matrices[1],local);wanted=gf::mat4_multiply(wanted,extra);wanted=gf::mat4_multiply(wanted,gf::mat4_scale(sizes));
    check(same(world.matrices[2],wanted),"secondary rotation/scale occur AFTER parent*local translation/quaternion");
    frame.bone1_look_angles={4096,8192,12288};
    check(gf::character_pose_direct_world(rig,pose,frame,&world,&error),"supplied source bone1 look angles");
    gf::Mat4 first;gf_transform_quaternion(pose.bones[1].quaternion,matrix);for(unsigned i=0;i<12;++i)first.m[i]=matrix[i];for(unsigned k=0;k<3;++k)first.at(k,3)=pose.bones[1].translation[k];
    auto look=gf::mat4_multiply(gf::mat4_rotation_z(12288*unit),gf::mat4_multiply(gf::mat4_rotation_x(4096*unit),gf::mat4_rotation_y(8192*unit)));
    wanted=gf::mat4_multiply(gf::mat4_multiply(anchor,look),first);
    check(same(world.matrices[1],wanted),"bone1 look uses Z/X/Y before bone1 local transform");
    auto stable=rig;p.clear();binding={};check(gf::character_pose_sample(rig,lanes,&pose,&error),"rig owns profile and bone bindings");p=profile();binding=bones();
    auto bad=[&](auto bytes,const auto& bs,const char* why){check(!load(bytes,bs)&&!rig.loaded&&rig.binding.bones.empty(),why);};
    auto x=p;word(x,24,0);bad(x,binding,"declared profile size mismatch refuses");
    x=p;word(x,12,0xfffffffcu);bad(x,binding,"table offset overflow refuses");
    x=p;word(x,16,32);bad(x,binding,"overlapping profile tables refuse");
    x=p;word(x,28,0x200);bad(x,binding,"wrong published model root refuses");
    x=p;word(x,44,0x900);bad(x,binding,"logical joint identity mismatch refuses");
    x=p;x[8]=0x80;bad(x,binding,"unsupported lane mask refuses");
    x=p;gf::store_be16(x.data()+4,2);bad(x,binding,"unknown profile version refuses");
    x=p;word(x,20,41);bad(x,binding,"unaligned identity table refuses");
    x=p;x[9]=5;bad(x,binding,"lane boundary outside bone count refuses");
    x=p;x[33]=2;bad(x,binding,"duplicate active updates refuse");
    x=p;x[38]=0;bad(x,binding,"cross-lane mirror target refuses until separately qualified");
    auto wrong=binding;wrong.bones[2].child=1;bad(p,wrong,"bone graph cycle refuses");
    wrong=binding;wrong.bones[2].parent=0;bad(p,wrong,"inconsistent parent topology refuses");rig=stable;
    auto bad_lanes=lanes;bad_lanes[1].current=nullptr;
    check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error)&&pose.bones.empty(),"missing active lane refuses atomically");
    bad_lanes=lanes;bad_lanes[0].previous_weight=.5f;
    check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"missing previous sample cannot masquerade as a blend");
    bad_lanes=lanes;bad_lanes[0].previous_weight=NAN;check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"nonfinite old weight refuses");
    bad_lanes[0].previous_weight=-.1f;check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"negative blend weight refuses");
    bad_lanes[0].previous_weight=1.1f;check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"extrapolating blend weight refuses");
    auto corrupt=a;corrupt.bones[2].present|=1u<<8;bad_lanes=lanes;bad_lanes[0].current=&corrupt;
    check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"scale/other callbacks outside source pose subset refuse");
    corrupt=a;corrupt.bones[2].value[2]=NAN;check(!gf::character_pose_sample(rig,bad_lanes,&pose,&error),"nonfinite motion values refuse");
    check(gf::character_pose_sample(rig,lanes,&pose,&error)&&gf::character_pose_initial_frame(rig,anchor,&frame,&error),"restore valid pose and frame");
    frame.inverse_kinematics=true;check(!gf::character_pose_direct_world(rig,pose,frame,&world,&error)&&world.matrices.empty(),"IK branch not silently replaced with direct hierarchy");frame.inverse_kinematics=false;
    frame.alternate_root=true;check(!gf::character_pose_direct_world(rig,pose,frame,&world,&error),"alternate root constructor remains explicit refusal");frame.alternate_root=false;
    frame.enabled_bones&=~4u;check(!gf::character_pose_direct_world(rig,pose,frame,&world,&error),"disabled-bone retained matrices require state, not static fallback");frame.enabled_bones=0xffffffff;
    frame.scales[2][0]=0;check(!gf::character_pose_direct_world(rig,pose,frame,&world,&error),"singular bone scale refuses");frame.scales[2][0]=1;
    pose.bones[2].quaternion[0]=pose.bones[2].quaternion[1]=pose.bones[2].quaternion[2]=pose.bones[2].quaternion[3]=0;
    check(!gf::character_pose_direct_world(rig,pose,frame,&world,&error),"zero quaternion refuses without identity fallback");
    bool truncated=true;for(std::size_t n=0;n<p.size();++n)truncated=truncated&&!gf::character_pose_rig_load({p.data(),n},binding,&rig,&error);
    check(truncated,"every truncated profile view refuses");
    unsigned seed=0xc031e7u;
    for(unsigned i=0;i<512;++i){x=p;seed=seed*1664525u+1013904223u;const auto at=seed%x.size();seed=seed*1664525u+1013904223u;x[at]^=gf::u8((seed>>24)|1);
        const bool ok=load(x,binding);if(!ok&&(rig.loaded||!rig.binding.bones.empty()))std::abort();}
    check(true,"512 deterministic profile mutations are bounded and atomic on refusal");
    unsigned real=0,frames=0;
    for(const char* name:{"g-red","sasuke"}){
        const std::string base=std::string("local/content/startup-characters/")+name+"/";
        auto prefix=read(base+"model-prefix.bin"),model=read(base+"model.hsd"),layout=read(std::string("local/content/character-pose-bindings/")+name+".gfcp");if(prefix.empty()||model.empty()||layout.empty())continue;
        gf::HsdModelBones bound;check(gf::hsd_bones_load({prefix.data(),prefix.size()},{model.data(),model.size()},&bound,&error)&&load(layout,bound),"original bone identities plus source lane/mirror profile");
        for(unsigned choice:{0,1,3,4,6,7}){
            auto data=read(std::string("local/content/startup-motion-clips/")+name+"/startup-choice-0"+std::to_string(choice)+".hsd");gf::HsdBoneMotionClip clip;
            check(gf::hsd_bone_motion_load({data.data(),data.size()},bound,&clip,&error),"original source-selected loose clip loads");
            for(unsigned t=0;t<=unsigned(clip.end_frame);++t){
                gf::HsdBoneMotionSample values,old;check(gf::hsd_bone_motion_sample(clip,t,&values,&error)&&gf::hsd_bone_motion_sample(clip,t?t-1:0,&old,&error),"actual original requested channel frames");
                std::array<gf::CharacterPoseLane,4> requested;for(unsigned i=0;i<2;++i){requested[i].current=&values;requested[i].previous=&old;requested[i].previous_weight=t?.25f:0;requested[i].mirrored=(t&1)!=0;}
                check(gf::character_pose_sample(rig,requested,&pose,&error)&&gf::character_pose_initial_frame(rig,anchor,&frame,&error)&&gf::character_pose_direct_world(rig,pose,frame,&world,&error),"actual original channels become bounded source-direct matrices, not file SRT");
                check(world.matrices.size()==bound.bones.size()&&world.joint_offsets[1]==bound.bones[1].joint_offset&&world.matrices[0].at(0,3)==10,"actual matrix identities and supplied actor anchor retained");++frames;
            }
        }
        ++real;
    }
    std::printf("{\"character_pose_checks\":%u,\"real_models\":%u,\"source_channel_frames\":%u,\"direct_pose_subset_only\":true,\"startup_or_actor_controller_admission\":false,\"all_passed\":true}\n",checks,real,frames);
}
