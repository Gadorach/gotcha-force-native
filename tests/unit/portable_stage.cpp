#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "platform/pc/borg_gameplay_load.hpp"
#include "platform/pc/stage_package_scan.hpp"

namespace {
using namespace gf;
namespace fs=std::filesystem;
void check(bool okay,std::string_view error){
    if(!okay){std::fprintf(stderr,"portable stage: %.*s\n",int(error.size()),error.data());std::exit(1);}
}
[[maybe_unused]] void check(bool okay,const std::string* error){check(okay,*error);}
[[maybe_unused]] void write(const fs::path& path,std::string_view text){
    std::ofstream stream(path,std::ios::binary);stream<<text;check(bool(stream),"write disposable test file");
}
struct Backend final:IRenderBackend {
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    RenderCapabilities capabilities()const override{return {4096,false,false,"portable stage"};}
    bool upload_vertices(const Vertex*,std::size_t)override{return true;}
    bool upload_texture(u32,const TextureDesc&)override{return true;}
    bool upload_matrix(u32,const f32*)override{return true;}
    void begin_frame(const FrameDesc&)override{submissions=0;}
    void submit(const DrawCall&)override{++submissions;}
    void end_frame()override{}
    void present()override{}
    unsigned submissions=0;
};
constexpr std::string_view manifest=
    "GFSTAGE 1\r\n# Atelier \xc3\xa9\r\nid=fixture:room\r\nname=Atelier \xc3\xa9\r\n"
    "model=art/level.hsd\r\nfloor=grids/floor.stih\r\narena-half=1234.5\r\n"
    "spawn=left|-40.5|9.125|20\r\nspawn=right|40.5|0|-20";
void parse_checks(){
    StagePackageManifest parsed;std::string error;
    check(stage_package_read_manifest(manifest,&parsed,&error),"CRLF, UTF-8, signed decimal world coordinates");
    check(parsed.id=="fixture:room"&&parsed.arena_half==1234.5f&&parsed.spawn("left")->query[1]==9.125f,"typed fields");
    const auto bad=[&](std::string text){
        StagePackageManifest previous=parsed;
        check(!stage_package_read_manifest(text,&parsed,&error)&&parsed==previous&&!error.empty(),"reject without replacing output");
    };
    bad("GFSTAGE 2\n");
    for(auto tail:{"\nid=fixture:other","\nname=Duplicate","\nmodel=b.hsd","\narena-half=2000","\nsecondary=sec.stih",
                   "\nspawn=left|0|0|0","\nspawn=third|0|0|inf","\nspawn=third|1|0|0|0","\nspawn=third|1e100|0|0",
                   "\nspawn=third|0|20001|0","\nspawn=bad.id|0|0|0","\noption=unknown","\n# bad UTF-8 \xc0\xaf"})
        bad(std::string(manifest)+tail);
    auto replaced=std::string(manifest);
    replaced.replace(replaced.find("art/level.hsd"),13,"art/../level.hsd");bad(replaced);
    replaced=std::string(manifest);
    replaced.replace(replaced.find("art/level.hsd"),13,"/tmp/level.hsd");bad(replaced);
    bad(std::string(manifest)+std::string(kStageManifestBytes,'\n'));
    replaced=std::string(manifest);
    for(unsigned i=2;i<kStageSpawnCount;++i)replaced+="\nspawn=s"+std::to_string(i)+"|0|0|0";
    StagePackageManifest full;
    check(stage_package_read_manifest(replaced,&full,&error)&&full.spawns.size()==32,"exact stage marker count ceiling");
    bad(replaced+"\nspawn=excess|0|0|0");
    for(const auto* row:{"body-partition=wall|-20|-2|20|-2","body-partition=wall|19|-2|18|2",
                         "body-partition=wall|-20|-2|1300|2","body-partition=wall|-20|nan|20|2",
                         "body-partition=wall|-20|-2|20|2|extra"})
        bad(std::string(manifest)+"\n"+row);
    const std::string wall="\nbody-partition=wall|-20|-2|20|2";
    bad(std::string(manifest)+wall+wall);
    const StageBodyPartition expected_wall{"wall",-20,-2,20,2};
    check(stage_package_read_manifest(std::string(manifest)+wall,&full,&error)&&
          full.body_partitions.size()==1&&full.body_partitions.front()==expected_wall,
          "authored body partition parses separately from projectile grids");
    check(stage_package_read_manifest(std::string(manifest)+wall+"|melee",&full,&error)&&
          full.body_partitions.size()==1&&full.body_partitions.front().blocks_melee,
          "melee occlusion is an explicit per-partition opt-in");
    replaced=std::string(manifest);
    for(unsigned i=0;i<kStageBodyPartitionCount;++i)
        replaced+="\nbody-partition=wall-"+std::to_string(i)+"|-20|-2|20|2";
    check(stage_package_read_manifest(replaced,&full,&error),"exact partition count accepted");
    bad(replaced+wall);
}

[[maybe_unused]] void partition_contact_checks(const IBattleMvpTerrain& stage){
    NativeBorgContactRequest request;
    request.position={0,0,-80};request.delta={0,160};
    NativeBorgContactResult contact;
    check(native_borg_resolve_contact(request,&stage,&contact)&&contact.terrain_blocked&&!contact.body_hit&&!contact.slid&&
          contact.position[2]<-48&&contact.position[2]>-48.1f,"swept body stops before the thin partition by its radius");
    request.position=contact.position;request.delta={100,0};
    check(native_borg_resolve_contact(request,&stage,&contact)&&!contact.terrain_blocked&&contact.actual_delta.x==100,
          "a step parallel to the partition does not stick");
    request.position={0,0,-80};request.delta={100,160};
    check(native_borg_resolve_contact(request,&stage,&contact)&&contact.terrain_blocked&&contact.slid&&
          contact.position==BattleMvpVec{100,0,-80},"diagonal chooses clear tangent motion over the clipped step");
    request.position={700,0,-80};request.delta={0,160};
    check(native_borg_resolve_contact(request,&stage,&contact)&&!contact.terrain_blocked&&contact.position[2]==80,
          "walk around the end of a partition");
    request.position={0,900,-80};request.grounded=false;
    check(native_borg_resolve_contact(request,&stage,&contact)&&contact.terrain_blocked&&!contact.grounded&&contact.position[1]==900,
          "partition is full-height, not a new landing surface or flight rule");
    request.position={0,0,-250};request.delta={0,425};request.grounded=true;
    const std::array<BattleMvpVec,1> near_peer{BattleMvpVec{0,0,-140}},far_peer{BattleMvpVec{0,0,100}};
    check(native_borg_resolve_contact(request,near_peer,&stage,&contact)&&contact.body_hit&&!contact.terrain_blocked&&contact.position[2]<-224,
          "a nearer solid peer still stops movement before the partition");
    check(native_borg_resolve_contact(request,far_peer,&stage,&contact)&&!contact.body_hit&&contact.terrain_blocked&&contact.position[2]<-48,
          "the partition wins against an unreachable peer across it");
}
}

