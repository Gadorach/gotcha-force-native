// Exercise the real desktop entry point's loop without a window, audio device,
// or production test-input option. The renamed entry point is not called.
#define main gf_desktop_entry
#include "platform/pc/native_main.cpp"
#undef main

namespace {
enum class Replay {Session,SingleStep,Jump,Camera,Separate,RefusedInput,RaisedFace,FixtureWalk,FixtureRestart,PickerPractice,PickerExpiry,ForcePicker};

class ReplayInput final:public gf::IInputBackend,public gf::IClock {
public:
    explicit ReplayInput(Replay replay):replay_(replay){}
    void pump()override{
        snapshot_.clear();snapshot_.port_count=1;
        auto& raw=snapshot_.pads[0];const unsigned f=frame_++;
        switch(replay_){
        case Replay::Session:
            if(f==3||f==5||f==7)raw.buttons|=(1u<<4)|1u; //Enter with an unsupported combat button.
            if(f>=7&&f<=9)raw.buttons|=1u<<3;            //Q at a zero-tick frame, then held.
            break;
        case Replay::SingleStep:
            if(f==3||f==10)raw.buttons|=1u<<4;
            snapshot_.frame_step=(f>=4&&f<=5)||f==7||f==9||f==11;
            if(f==4)raw.buttons|=1u<<1;
            if(f==7)raw.buttons|=1u; //A refused step leaves the tick and pause intact.
            break;
        case Replay::Jump:
        case Replay::Camera:
        case Replay::Separate:
            if(f==3)raw.buttons|=1u<<1;
            if(replay_==Replay::Camera)snapshot_.camera_toggle=(f>=3&&f<=5)||f==24;
            if(replay_==Replay::Separate&&f==5)raw.axis[0]=32767;
            break;
        case Replay::RefusedInput:
            if(f>=8&&f<=10)raw.buttons|=1u;
            break;
        case Replay::RaisedFace:
            if(f>=5&&f<31)raw.buttons|=1u<<1;
            if(f>=78&&f<105)raw.axis[0]=-32768;
            if(f>=132&&f<136)raw.buttons|=1u<<3;
            if(f==132)raw.buttons|=1u;
            break;
        case Replay::FixtureWalk:
        case Replay::FixtureRestart:
            if(f>=5&&f<25)raw.buttons|=1u<<7; // Actual D-pad left from the verified foothold.
            if(replay_==Replay::FixtureRestart&&f>=75&&f<79)raw.buttons|=1u<<3;
            break;
        case Replay::PickerPractice:
            // Hold both launch and exit across the renderer/session handoff.
            if(f<=3||(f>=16&&f<=18))raw.buttons|=1u<<3;
            if((f>=11&&f<=13)||(f>=22&&f<=24))raw.quit=true;
            if(f==14)raw.buttons|=1u<<6; // Held on the return/release frame, not a fresh collection move.
            if(f==15)raw.buttons|=1u<<7; // Wrap from Standard to the timer-expiry foothold for the second launch.
            break;
        case Replay::PickerExpiry:
            if(f==0||(f>=7&&f<27))raw.buttons|=1u<<7; // First choose the route, then walk with live input.
            if(f>=1&&f<=4)raw.buttons|=1u<<3;
            if(f>=190&&f<=192)raw.quit=true;
            break;
        case Replay::ForcePicker:
            if(f<=3||f==18||(f>=22&&f<=24))raw.buttons|=1u<<3;
            if((f>=11&&f<=13)||(f>=30&&f<=32))raw.quit=true;
            if(f==14||f==16)raw.buttons|=1u<<5; // First edge is consumed by return; second selects overlap.
            if(f==20)raw.buttons|=1u<<6;
            break;
        }
    }
    const gf::InputSnapshot& snapshot()const override{return snapshot_;}
    gf::u64 monotonic_ns()override{
        if(frame_<3)return gf::u64(frame_)*20000000;
        if(frame_<=7)return 40000000+gf::u64(frame_-2)*1000000;
        return 45000000+gf::u64(frame_-7)*17000000;
    }
    void sleep_for_ns(gf::u64)override{}
private:
    Replay replay_;
    unsigned frame_=0;
    gf::InputSnapshot snapshot_;
};

class ReplayLog final:public gf::ILogBackend {
public:
    void write(gf::LogLevel,const char*,const std::string& message)override{
        if(text.size()<16384){text+=message;text+='\n';}
    }
    std::string text;
};

void check(bool ok,const char* scenario,const std::string& log){
    if(!ok){std::fprintf(stderr,"G-Red practice replay failed: %s\n%s",scenario,log.c_str());std::exit(1);}
}

std::string run(Replay replay,unsigned frames,const char* scenario){
    auto files=gf::sdl::create_file_system();
    gf::headless::Bundle headless;ReplayInput input(replay);ReplayLog log;
    auto backends=headless.get();
    backends.files=files.get();backends.input=&input;backends.clock=&input;backends.log=&log;
    Options options;
    const bool fixture=replay==Replay::FixtureWalk||replay==Replay::FixtureRestart;
    char program[]="gf_native",practice[]="--gred-movement",expiry[]="--gred-walk-ledge-expiry-push";
    char audio[]="--no-audio",start[]="--practice-start";
    char* args[]={program,fixture?expiry:practice,audio,start};
    check(parse(fixture?4:3,args,&options),scenario,log.text);
    options.frames=frames;
    options.headless=replay==Replay::RaisedFace||fixture; // Exactly one scheduled tick per frame.
    gf::Context context(backends,60);gf::RenderEngine render(*backends.render);std::string error;
    check(render.initialize({640,480,false,true,"Practice replay"},nullptr,&error),scenario,error);
    check(run_battle_mvp(context,render,options)==0,scenario,log.text);
    check(log.text.find("projectiles-spawned=0 hits=0 health=100/100")!=std::string::npos,scenario,log.text);
    return log.text;
}

bool has(const std::string& log,const char* text){return log.find(text)!=std::string::npos;}
bool once(const std::string& log,const char* text){
    const auto first=log.find(text);
    return first!=std::string::npos&&log.find(text,first+1)==std::string::npos;
}
std::size_t occurrences(std::string_view text,std::string_view part){
    std::size_t found=0;
    for(std::size_t at=0;(at=text.find(part,at))!=std::string_view::npos;at+=part.size())++found;
    return found;
}
std::string line(const std::string& log,const char* prefix){
    const auto start=log.find(prefix);
    return start==std::string::npos?std::string{}:log.substr(start,log.find('\n',start)-start);
}

// Test-only entry point. Create a fresh isolated account store rather
// than opening any existing data to manufacture a roster.
int seed_owned_practice(const std::string& root,const char* retail_path=nullptr){
    gf::ForceCatalog catalogue;
    if(retail_path){
        const auto files=gf::sdl::create_file_system();
        if(!files||!load_retail_force_catalogue(*files,retail_path,&catalogue))return 2;
    }
    std::error_code ec;
    if(!std::filesystem::create_directory(root,ec)||ec)return 2;
    auto storage=gf::pc::create_account_save_storage(root,false,nullptr);
    if(!storage)return 1;
    gf::AccountSaveRepository repo(*storage);
    gf::AccountSaveEntry account,save,other,empty;
    using R=gf::AccountResult;
    if(repo.create_account(7,"Practice Fixtures",&account)!=R::Ok||
       repo.create_save(account,11,"Owned Practice",&save)!=R::Ok||
       repo.create_save(account,12,"Other Practice",&other)!=R::Ok||
       repo.create_save(account,13,"No Roster",&empty)!=R::Ok)return 1;
    gf::PackageBorgRoster roster{{
        {8,{"fixture:gred","Saved G-Red"},false,gf::SavedPackageVariant{"normal","Saved Standard"}},
        {9,{"fixture:gred","Disabled G-Red"},true,gf::SavedPackageVariant{"normal","Disabled"}},
        {10,{"fixture:gred","Legacy G-Red"}},
        {11,{"fixture:gred","Shadow G-Red"},false,gf::SavedPackageVariant{"shadow","Shadow"}},
        {12,{"fixture:missing","Missing Borg"},false,gf::SavedPackageVariant{"normal","Missing"}},
    },{{8,0,1}}};
    if(retail_path){
        roster.owned.push_back({13,{"fixture:gred","Overlap G-Red"},false,gf::SavedPackageVariant{"normal","Normal"}});
        roster.placements.insert(roster.placements.begin(),{13,0,0});
        roster.placements.push_back({9,0,2});
    }
    const gf::PackageBorgRoster other_roster{{
        {8,{"fixture:missing","Other instance eight"},false,gf::SavedPackageVariant{"normal","Missing"}},
    },{}};
    auto write=[&](const gf::AccountSaveEntry& entry,const gf::PackageBorgRoster& value,bool with_retail){
        std::unique_ptr<gf::AccountSaveBackend> backend;
        if(repo.open(entry,&backend)!=R::Ok)return false;
        gf::PackageBorgSaveStore store(*backend);gf::PackageBorgRoster unused;
        bool ok=store.load(&unused)==gf::PackageBorgLoad::Missing&&store.write(value);
        if(ok&&with_retail){
            gf::ForceInventoryStore retail_store(*backend);gf::ForceInventory inventory;
            ok=retail_store.load(catalogue,&inventory)==gf::ForceInventoryLoad::Missing&&inventory.create(catalogue)&&
                inventory.place(1,0,0)==gf::ForceChange::Applied&&retail_store.write(inventory);
        }
        repo.close();return ok;
    };
    return write(save,roster,retail_path!=nullptr)&&write(other,other_roster,false)?0:1;
}

int replay_owned_practice_picker(std::string root){
    auto files=gf::sdl::create_file_system();
    gf::headless::Bundle headless;ReplayInput input(Replay::PickerPractice);ReplayLog log;
    auto backends=headless.get();backends.files=files.get();backends.input=&input;backends.clock=&input;backends.log=&log;
    auto run_picker=[&](char* instance,char* limit){
        Options options;
        char program[]="gf_native",mode[]="--browse-owned-borgs",account[]="7",save[]="11";
        char owned[]="--owned-instance",practice[]="--practice-from-picker",off[]="--headless",frames[]="--frames";
        char* args[]={program,mode,root.data(),account,save,owned,instance,practice,off,frames,limit};
        check(parse(int(std::size(args)),args,&options),"owned picker options",log.text);
        gf::Context context(backends,60);gf::RenderEngine render(context.render());std::string error;
        check(render.initialize({640,480,false,true,"Owned picker replay"},nullptr,&error),"picker renderer",error);
        const auto packages=gf::pc::scan_borg_packages("borgs");
        check(run_borg_browser(context,render,options,packages)==0,"owned picker practice",log.text);
        return context.frame_index();
    };
    char enabled[]="8",short_limit[]="32",long_limit[]="205";
    check(run_picker(enabled,short_limit)==32&&occurrences(log.text,"launched practice for owned instance=8")==2&&
        occurrences(log.text,"returned to owned instance=8 after practice")==2&&
        occurrences(log.text,"owned instance=8 (Saved G-Red / Saved Standard); save is read-only")==2&&
        has(log.text,"launched practice for owned instance=8 route=STANDARD")&&
        has(log.text,"launched practice for owned instance=8 route=LEDGE TIMER EXPIRY")&&
        has(log.text,"returned to owned instance=8 after practice route=LEDGE TIMER EXPIRY")&&
        has(log.text,"G-Red-position=-1250.97/")&&
        !has(log.text,"practice restarted at tick=0")&&has(log.text,"selected=fixture:gred variant=normal view=picker")&&
        has(log.text,"frames=32 packages=1")&&has(log.text,"instance=8 saved-variant=normal"),
        "two distinct routes, held-key isolation, exact return and one total frame budget",log.text);
    log.text.clear();input=ReplayInput(Replay::PickerExpiry);
    check(run_picker(enabled,long_limit)==205&&once(log.text,"launched practice for owned instance=8 route=LEDGE TIMER EXPIRY")&&
        once(log.text,"practice start: demo placement, live input, no scripted replay")&&
        !has(log.text,"walk ledge expiry demo:")&&has(log.text,"ground-departures=1 loss-entries=1 loss-landings=1")&&
        has(log.text,"walk-ledge-fall-pushes=2 last-angle-s16=13653 landings=1 resource=10000 selector=0")&&
        once(log.text,"returned to owned instance=8 after practice route=LEDGE TIMER EXPIRY"),
        "selected foothold runs the full route from live input and returns with the route retained",log.text);
    log.text.clear();input=ReplayInput(Replay::PickerPractice);
    char disabled[]="9";
    check(run_picker(disabled,short_limit)>0&&!has(log.text,"launched practice for")&&
        has(log.text,"practice unavailable for instance=9: the requested owned copy is disabled")&&
        has(log.text,"instance=9 saved-variant=normal disabled=1"),
        "disabled copy stays in the collection but cannot launch",log.text);
    std::puts("Owned picker practice: exact launch/return, live routes, disabled refusal, and held-key isolation PASS");
    return 0;
}

int replay_force_practice_picker(std::string root,std::string retail_path){
    auto files=gf::sdl::create_file_system();gf::ForceCatalog retail;
    check(files&&load_retail_force_catalogue(*files,retail_path,&retail),"fixture catalogue","");
    gf::headless::Bundle headless;ReplayInput input(Replay::ForcePicker);ReplayLog log;
    auto backends=headless.get();backends.files=files.get();backends.input=&input;backends.clock=&input;backends.log=&log;
    Options options;
    char program[]="gf_native",mode[]="--browse-borg-force",account[]="7",save[]="11",force[]="1",slot[]="2";
    char select[]="--force-slot",practice[]="--practice-from-picker",catalogue[]="--retail-catalog",
         off[]="--headless",frames[]="--frames",count[]="44";
    char* args[]={program,mode,root.data(),account,save,force,select,slot,practice,catalogue,retail_path.data(),off,frames,count};
    check(parse(int(std::size(args)),args,&options),"Force browser options",log.text);
    const auto packages=gf::pc::scan_borg_packages("borgs");std::string error;
    auto snapshot=gf::pc::BorgForceBrowseSnapshot::open(root,{7,11},"borgs",packages,std::move(retail),&error);
    check(bool(snapshot),"verified mixed fixture",error);
    gf::Context context(backends,60);gf::RenderEngine render(context.render());
    check(render.initialize({640,480,false,true,"Force picker replay"},nullptr,&error),"Force browser renderer",error);
    check(run_borg_force_browser(context,render,options,packages,*snapshot)==0&&context.frame_index()==44,
        "Force browser has a single frame budget across both handoffs",log.text);
    check(occurrences(log.text,"launched practice for Force 1 slot 2 instance=8")==2&&
        occurrences(log.text,"returned to Force 1 slot 2 after practice")==2&&
        occurrences(log.text,"Force 1 slot 2 supplies owned instance=8; one practice copy, not Force admission")==2&&
        once(log.text,"practice unavailable for Force 1 slot 1: the requested Force slot overlaps retail and package claims")&&
        !has(log.text,"practice restarted at tick=0")&&snapshot->current(),
        "exact Force slot launch and return, held input isolation, overlap refusal, and unchanged scope",log.text);
    std::puts("Force browser practice: exact slot launch/return and overlap refusal PASS");
    return 0;
}
}

