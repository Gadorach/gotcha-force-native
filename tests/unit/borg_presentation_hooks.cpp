#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_movement_audio.hpp"
#include "engine/gameplay/attachment_transform.hpp"
#include "engine/gameplay/borg_trailing_chain.hpp"
#include "engine/assets/stage_view_fade.hpp"
#include "engine/graphics/scene_draw.hpp"
namespace {
using namespace gf;
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"%s\n",why);std::abort();}}
std::shared_ptr<const BorgGameplayProfile> parse(std::string_view text){
    BorgGameplayProfile p;std::string error;
    check(borg_read_gameplay_profile(text,&p,&error),error.c_str());
    return std::make_shared<const BorgGameplayProfile>(std::move(p));
}
struct Origin final:IBorgBattleOrigins {
    bool accept=true;
    mutable unsigned calls=0;
    bool socket(unsigned owner,const BattleMvpActor& actor,u32 tick,u8 move,u16 frame,const BorgBattleSocket& spec,
                BattleMvpVec* out)const override{
        ++calls;
        check(!owner&&tick==2&&!move&&frame==1&&spec.bone==7&&spec.local==BattleMvpVec{-50,0,20},
              "projectile uses its declared socket on the actual release tick");
        *out={actor.position[0]+40,65,actor.position[2]+10};return accept;
    }
};
}
int main(){
    constexpr BorgTrailingChainSpec chain{{1,2,3},3};
    HsdModelBones tail_rig;tail_rig.bones.resize(4);
    for(unsigned i=1;i<4;++i)tail_rig.bones[i].parent=i-1;
    check(borg_trailing_chain_binds(chain,tail_rig),"terminal presentation chain admits its exact links");
    tail_rig.bones.push_back({});tail_rig.bones.back().parent=2;
    check(!borg_trailing_chain_binds(chain,tail_rig),"branch refuses instead of disconnecting a child");
    const auto flat=[](const BattleMvpVec&,f32* height){*height=0;return true;};
    CharacterWorldPose rest,posed;rest.matrices.resize(4);
    rest.matrices[1].at(1,3)=15;rest.matrices[2].at(1,3)=5;rest.matrices[3].at(1,3)=-5;
    BorgTrailingChain ribbon;
    check(ribbon.step(chain,0,rest,flat,&posed)&&ribbon.contacts()&&ribbon.tip()->at(1)>0&&ribbon.tip()->at(2)<0,
          "a hanging link swings back at its own terrain contact");
    const auto length=[](const Mat4& a,const Mat4& b){
        return std::hypot(a.at(0,3)-b.at(0,3),a.at(1,3)-b.at(1,3),a.at(2,3)-b.at(2,3));
    };
    check(std::abs(length(posed.matrices[2],posed.matrices[3])-10)<1e-4f&&
          rest.matrices[3].at(1,3)==-5&&posed.matrices[0].m[0]==rest.matrices[0].m[0],
          "maintains link lengths and does not change combat pose");
    const auto initial_tip=*ribbon.tip();auto moved=rest;
    for(auto& matrix:moved.matrices)matrix.at(0,3)+=3;
    check(ribbon.step(chain,1,moved,flat,&posed)&&ribbon.tip()->at(0)<3,"tip lags behind anchor motion");
    const auto advanced_tip=*ribbon.tip();
    check(ribbon.step(chain,1,moved,flat,&posed)&&length(posed.matrices[1],posed.matrices[2])>9.999f&&
          length(posed.matrices[1],posed.matrices[2])<10.001f&&*ribbon.tip()==advanced_tip,
          "no numerical drift from a second same-tick view");
    check(ribbon.step(chain,0,rest,flat,&posed)&&*ribbon.tip()==initial_tip,"round rewind re-seeds the chain");
    StageViewFadeProfile stage_fade;
    constexpr std::string_view fade_text="GFSTAGE-FADE 1\nobstacle=3|20|0.25|0.5|0.125|-50|0|-4|50|60|4\n";
    check(stage_view_fade_load(fade_text,&stage_fade,nullptr),"authored stage-fade descriptor");
    check(!stage_view_fade_load("GFSTAGE-FADE 1\nobstacle=0|20|0.25|0.5|0.125|-50|0|-4|50|60|4\n",&stage_fade,nullptr)&&
          stage_view_fade_load(fade_text,&stage_fade,nullptr),"cannot select the floor as alternate");
    std::array<StageViewFadeState,2> views{};
    check(views[0].update(stage_fade,{0,5,5},1)==.5f&&views[1].update(stage_fade,{0,5,80},1)==1&&
          views[0].update(stage_fade,{0,5,5},1)==.5f,"near/far viewers independent; same tick does not age");
    check(views[0].update(stage_fade,{0,5,5},2)==.375f&&views[0].update(stage_fade,{0,5,5},3)==.25f&&
          views[1].alpha==1,"only near view fades to authored minimum");
    check(views[0].update(stage_fade,{0,5,80},4)==.375f&&views[0].update(stage_fade,{0,5,80},5)==.5f&&
          views[0].update(stage_fade,{0,5,80},6)==1&&views[0].update(stage_fade,{0,5,5},1)==.5f,"view restore and round rewind");
    constexpr std::string_view occlusion_text="GFSTAGE-FADE 2\ngate=occlusion\nobstacle=3|0|0.25|0.5|0.125|-50|0|-4|50|60|4\n";
    check(stage_view_fade_load(occlusion_text,&stage_fade,nullptr)&&stage_fade.gate==StageViewFadeGate::Occlusion,
          "explicit camera-occlusion mode");
    const std::array<f32,3> target{0,10,-30};
    check(stage_view_fade_occludes(stage_fade,{0,30,90},target),
          "obstacle can occlude the view even when both endpoints are far away");
    check(!stage_view_fade_occludes(stage_fade,{0,10,-10},target)&&
          !stage_view_fade_occludes(stage_fade,{0,10,-10},{0,10,-10})&&
          !stage_view_fade_occludes(stage_fade,{80,10,-40},{80,10,40}),
          "behind-target, degenerate and parallel outside rays do not fade");
    views={};
    check(views[0].update_gate(stage_fade,stage_view_fade_occludes(stage_fade,{0,30,90},target),1)==.5f&&
          views[1].update_gate(stage_fade,stage_view_fade_occludes(stage_fade,{0,10,-10},target),1)==1&&
          views[0].update_gate(stage_fade,false,1)==.5f,"one blocked camera does not fade a second view or advance on a redraw");
    constexpr std::string_view multi="GFSTAGE-FADE 3\n"
        "obstacle=occlusion|2|3|0|0.25|0.5|0.125|-50|0|-4|50|60|4\n"
        "obstacle=near|4|5|15|0.3|0.6|0.1|70|0|-4|80|60|4\n";
    std::vector<StageViewFadeProfile> fades;
    check(stage_view_fades_load(multi,&fades,nullptr)&&fades.size()==2&&fades[0].normal==2&&fades[1].normal==4&&
          fades[1].gate==StageViewFadeGate::Proximity,"mixed gates with separate record pairs");
    check(!stage_view_fades_load(std::string(multi)+"obstacle=near|5|6|15|0.3|0.6|0.1|0|0|0|10|10|10\n",&fades,nullptr)&&
          fades.empty(),"cross-pair alias refuses without retaining earlier rows");
    check(stage_view_fades_load(occlusion_text,&fades,nullptr)&&fades.size()==1&&fades[0].normal==2,
          "legacy viewport descriptor still uses the default record");
    using enum BorgBattleEventKind;
    constexpr std::string_view setup=R"(GFBORG-GAMEPLAY 1
variant=normal
health=180
movement=grounded-v1|7|13|14
rig-prefix=body.prefix
pose-profile=body.pose
idle=0:0
walk=0:1
air=0:7
dash=0:2
air-jump-points=3
stock=shots|4|150
projectile=star|shuriken|1|42|0|0|0|8|100|9|ignore-floor
origin=star|7|-50|0|20|target-height
look=star|64|122|218|255
projectile-animation=star|0.5|8|40
move=throw|b|20|any|8|3:0
pose-start=throw|7
release=throw|1|shots|1|star
part-spin=throw|5|z|8|reverse
part-motion=throw|6|0:2|0|6
part-scroll=throw|6|u|8|reverse
part-record=badge|6|1
part-pose=badge|2|0|0|0|0|0
part-turn=badge|546|1040|0
part-swap=throw|badge|2|5|2
part-swap-bone=throw|badge|4
part-swap-pose=throw|badge|2|0|0|90|90|0
part-material=2|0|1|4|loop
movement-effect=air|air-jump|puff|0|0|0|0|62|16
movement-sound=landing|audio/land.pcm
)";
    const auto p=parse(setup);
    const auto body=parse(std::string(setup)+"body-record=0\nbody-mount=2|1|-15|0|7\nanimated-translation-percent=50\n");
    check(body->animated_translation_percent==50&&body->body_mounts.size()==1&&
          body->body_mounts[0].offset==std::array<s16,3>{-15,0,7},"body recipe opts in to static assembly and a keyed translation scale");
    const auto reflected=parse(std::string(setup)+"body-record=0\nzero-reflection=15c98|7\n");
    check(reflected->zero_reflections[0].pobj==0x15c98&&reflected->zero_reflections[0].count==7,"zero-reflection rule identifies a bounded exact PObj");
    check(parse(std::string(setup)+"body-record=0\nzero-reflection=11488|20\nzero-reflection=129e8|44\n")->zero_reflections.size()==2,
          "two exact reflection meshes remain independently constrained");
    for(const auto* bad:{"zero-reflection=0|7","zero-reflection=15c99|7","zero-reflection=15c98|0","zero-reflection=15c98|65",
                         "zero-reflection=15c98|7\nzero-reflection=15c98|8"}){
        BorgGameplayProfile refused;
        check(!borg_read_gameplay_profile(std::string(setup)+"body-record=0\n"+bad+"\n",&refused,nullptr),
              "unbound or out-of-budget zero-reflection rule refuses");
    }
    for(const auto* bad:{"body-mount=1|7|0|4097|0","body-mount=2|3|0|0|0\nbody-mount=2|3|2|1|0",
                         "animated-translation-percent=100","animated-translation-percent=50\nanimated-translation-percent=80"}){
        BorgGameplayProfile refused;
        check(!borg_read_gameplay_profile(std::string(setup)+"body-record=0\n"+bad+"\n",&refused,nullptr),
              "invalid static assembly or translation scale refuses");
    }
    check(parse(std::string(setup)+"body-record=0\ntrailing-chain=27|28|29|30|31\n")->trailing_chain->count==5,
          "authored trailing pose can bind an exact branch");
    for(const auto* bad:{"27|28","27|27|28","27|0|28","27|28|32","27|28|29|","1|2|3|4|5|6|7|8|9"}){
        BorgGameplayProfile refused;
        check(!borg_read_gameplay_profile(std::string(setup)+"body-record=0\ntrailing-chain="+bad+"\n",&refused,nullptr),
              "malformed or out-of-budget trailing chain refuses");
    }
    check(p->moves[0].pose_start==7&&p->movement_fx.size()==1&&p->movement_sound[2]=="audio/land.pcm"&&
          p->moves[0].part_spins[0].reverse&&p->moves[0].part_motions[0].bone==6&&
          p->moves[0].part_scrolls[0].reverse&&p->moves[0].part_scrolls[0].bone==6,"loaded independent assets");
    BorgGameplayProfile invalid;
    for(const auto row:{"look=star|181|0|1|2","origin=star|1|0|0|0|fixed","movement-effect=invalid|rise|jet|0|0|0|0|50|1",
                        "movement-sound=takeoff|../outside.pcm","part-spin=throw|0|z|8|forward",
                        "part-spin=throw|5|z|12|forward","part-motion=throw|6|0:1|5|10","projectile-animation=star|0|1|0",
                        "part-scroll=throw|6|u|8|forward","part-scroll=throw|6|z|8|forward","part-scroll=throw|6|v|1|forward",
                        "part-scroll=missing|6|u|8|forward","part-record=badge|6|2",
                        "part-swap=throw|missing|0|5|2","part-swap=throw|badge|6|7|1",
                        "part-material=2|1|0|5|once","part-material=3|0|0|5|once","part-material=1|0|5|5|loop",
                        "part-pose=badge|0|0|0|0|0|0","part-pose=missing|0|0|0|0|0|0",
                        "part-swap-pose=missing|badge|0|0|0|0|0|0","part-pose=other|0|0|0|0|0|181",
                        "part-turn=badge|2|0|0","part-turn=missing|546|0|0","part-turn=other|8193|0|0","part-turn=other|0|0|0",
                         "part-swap-bone=throw|badge|0","part-swap-bone=throw|other|3","part-swap-bone=throw|badge|32",
                         "part-basis=missing|world-zyx","part-basis=badge|world-xyz","part-basis=badge|world-zyx\npart-basis=badge|world-zyx",
                         "part-mount-spin=missing|y|32|forward","part-mount-spin=badge|y|1|reverse",
                         "part-hierarchy=0","part-hierarchy=255","part-hierarchy=2"})
        check(!borg_read_gameplay_profile(std::string(setup)+row,&invalid,nullptr),"oversized or ambiguous binding refuses");
    check(p->part_slots[0].basis==BorgPartBasis::Bone&&
          parse(std::string(setup)+"part-basis=badge|world-zyx")->part_slots[0].basis==BorgPartBasis::WorldZyx&&
          parse(std::string(setup)+"part-basis=badge|bone-zyx")->part_slots[0].basis==BorgPartBasis::BoneZyx,
          "basis and order are per-slot opt-in; earlier mounts stay unchanged");
    const auto spinning=parse(std::string(setup)+"part-mount-spin=badge|y|32|reverse\n");
    check(spinning->part_slots[0].mount_spin&&spinning->part_slots[0].mount_spin->axis==1&&
          spinning->part_slots[0].mount_spin->period==32&&spinning->part_slots[0].mount_spin->reverse,
          "visible mount can spin independently of the actor");
    const auto& slot=p->part_slots[0];const auto& move=p->moves[0];
    check(borg_part_record(slot,0,nullptr,2)==1&&borg_part_record(slot,0,&move,1)==1&&
          borg_part_record(slot,0,&move,2)==2&&borg_part_record(slot,0,&move,5)==2&&
          borg_part_record(slot,0,&move,6)==1,"part record changes only during the accepted move window");
    const auto* before_pose=borg_part_pose(slot,borg_part_active_swap(0,&move,1));
    const auto* swap_pose=borg_part_pose(slot,borg_part_active_swap(0,&move,2));
    check(before_pose&&swap_pose&&before_pose->degrees[0]==0&&swap_pose->degrees[0]==90&&
          borg_part_pose(slot,borg_part_active_swap(0,&move,6))==before_pose,"mount placement follows only the swap window");
    check(borg_part_bone(slot,borg_part_active_swap(0,&move,1))==6&&
          borg_part_bone(slot,borg_part_active_swap(0,&move,2))==4&&
          borg_part_bone(slot,borg_part_active_swap(0,nullptr,2))==6,"equipment follows active mount and interruption restores default");
    const f32 base[]{10,20,30};
    const std::array parent{mat4_multiply(mat4_translation(base),mat4_rotation_z(1.57079632679f))};
    const auto unchanged=parent;
    AttachmentBoneTransform placed;
    check(attachment_mount_transform(parent,0,swap_pose->offset,swap_pose->degrees,&placed,nullptr)&&
          std::abs(placed.position[0]-10)<.001f&&std::abs(placed.position[1]-22)<.001f&&
          std::abs(placed.position[2]-30)<.001f&&std::abs(placed.world.at(2,1)-1)<.001f&&
          std::equal(parent[0].m,parent[0].m+16,unchanged[0].m),"local offset then X/Y rotation without altering the parent");
    check(!attachment_mount_transform(parent,0,swap_pose->offset,{0,181,0},&placed,nullptr)&&!placed.valid,
          "invalid mount is not published");
    AttachmentMountTurn turn;
    const auto initial=turn.sample({-90,90,35},slot.turn_step,9);
    check(turn.angles==std::array<s16,3>{-546,1040,0}&&initial[2]==35,"independent mount turn limits and snap axis");
    check(turn.sample({-90,90,35},slot.turn_step,9)==initial,"second publication of same tick cannot age turn");
    turn.sample({-75,-90,0},slot.turn_step,10);
    check(turn.angles==std::array<s16,3>{-1092,0,0},"new pose preserves the prior visible angle");
    turn.sample({0,0,0},slot.turn_step,11);
    turn.sample({0,0,0},slot.turn_step,12);
    check(turn.angles==std::array<s16,3>{0,0,0},"default pose is restored by turning, without overshoot");
    turn={};turn.sample({-90,90,0},slot.turn_step,14);
    turn.sample({90,-90,0},slot.turn_step,1);
    check(turn.angles==std::array<s16,3>{546,-1040,0},"rewound tick starts from neutral");
    turn.angles[0]=32000;
    turn.sample({-178,0,0},slot.turn_step,2);
    check(turn.angles[0]==32546,"turn crosses the signed boundary by the short arc");
    auto material=p->part_materials[0];
    check(borg_part_material_phase(material,0)==0&&borg_part_material_phase(material,3)==3&&
          borg_part_material_phase(material,4)==0&&borg_part_material_phase(material,13)==1,"part clock loops on the authored inclusive range");
    material.playback=BorgPartMaterialPlayback::Once;
    check(borg_part_material_phase(material,40)==3,"one-shot part material holds its last phase");
    material.last=material.first;material.playback=BorgPartMaterialPlayback::Sample;
    check(borg_part_material_phase(material,40)==0,"fixed material sample doesn't advance");
    BattleMvpConfig config;config.packages={p,p};config.spawn={BattlePoint{},BattlePoint{0,400}};
    NativeBattleMvp world;Origin origin;
    const auto step=[&](u8 attack=0,bool jump=false){
        GamePadState pad;pad.b=jump;
        check(world.step({pad,{}},false,nullptr,nullptr,nullptr,nullptr,{BorgBattleInputs{attack,0,0},{}},&origin),"world tick");
    };
    check(world.start(config),"bind");
    step(BorgBattleInputs::b);step();
    const auto& star=world.projectiles()[0];
    check(origin.calls==1&&star.previous==BattleMvpVec{40,65,10}&&star.weapon==BattleMvpWeapon::Shuriken&&
          star.package_look.size==64&&star.package_radius==8&&star.velocity[1]<0,"origin, image and collision are independent");
    const auto first=borg_projectile_visual_sample(star,2),peak=borg_projectile_visual_sample(star,6),repeat=borg_projectile_visual_sample(star,10);
    check(first.scale==1&&first.angle==0&&std::abs(peak.scale-1.4f)<.001f&&peak.angle==2&&repeat.scale==1&&repeat.angle==4&&
          star.package_radius==8,"pulse and spin use simulation age without writing collision size");
    origin.accept=false;check(world.start(config),"socket refusal world");step(BorgBattleInputs::b);step();
    check(!world.shots_spawned()&&std::any_of(world.package_events().begin(),world.package_events().end(),
        [](auto e){return e.kind==SocketRefused;}),"missing accepted socket never falls back");
    Camera camera;camera.eye[2]=10;camera.viewport_width=640;camera.viewport_height=480;
    std::vector<Vertex> triangles;const f32 center[]{0,0,0};
    check(ScreenDraw::shuriken(camera,center,2,0,{122,218,255,255},triangles)&&triangles.size()==24,
        "four-blade geometry rather than a bullet quad");
    const auto previous_geometry=triangles.size();
    check(ScreenDraw::energy_orb(camera,center,2,.25f,{122,218,255,255},triangles)&&triangles.size()==previous_geometry+108,
        "orb draws an independent core and rotating detail");
    BorgMovementSoundBank bank;std::string pcm{"\xd0\x07\xd0\x07",4};
    check(borg_movement_sound_decode(pcm,&bank[2])&&bank[2]==std::vector<s16>{2000,2000}&&
          !borg_movement_sound_decode("xx",&bank[1]),"bounded little-endian stereo decode");
    bank[1]=bank[2];bank[3]=bank[2];
    const std::array<const BorgMovementSoundBank*,2> banks{&bank,nullptr};
    BorgMovementAudio music;std::vector<s16> out;
    check(world.start(config),"movement cues fixture");
    step(0,true);
    check(world.actors()[0].package_movement_cues==9&&music.visit(world,banks,&out)&&out[10]==1000,"takeoff and rising loop");
    step();step(0,true);
    check(world.actors()[0].package_movement_cues==10&&music.visit(world,banks,&out)&&out[0]==2000&&out[10]==1000,
        "accepted airborne jump emits one pulse alongside the current loop");
    check(!music.visit(world,banks,&out),"same tick cannot repeat audio");
    step();step(0,true);step();step(0,true);
    check(world.actors()[0].package_air_points_used==3&&world.actors()[0].package_movement_cues==8,"cap prevents another air-jump cue");
    for(unsigned i=0;i<100&&!(world.actors()[0].package_movement_cues&4u);++i)step();
    check(world.actors()[0].package_movement_cues==4&&music.visit(world,banks,&out)&&out[0]==1000&&out[10]==0,
        "landing plays a one-shot, not the ascent loop");
    check(world.start(config),"replace round");step();
    check(music.visit(world,banks,&out)&&out.empty(),"no old movement voices after restart");
    const auto sound_setup=std::string(setup)+"launch-sound=star|audio/star.pcm\n";
    config.packages[0]=parse(sound_setup);
    for(auto row:{"launch-sound=star|audio/again.pcm\n","launch-sound=missing|audio/again.pcm\n",
                  "launch-sound=star|../unbound.pcm\n"}){
        BorgGameplayProfile bad;
        check(!borg_read_gameplay_profile(sound_setup+row,&bad,nullptr),"duplicate, unresolved and unsafe sound rows refuse");
    }
    BorgLaunchSoundBank launch;
    launch[0].assign(1800,2000); // beyond one block, to exercise round-scope discard
    const std::array<const BorgLaunchSoundBank*,2> launches{&launch,nullptr};
    origin.accept=true;
    check(world.start(config),"launch sound round");
    step(BorgBattleInputs::b);
    check(music.visit(world,banks,&out,launches)&&out.empty(),"starting the move is not a launch cue");
    step();
    check(music.visit(world,banks,&out,launches)&&music.launch_started()==1&&out[0]==700&&
          !music.visit(world,banks,&out,launches),"accepted shot queues one bounded sound visit");
    origin.accept=false;
    check(world.start(config),"launch refusal round");
    step(BorgBattleInputs::b);step();
    check(music.visit(world,banks,&out,launches)&&!music.launch_started()&&out.empty(),
          "refused socket queues no sound and an old launch tail cannot survive the restart");
    const auto paid_sound=sound_setup+"release-sound=throw|1|audio/paid.pcm\n";
    config.packages[0]=parse(paid_sound);
    for(auto row:{"release-sound=throw|1|audio/again.pcm\n","release-sound=throw|2|audio/else.pcm\n",
                  "release-sound=unknown|1|audio/else.pcm\n"}){
        BorgGameplayProfile bad;
        check(!borg_read_gameplay_profile(paid_sound+row,&bad,nullptr),"release sound must name one exact paid marker");
    }
    BorgReleaseSoundBank release_sounds;
    release_sounds[0]={0,1,{4000,4000}};
    const std::array<const BorgReleaseSoundBank*,2> release_banks{&release_sounds,nullptr};
    check(world.start(config),"paid sound with refused socket");
    step(BorgBattleInputs::b);step();
    check(music.visit(world,banks,&out,launches,release_banks)&&music.release_started()==1&&
          !music.launch_started()&&out[0]==1400&&!world.shots_spawned(),
          "paid actor cue survives socket refusal without a child cue");
    origin.accept=true;
    check(world.start(config),"paid and accepted launch sounds");
    step(BorgBattleInputs::b);step();
    check(music.visit(world,banks,&out,launches,release_banks)&&music.release_started()==1&&music.launch_started()==1&&
          out[0]==2100,"actor and child cues sum on the shared mixer");
    BorgBattleActionState unpaid;unpaid.reset(*config.packages[0]);unpaid.stocks[0].current=0;
    std::vector<BorgBattleEventKind> trace;
    auto fire=[&](u8 press){unpaid.visit(*config.packages[0],{press,0,0},true,false,
        [](const auto&,u8,u8){return u32(1);},[&](auto kind,u8,u16,u32,u16){trace.push_back(kind);});};
    fire(BorgBattleInputs::b);fire(0);
    check(std::find(trace.begin(),trace.end(),BorgBattleEventKind::StockRefused)!=trace.end()&&
          std::find(trace.begin(),trace.end(),BorgBattleEventKind::ReleaseSound)==trace.end(),
          "refused debit never starts the paid cue");
    BattleMvpConfig lab;lab.provisional_walk=lab.provisional_jump=true;
    check(world.start(lab),"solo jump audio setup");
    GamePadState jump;jump.b=true;
    check(world.step({jump,{}},false)&&!world.actors()[0].package_movement_cues&&
          music.visit(world,banks,&out)&&out.empty(),"solo takeoff does not reuse authored package rise audio");
    unsigned solo_landings=0,solo_blocks=0;
    for(unsigned j=0;j<70;++j){
        const bool airborne=!world.actors()[0].grounded;
        check(world.step({},false)&&music.visit(world,banks,&out),"solo fixed-tick cue consumption");
        const bool landed=airborne&&world.actors()[0].grounded;
        check(unsigned(world.actors()[0].package_movement_cues)==(landed?u8(BorgMovementCue::Landing):0),
              "solo landing cue only at accepted contact");
        solo_landings+=unsigned(landed);
        if(!out.empty()){
            ++solo_blocks;
            check(landed&&out[0]==1000&&!music.visit(world,banks,&out),"single mixed landing pulse; duplicate tick refuses");
        }
    }
    check(solo_landings==1&&solo_blocks==1,"one pulse for a complete native solo jump");
    check(world.start(lab)&&world.step({},false)&&music.visit(world,banks,&out)&&out.empty(),
          "solo reset discards audio state before any new landing");
    std::puts("Borg presentation: socket release, shuriken geometry, movement cues and PCM lifetime PASS");
}
