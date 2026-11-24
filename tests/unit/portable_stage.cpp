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
    const auto borg_root=root/"borgs";const auto borgs=pc::scan_borg_packages(borg_root);
    std::array<pc::BorgBattlePackageFiles,2> packages;
    check(pc::load_borg_battle_package(borg_root,borgs,"example:flight-stick","normal",&packages[0],&error),&error);
    check(pc::load_borg_battle_package(borg_root,borgs,"example:jump-stick","normal",&packages[1],&error),&error);
    BattleMvpConfig config;
    config.packages={packages[0].definition,packages[1].definition};
    const auto visuals=std::array{packages[0].visuals(),packages[1].visuals()};
    const std::array<std::string_view,2> markers{"east","west"};
    check(!view.configure_stage_duel("fixture:room",markers,&config,&error),"not a different stage");
    check(!view.configure_stage_duel(view.stage_id(),{"missing","west"},&config,&error)&&error.find("missing stage spawn")!=std::string::npos,
          "missing marker not replaced");
    check(!view.configure_stage_duel(view.stage_id(),{"east","east"},&config,&error),"same marker refused");
    check(view.configure_stage_duel(view.stage_id(),markers,&config,&error),&error);
    check(config.spawn[0].x==240&&config.spawn[1].x==-240&&config.spawn_query_y[0]==-10&&config.spawn_query_y[1]==20,"ordered markers and floor queries");
    check(view.load_packages(render,config.packages,visuals,&error),&error);
    NativeBattleMvp world;
    check(world.start(config,&view)&&view.update(world,&error),"engine starts ordinary packaged duel");
    check(world.actors()[0].position==BattleMvpVec{240,0,-300}&&world.actors()[1].position==BattleMvpVec{-240,0,300}&&
          world.actors()[0].facing.x<0&&world.actors()[1].facing.x>0,"engine resolves support and facing, not raw authored height");
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
    auto incompatible=*snapshot;
    incompatible.manifest.spawns[0].query[0]=1790;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("outside arena")!=std::string::npos,"bounds check includes actor radius");
    incompatible.manifest.spawns[0].query=incompatible.manifest.spawns[1].query;
    incompatible.manifest.spawns[0].query[0]+=83;
    check(other.bind_stage(incompatible,render,&error),&error);
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("overlap")!=std::string::npos,"physical overlap of different markers refused");
    incompatible=*snapshot;
    // Mask the floor cell's list (left spawn is in cell x=3,z=4).
    auto& floor=incompatible.bytes[unsigned(StageAsset::Floor)];
    const std::size_t cell=40+4*(3*8+4),sentinel=64*4+40+8-8;
    floor[cell+2]=u8(sentinel>>8);floor[cell+3]=u8(sentinel);
    for(const auto asset:{StageAsset::ProjectileStage,StageAsset::Secondary,StageAsset::Tertiary}){
        incompatible.manifest.asset_paths[unsigned(asset)].clear();incompatible.bytes[unsigned(asset)].clear();
    }
    check(other.bind_stage(incompatible,render,&error)&&!other.supports_projectile_stage(),"fade does not require projectile capabilities");
    check(!other.configure_stage_duel(other.stage_id(),markers,&config,&error)&&error.find("no supported floor")!=std::string::npos,
          "unsupported spawn cannot use default floor");
    check(!other.load_packages(render,config.packages,visuals,&error)&&error.find("unsupported stage projectile capability")!=std::string::npos,
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