int main(int argc,char** argv){
    if(argc!=1){
        if(argc==4){
            const std::string_view command=argv[1];
            if(command=="--seed-force-practice")return seed_owned_practice(argv[2],argv[3]);
            if(command=="--replay-force-practice-picker")return replay_force_practice_picker(argv[2],argv[3]);
            return 2;
        }
        if(argc!=3)return 2;
        const std::string_view command=argv[1];
        if(command=="--seed-owned-practice")return seed_owned_practice(argv[2]);
        if(command=="--replay-owned-practice-picker")return replay_owned_practice_picker(argv[2]);
        return 2;
    }
    const auto files=gf::sdl::create_file_system();
    if(!files->exists("local/content/source-gred-ground-v9/motion.gfmm")||
       !files->exists("local/content/native-battle-mvp-v1/stage.hsd")){
        std::puts("G-Red practice replay skipped: qualified local movement and stage data are required");
        return 77;
    }
    const auto session=run(Replay::Session,12,"session controls");
    check(once(session,"practice paused at tick=2")&&once(session,"practice resumed at tick=2")&&
        once(session,"practice restarted at tick=0")&&has(session,"frames=12 ticks=4")&&
        !has(session,"input refused"),"between-tick controls and restart priority",session);

    const auto jump=run(Replay::Jump,12,"short jump");
    check(has(jump,"frames=12 ticks=6")&&has(jump,"practice-phase=TAKEOFF")&&has(jump,"ordered-entries=0"),
        "between-tick jump reaches its next movement tick",jump);

    const auto standard=run(Replay::Jump,26,"standard practice camera");
    const auto close=run(Replay::Camera,26,"toggle practice camera");
    check(once(close,"practice camera=close")&&once(close,"practice camera=standard")&&
        has(close,"frames=26 ticks=21")&&has(close,"practice-phase=FALLING")&&
        line(standard,"accepted native tick=")==line(close,"accepted native tick=")&&
        line(standard,"practice-meter=")==line(close,"practice-meter="),
        "camera toggles neither consume nor change an accepted movement tick",close);
    const auto stepped=run(Replay::SingleStep,12,"paused single step");
    check(once(stepped,"practice paused at tick=2")&&once(stepped,"practice stepped to tick=3")&&
        once(stepped,"no simulation advanced")&&once(stepped,"practice stepped to tick=4")&&
        once(stepped,"practice resumed at tick=4")&&!has(stepped,"practice stepped to tick=5")&&
        has(stepped,"frames=12 ticks=5")&&has(stepped,"practice-phase=TAKEOFF"),
        "single step uses live input, does not repeat when held, and preserves pause after a refusal",stepped);

    const auto separate=run(Replay::Separate,12,"separate inputs");
    check(has(separate,"frames=12 ticks=6")&&has(separate,"practice-phase=GROUND DASH")&&
        has(separate,"ordered-entries=1 active=3")&&!has(separate,"practice-phase=TAKEOFF"),
        "separately sampled jumps and flicks do not merge",separate);

    const auto refused=run(Replay::RefusedInput,12,"refused input");
    check(once(refused,"no simulation advanced")&&has(refused,"frames=12 ticks=3")&&
        has(refused,"practice-meter=10000/10000 at accepted tick=3")&&!has(refused,"practice stopped")&&
        !has(refused,"practice restarted"),"refused input recovers on release without a restart",refused);

    const auto stopped=run(Replay::RaisedFace,120,"raised face stop");
    check(once(stopped,"practice stopped at tick=104")&&has(stopped,"frames=120 ticks=104")&&!has(stopped,"practice-meter="),
        "raised face does not advance, fall back or retain its meter",stopped);
    const auto restarted=run(Replay::RaisedFace,141,"raised face restart");
    check(once(restarted,"practice stopped at tick=104")&&once(restarted,"practice restarted at tick=0")&&
        has(restarted,"frames=141 ticks=8")&&has(restarted,"practice-meter=10000/10000 at accepted tick=8"),
        "held Q restarts once and the meter returns on a fresh source frame",restarted);
    const auto walked=run(Replay::FixtureWalk,180,"live expiry fixture");
    check(once(walked,"practice start: demo placement, live input, no scripted replay")&&
        !has(walked,"walk ledge expiry demo:")&&has(walked,"frames=180 ticks=180")&&
        has(walked,"ground-departures=1 loss-entries=1 loss-landings=1")&&
        has(walked,"walk-ledge-fall-pushes=2 last-angle-s16=13653 landings=1 resource=10000 selector=0"),
        "real input follows the verified route from the shared demo placement, with no replay injected",walked);
    const auto fixture_restart=run(Replay::FixtureRestart,86,"restart from the fixture mid-fall");
    check(once(fixture_restart,"practice restarted at tick=0")&&has(fixture_restart,"frames=86 ticks=10")&&
        has(fixture_restart,"ground-departures=0 loss-entries=0 loss-landings=0")&&
        has(fixture_restart,"walk-ledge-fall-pushes=0 last-angle-s16=0 landings=0 resource=10000")&&
        has(fixture_restart,"accepted native tick=10 G-Red-position=-1250.97/")&&
        has(fixture_restart,"/-3313.80; accepted-frames=10")&&has(fixture_restart,"practice-phase=IDLE"),
        "held Q returns to the same grounded fixture once, clearing old motion and counters",fixture_restart);
    std::puts("G-Red practice replay: eleven desktop-loop scenarios PASS without opening devices");
}