int main(int argc,char** argv){
    parse_checks();
#if !defined(__linux__)
    (void)argc;(void)argv;
    return 77;
#else
    check(argc==2,"generated content root required");
    const fs::path root=argv[1],stages=root/"stages",stage=stages/"workshop",renamed=stages/"unrelated-directory-name";
    fs::rename(stage,renamed);
    check(pc::scan_stage_packages(root/"missing").status==StagePackageScanStatus::NoDirectory,"missing root");
    fs::create_directory_symlink(stages,root/"linked-stages");
    check(pc::scan_stage_packages(root/"linked-stages"/".").status==StagePackageScanStatus::Unavailable,"root links refused");
    fs::create_directory_symlink(renamed,stages/"linked-workshop");
    auto catalog=pc::scan_stage_packages(stages);
    const auto* workshop=catalog.find("example:workshop");
    check(workshop&&workshop->folder=="unrelated-directory-name"&&workshop->issue==StagePackageIssue::None,"identity independent of directory");
    check(catalog.issues.size()==1&&catalog.issues[0].issue==StagePackageIssue::Malformed,"linked directory refused");
    fs::create_directories(stages/"duplicate");
    fs::copy_file(renamed/"stage.manifest",stages/"duplicate"/"stage.manifest");
    auto duplicate=pc::scan_stage_packages(stages);
    std::string error;
    std::shared_ptr<const StagePackageSnapshot> snapshot;
    check(!duplicate.find("example:workshop")&&
        !pc::pin_stage_package(stages,duplicate,"example:workshop",&snapshot,&error)&&error.find("duplicate")!=std::string::npos,
        "duplicate ID refuses even when one package has missing assets");
    fs::remove(stages/"duplicate"/"stage.manifest");fs::remove(stages/"duplicate");
    check(!pc::pin_stage_package(stages,catalog,"example:missing",&snapshot,&error)&&error.find("missing stage ID")!=std::string::npos,"no fallback stage");
    fs::rename(renamed/"art",renamed/"original-art");
    fs::create_directory_symlink(renamed/"original-art",renamed/"art");
    check(!pc::pin_stage_package(stages,catalog,"example:workshop",&snapshot,&error)&&error.find("linked")!=std::string::npos,
          "linked dependency directory after discovery is refused");
    fs::remove(renamed/"art");fs::rename(renamed/"original-art",renamed/"art");
    fs::create_hard_link(renamed/"art/workshop.hsd",root/"hard-link");
    auto linked=pc::scan_stage_packages(stages);
    check(linked.find("example:workshop")&&linked.find("example:workshop")->issue==StagePackageIssue::Malformed,"hard link refused");
    fs::remove(root/"hard-link");
    fs::rename(renamed/"collision/props.stih",renamed/"props-stash");
    auto missing=pc::scan_stage_packages(stages);
    check(missing.find("example:workshop")&&missing.find("example:workshop")->issue==StagePackageIssue::Missing,"list missing dependency by stage ID");
    fs::rename(renamed/"props-stash",renamed/"collision/props.stih");
    check(pc::pin_stage_package(stages,catalog,"example:workshop",&snapshot,&error),"pin exact stage and dependencies");
    write(renamed/"stage.manifest",manifest);
    write(renamed/"art/workshop.hsd","broken model");
    check(!pc::pin_stage_package(stages,catalog,"example:workshop",&snapshot,&error)&&error.find("changed")!=std::string::npos,
        "changed manifest refused; previous owned snapshot remains intact");

    Backend backend;RenderEngine render(backend);
    check(render.initialize({640,480,false,true,"portable stage"},nullptr,&error),"render initialization");
    NativeBattleMvpView view;
    check(view.bind_stage(*snapshot,render,&error),&error);
    check(view.stage_id()=="example:workshop"&&view.supports_projectile_stage(),"bound identity and projectile capability");
    NativeBorgStageProjectileHit hit;
    check(view.projectile_stage({0,120,400},{0,120,-400},&hit)&&hit.hit&&hit.layer==2&&std::abs(hit.fraction-.5f)<.001f,
           "existing projectile query uses the manifest's secondary grid");
    partition_contact_checks(view);
    const auto borg_root=root/"borgs";const auto borgs=pc::scan_borg_packages(borg_root);
    std::array<pc::BorgBattlePackageFiles,2> packages;
    check(pc::load_borg_battle_package(borg_root,borgs,"example:flight-stick","normal",&packages[0],&error),&error);
    check(pc::load_borg_battle_package(borg_root,borgs,"example:jump-stick","normal",&packages[1],&error),&error);
    BattleMvpConfig config;
    config.packages={packages[0].definition,packages[1].definition};
    const auto visuals=std::array{packages[0].visuals(),packages[1].visuals()};
    const std::array<std::shared_ptr<const BorgGameplayProfile>,2> definitions{
        config.packages[0],config.packages[1]};
    const std::array<std::string_view,2> markers{"east","west"};
    check(!view.configure_stage_duel("fixture:room",markers,&config,&error),"not a different stage");
    check(!view.configure_stage_duel(view.stage_id(),{"missing","west"},&config,&error)&&error.find("missing stage spawn")!=std::string::npos,
          "missing marker not replaced");
    check(!view.configure_stage_duel(view.stage_id(),{"east","east"},&config,&error),"same marker refused");
    check(view.configure_stage_duel(view.stage_id(),markers,&config,&error),&error);
    check(config.spawn[0].x==240&&config.spawn[1].x==-240&&config.spawn_query_y[0]==-10&&config.spawn_query_y[1]==20,"ordered markers and floor queries");
    check(view.load_packages(render,definitions,visuals,&error),&error);
    NativeBattleMvp world;
    check(world.start(config,&view)&&view.update(world,&error),"engine starts ordinary packaged duel");
    check(world.actors()[0].position==BattleMvpVec{240,0,-300}&&world.actors()[1].position==BattleMvpVec{-240,0,300}&&
           world.actors()[0].facing.x<0&&world.actors()[1].facing.x>0,"engine resolves support and facing, not raw authored height");
    {
        NativeBattleMvp walker;
        check(walker.start(config,&view),"start partition walk");
        std::array<GamePadState,2> pads{};pads[0].dpad_down=true;
        for(unsigned i=0;i<100;++i)check(walker.step(pads,false,&view),"advance walking toward partition");
        check(walker.actors()[0].position[2]<-48&&walker.actors()[0].position[2]>-48.1f,
              "shared packaged movement clips at the authored partition");
        auto near=config;near.spawn[0].z=-112;
        check(walker.start(near,&view),"start partition dash");
        pads[0].l=true;
        for(unsigned i=0;i<6;++i)check(walker.step(pads,false,&view),"advance dash toward partition");
        check(!walker.actors()[0].dash_ticks&&walker.actors()[0].dash_cooldown&&walker.actors()[0].position[2]<-48&&
              walker.actors()[0].position[2]>-48.1f,"partition contact ends the dash before timeout");
        auto overlapping=config;overlapping.spawn[0].z=-20;
        check(!walker.start(overlapping,&view)&&walker.config().spawn[0].z==-112,
              "world admission refuses a body overlapping a partition atomically");
    }
    Camera camera;
    camera.eye[1]=140;camera.eye[2]=680;camera.target[1]=120;camera.target[2]=-260;camera.far_plane=3000;
    render.begin_frame({0,0,0,0});render.reset_arena();
    const auto draws=view.draw(render,world,640,480,&error,false,0,&camera);
    render.end_frame();
    check(draws&&backend.submissions&&view.stage_view_alpha(0)==.5f,"manifest fade via shared stage renderer");
    check(world.step({},false,&view)&&view.update(world,&error)&&world.tick()==1,"authored battle advances");
    view.reset_control_test_pose();
    check(world.start(config,&view)&&view.update(world,&error)&&world.tick()==0,"restart reuses pinned stage after files changed");

    NativeBattleMvpView other;
    const std::array<std::string_view,6> roster_markers{"east","west","east-rear","east-outer","west-outer","west-rear"};
    auto roster=config;
    roster.declared_actors=roster_markers.size();
    roster.teams_declared=true;roster.teams={0,1,0,0,1,1};
    for(unsigned i=2;i<roster.declared_actors;++i)roster.packages[i]=definitions[roster.teams[i]];
    const auto before_roster=roster;
    auto rejected=roster_markers;rejected.back()="missing";
    check(!view.configure_stage_roster(view.stage_id(),rejected,&roster,&error)&&error.find("missing stage spawn")!=std::string::npos,
          "missing last CPU marker refused");
    rejected.back()=roster_markers[3];
    check(!view.configure_stage_roster(view.stage_id(),rejected,&roster,&error)&&error.find("markers must be distinct")!=std::string::npos,
          "repeated CPU marker refused");
    check(!view.configure_stage_duel(view.stage_id(),markers,&roster,&error)&&
          !view.configure_stage_roster(view.stage_id(),std::span(roster_markers).first(5),&roster,&error),
          "roster placement never fills missing slots");
    check(view.configure_stage_roster(view.stage_id(),roster_markers,&roster,&error),&error);
    check(world.start(roster,&view)&&world.actor_count()==6,"all named stage placements admit one world");
    for(unsigned i=0;i<world.actor_count();++i){
        const auto& query=snapshot->manifest.spawn(roster_markers[i])->query;
        check(world.actors()[i].position==BattleMvpVec{query[0],0,query[2]}&&world.actors()[i].team==roster.teams[i]&&
              roster.spawn_query_y[i]==query[1],"ordered actor placement keeps stage support and team ownership separate");
    }
    const auto stale=view.roster_framing(world,640,480);
    check(std::none_of(stale.bodies.begin(),stale.bodies.end(),[](const auto& body){return bool(body);}),
          "an older draw publication at the same tick cannot frame a restarted roster");
    auto high=std::make_shared<BorgGameplayProfile>(*roster.packages[4]);
    high->pose_ground_offset=-1000; // visible geometry only; the native actor still stands on the floor
    roster.packages[4]=high;
    roster.local_aim_cycle=true;
    std::array<BorgBattleVisualAssets,6> roster_assets{};
    for(unsigned i=0;i<roster_assets.size();++i)roster_assets[i]=visuals[roster.teams[i]];
    check(view.load_packages(render,roster.packages,roster_assets,&error)&&world.start(roster,&view)&&view.update(world,&error),&error);
    bool proxy_clips_body=false;
    const auto basis=NativeBattleMvpView::roster_movement_frame(world);
    for(const auto& [width,height]:{std::pair{640,480},std::pair{320,720}}){
        const auto framed=view.roster_framing(world,width,height);
        check(framed.bodies[4]&&framed.bodies[4]->high[1]>1000&&world.actors()[4].position[1]==0,
              "camera observes the lifted pose without moving its collision proxy");
        const auto proxy=NativeBattleMvpView::roster_camera_for(world,width,height);
        const auto actual_basis=NativeBattleMvpView::movement_frame(framed.camera);
        check(std::hypot(basis.forward.x-actual_basis.forward.x,basis.forward.z-actual_basis.forward.z)<.0001f,
              "posed framing keeps the same local movement axes");
        const auto in_view=[&](const Camera& c,const BattleMvpVec& point){
            f32 x=0,y=0;
            return c.project(point.data(),&x,&y,nullptr)&&x>=0&&x<=width&&y>=0&&y<=height;
        };
        for(const auto& body:framed.bodies){
            check(bool(body),"every living roster body has current visible bounds");
            for(unsigned n=0;n<8;++n){
                BattleMvpVec point{};
                for(unsigned k=0;k<3;++k)point[k]=n&(1u<<k)?body->high[k]:body->low[k];
                proxy_clips_body|=!in_view(proxy,point);
                check(in_view(framed.camera,point),"all visible body corners fit the roster camera");
            }
        }
    }
    check(proxy_clips_body,"collision-only framing demonstrably clips this pose");
    render.begin_frame({0,0,0,0});render.reset_arena();
    check(view.draw(render,world,640,480,&error,false,0,nullptr,true)&&view.package_body_draws(4),
          "the shared roster draw issues the high body along with the HUD");
    render.end_frame();

    {
        auto coop=roster;coop.teams={0,0,0,1,1,1};
        std::swap(coop.spawn[0],coop.spawn[1]);
        std::swap(coop.spawn_query_y[0],coop.spawn_query_y[1]);
        const auto begin=[&](){
            view.reset_control_test_pose();
            check(world.start(coop,&view)&&view.update(world,&error),&error);
        };
        Camera local_view;
        local_view.eye[1]=180;local_view.eye[2]=1000;
        local_view.target[1]=100;local_view.target[2]=300;
        local_view.far_plane=3500;
        const auto draw=[&](unsigned viewer,bool shared){
            render.begin_frame({0,0,0,0});render.reset_arena();
            check(view.draw(render,world,640,480,&error,false,viewer,&local_view,shared)!=0,&error);
            render.end_frame();
        };
        begin();
        draw(0,true);
        check(view.stage_view_alpha(0)==.5f,"P2 behind the wall fades it even when the camera target is in front");
        draw(1,false);
        check(view.stage_view_alpha(1)==1.f,"an independent viewport keeps its own camera-target gate");
        draw(0,true);
        check(view.stage_view_alpha(0)==.5f,"two local checks and a redraw still advance one fade per tick");
        check(world.step_roster(std::array<GamePadState,6>{},std::array<BorgBattleInputs,6>{},false,&view)&&
              view.update(world,&error),"advance shared fade test");
        draw(0,true);
        check(view.stage_view_alpha(0)&&std::abs(*view.stage_view_alpha(0)-.475f)<.0001f&&view.stage_view_alpha(1)==1.f,
              "next tick changes only the shared viewport's opacity");
        check(!view.draw(render,world,640,480,&error,false,2,&local_view,true)&&error.find("two-view scope")!=std::string::npos,
              "an extra actor slot is not a valid stage-fade viewport");

        coop.spawn[1]={240,300};
        local_view.target[2]=-300; // the centre ray now hits the wall; neither local ray does
        begin();
        draw(0,true);
        check(view.stage_view_alpha(0)==1.f,"an obstructed group target cannot fade the wall when both locals are clear");
    }

    auto incompatible=*snapshot;
    for(auto& marker:incompatible.manifest.spawns)if(marker.id==roster_markers.back()){
        marker.query=incompatible.manifest.spawn("east-rear")->query;
        marker.query[0]+=83;
    }
    auto failed=before_roster;failed.arena_half=1234;
    const auto unchanged=failed;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_roster(other.stage_id(),roster_markers,&failed,&error)&&
          error.find("overlap")!=std::string::npos,"physical overlap between distinct CPU markers refused");
    check(failed.arena_half==unchanged.arena_half&&failed.spawn_query_y==unchanged.spawn_query_y,
          "rejected placement leaves stage bounds and height queries untouched");
    for(unsigned i=0;i<kBattleMvpMaxActors;++i)
        check(failed.spawn[i].x==unchanged.spawn[i].x&&failed.spawn[i].z==unchanged.spawn[i].z,
              "rejected placement leaves every spawn untouched");
    incompatible=*snapshot;
    incompatible.manifest.spawns[0].query[0]=1790;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("outside arena")!=std::string::npos,"bounds check includes actor radius");
    incompatible.manifest.spawns[0].query=incompatible.manifest.spawns[1].query;
    incompatible.manifest.spawns[0].query[0]+=83;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("overlap")!=std::string::npos,"physical overlap of different markers refused");
    incompatible=*snapshot;
    incompatible.manifest.spawns[0].query[2]=-20;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("intersects a body partition")!=std::string::npos,
          "stage refuses a marker whose fighter radius enters the partition");
    incompatible.manifest.body_partitions.clear();
    check(other.bind_stage(incompatible,render,&error)&&other.configure_stage_duel(other.stage_id(),markers,&config,&error),
          "projectile-only geometry does not implicitly obstruct bodies");
    NativeBorgContactRequest through;
    through.position={0,0,-80};through.delta={0,160};
    NativeBorgContactResult unobstructed;
    check(native_borg_resolve_contact(through,&other,&unobstructed)&&!unobstructed.terrain_blocked&&unobstructed.position[2]==80,
          "same projectile wall is passable without the authored body partition");
    incompatible=*snapshot;
    // Mask the floor cell's list (left spawn is in cell x=3,z=4).
    auto& floor=incompatible.bytes[unsigned(StageAsset::Floor)];
    const std::size_t cell=40+4*(3*8+4),sentinel=64*4+40+8-8;
    floor[cell+2]=u8(sentinel>>8);floor[cell+3]=u8(sentinel);
    for(const auto asset:{StageAsset::ProjectileStage,StageAsset::Secondary,StageAsset::Tertiary}){
        incompatible.manifest.asset_paths[unsigned(asset)].clear();incompatible.bytes[unsigned(asset)].clear();
    }
    check(other.bind_stage(incompatible,render,&error)&&!other.supports_projectile_stage(),"fade does not require projectile capabilities");
    f32 partition_clip=1;
    check(other.body_partition_sweep({0,-80},{0,160},42,&partition_clip)&&partition_clip<.21f,
          "authored body partition does not require any projectile grid");
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("no supported floor")!=std::string::npos,
          "unsupported spawn cannot use default floor");
    check(!other.load_packages(render,definitions,visuals,&error)&&error.find("unsupported stage projectile capability")!=std::string::npos,
          "Borg requiring collision cannot use a floor-only stage");
    incompatible=*snapshot;
    incompatible.bytes[unsigned(StageAsset::ProjectileStage)][0]=0;
    check(!other.bind_stage(incompatible,render,&error)&&other.stage_id().empty()&&!other.supports_projectile_stage(),"invalid GFSR blocks binding");
    incompatible=*snapshot;
    incompatible.bytes[unsigned(StageAsset::ViewFade)]={1};
    check(!other.bind_stage(incompatible,render,&error)&&other.stage_id().empty(),"invalid fade is not silently dropped");
    for(const auto& [asset,bytes]:std::array{
        std::pair{StageAsset::Floor,std::vector<u8>{120,78,0,90,42}},
        std::pair{StageAsset::Model,std::vector<u8>{87,132,0,157,219}},
        std::pair{StageAsset::Secondary,std::vector<u8>{199,22,5,94,152}},
        std::pair{StageAsset::Tertiary,std::vector<u8>{139,182,2,145,252}},
    }){
        // Synthetic five-byte fingerprints; no retail asset is needed.
        incompatible=*snapshot;incompatible.bytes[unsigned(asset)]=bytes;
        check(!other.bind_stage(incompatible,render,&error)&&error.find("exact legacy qualification")!=std::string::npos,
              "portable path refuses pinned retail signatures independently");
    }
    render.shutdown();
    std::puts("portable stage: manifest, discovery, pinned rendering/collision and engine placement PASS");
    return 0;
#endif
}
