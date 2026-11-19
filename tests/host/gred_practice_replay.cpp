// Exercise the real desktop entry point's loop without a window, audio device,
// or production test-input option. The renamed entry point is not called.
#define main gf_desktop_entry
#include "platform/pc/native_main.cpp"
#undef main

namespace {
enum class Replay {Session,Jump,Separate,RefusedInput,RaisedFace};

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
        case Replay::Jump:
        case Replay::Separate:
            if(f==3)raw.buttons|=1u<<1;
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
    char program[]="gf_native",practice[]="--gred-movement",audio[]="--no-audio";
    char* args[]={program,practice,audio};
    check(parse(3,args,&options),scenario,log.text);
    options.frames=frames;
    options.headless=replay==Replay::RaisedFace; //Exactly one scheduled tick per frame before the stop.
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
}

int main(){
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
    std::puts("G-Red practice replay: six desktop-loop scenarios PASS without opening devices");
}
