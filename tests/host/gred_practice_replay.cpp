// Exercise the real desktop entry point's loop without a window, audio device,
// or production test-input option. The renamed entry point is not called.
#define main gf_desktop_entry
#include "platform/pc/native_main.cpp"
#undef main

namespace {
enum class Replay {Session,SingleStep,Jump,Camera,Separate,RefusedInput,RaisedFace,FixtureWalk,FixtureRestart,
    RecoverySeamRestart,InclineDashJump,TimedInclineLandingDash,DirectExpiryRelaunch,ExpiringDirectRelaunch,ExpiringDirectRelaunchPush,
    ExpiringAdjacentRelaunchPush,ExpiringAirDash,ExpiringAirDashRelaunch,ExpiringShortAirDashRelaunch,
    TimedSnapDash,FloorWindupRelaunch,FloorWindupDash,
    PositiveFloorHandoff,ExpiringFloorHandoff,DirectTimedRelaunch,DirectTimedUpRelaunch,ActiveSnapRelaunch,TimedReleasePush,TimedReleaseTwoPushes,
    PickerPractice,PickerExpiry,ForcePicker,ForceExpiryAirDash,OwnedRecoverySeam,ForceRecoverySeam,
    OwnedTimedHandoff,ForceExpiryHandoff,OwnedTimedFloor,ForceTimedFloor,OwnedInclineJump,ForceInclineJump,
    OwnedLandingDash,ForceLandingDash,OwnedDirectExpiry,ForceDirectExpiry,
    OwnedPositiveFloor,ForcePositiveFloor,OwnedExpiredFloor,ForceExpiredFloor,OwnedSnapDash,ForceSnapDash,
    OwnedExpiryStep,ForceExpiryStep,OwnedDirectTimed,ForceDirectTimed,OwnedDirectTimedUp,ForceDirectTimedUp,
    OwnedActiveSnap,ForceActiveSnap,OwnedExpiringDirect,ForceExpiringDirect,OwnedExpiringEntryPush,ForceExpiringEntryPush,
    OwnedAdjacentEntryPush,ForceAdjacentEntryPush,OwnedExpiringAirDash,ForceExpiringAirDash,OwnedExpiringDashRelaunch,ForceExpiringDashRelaunch,
    OwnedTimedReleasePush,ForceTimedReleasePush,OwnedTimedReleaseTwoPushes,ForceTimedReleaseTwoPushes};

enum class TimedPickerInputs {PositiveFloor,ExpiredFloor,SnapDash,ExpiryStep,DirectTimed,DirectTimedUp,ActiveSnap,
    ExpiringDirect,ExpiringEntryPush,AdjacentEntryPush,ExpiringAirDash,ExpiringDashRelaunch,ReleasePush,ReleaseTwoPushes};
struct TimedPickerReplay {
    Replay sequence;
    TimedPickerInputs inputs;
    bool owned;
    unsigned advances,ticks;
    const char* route;
    const char* guide;
    const char* expected;
    const char* receipt;
    bool previous=false;
    constexpr unsigned first_step()const{return 2*advances+5;}
    constexpr unsigned after_steps()const{return first_step()+2*ticks;}
    constexpr unsigned budget()const{return after_steps()+12;}
};
constexpr TimedPickerReplay timed_picker_replays[]{
    {Replay::OwnedPositiveFloor,TimedPickerInputs::PositiveFloor,true,6,32,"POSITIVE FLOOR HANDOFF","positive floor wind-up relaunch",
        "practice-meter=9250/10000 at accepted tick=32","practice-ledge-timer-ticks=6"},
    {Replay::ForcePositiveFloor,TimedPickerInputs::PositiveFloor,false,6,110,"POSITIVE FLOOR HANDOFF","positive floor wind-up relaunch",
        "accepted native tick=110 G-Red-position=1964.66/10.00/-1607.42","controller-loss-relaunches=1 timer698=0.0"},
    {Replay::OwnedExpiredFloor,TimedPickerInputs::ExpiredFloor,true,7,155,"EXPIRED FLOOR HANDOFF","floor wind-up relaunch",
        "practice-meter=9300/10000 at accepted tick=155","practice-floor-windup-handoff=SHIFT/B RELAUNCH / NEUTRAL DASH"},
    {Replay::ForceExpiredFloor,TimedPickerInputs::ExpiredFloor,false,7,220,"EXPIRED FLOOR HANDOFF","floor wind-up relaunch",
        "accepted native tick=220 G-Red-position=-2668.02/10.00/594.38","controller-loss-relaunches=2 timer698=0.0"},
    {Replay::OwnedSnapDash,TimedPickerInputs::SnapDash,true,8,150,"TIMED SNAP DASH","timed snap to air dash",
        "practice-meter=9750/10000 at accepted tick=150","practice-relaunch-snap-windup=AIRBORNE / RELAUNCH TO WIND-UP"},
    {Replay::ForceSnapDash,TimedPickerInputs::SnapDash,false,8,220,"TIMED SNAP DASH","timed snap to air dash",
        "accepted native tick=220 G-Red-position=-3094.35/10.00/1244.92","snap-dash-starts=1 windup-entry-floors=1 windup-floors=6 active-floors=30"},
    {Replay::OwnedExpiryStep,TimedPickerInputs::ExpiryStep,true,9,55,"EXPIRY STEP HANDOFF","expiring floor wind-up relaunch",
        "practice-meter=9250/10000 at accepted tick=55","controller-loss-relaunches=2 timer698=0.0"},
    {Replay::ForceExpiryStep,TimedPickerInputs::ExpiryStep,false,9,135,"EXPIRY STEP HANDOFF","expiring floor wind-up relaunch",
        "accepted native tick=135 G-Red-position=3777.91/10.00/-880.03","controller-loss-relaunches=1 timer698=0.0"},
    {Replay::OwnedDirectTimed,TimedPickerInputs::DirectTimed,true,10,25,"DIRECT TIMED RELAUNCH","direct timed relaunch",
        "practice-meter=9850/10000 at accepted tick=25","practice-direct-timed-relaunch=AIRBORNE / DIRECT TIMED RELAUNCH"},
    {Replay::ForceDirectTimed,TimedPickerInputs::DirectTimed,false,10,150,"DIRECT TIMED RELAUNCH","direct timed relaunch",
        "accepted native tick=150 G-Red-position=-3665.00/10.00/-3197.56","direct-timed-relaunches=1 air-dash-starts=0 floors=0 timer698=0.0"},
    {Replay::OwnedDirectTimedUp,TimedPickerInputs::DirectTimedUp,true,11,25,"DIRECT TIMED UP RELAUNCH","direct timed Up relaunch",
        "practice-ledge-timer-ticks=13","practice-direct-timed-relaunch=AIRBORNE / DIRECT TIMED UP RELAUNCH"},
    {Replay::ForceDirectTimedUp,TimedPickerInputs::DirectTimedUp,false,11,150,"DIRECT TIMED UP RELAUNCH","direct timed Up relaunch",
        "accepted native tick=150 G-Red-position=-3539.49/10.00/-3042.58","direct-timed-relaunches=1 air-dash-starts=0 floors=0 timer698=0.0"},
    {Replay::OwnedActiveSnap,TimedPickerInputs::ActiveSnap,true,12,33,"ACTIVE SNAP RELAUNCH","active snap relaunch",
        "practice-meter=9200/10000 at accepted tick=33","practice-active-snap-relaunch=AIRBORNE / ACTIVE DASH TO RELAUNCH"},
    {Replay::ForceActiveSnap,TimedPickerInputs::ActiveSnap,false,17,110,"ACTIVE SNAP RELAUNCH","active snap relaunch",
         "accepted native tick=110 G-Red-position=2490.76/10.00/-1941.56","active-floors=1",true},
    {Replay::OwnedExpiringDirect,TimedPickerInputs::ExpiringDirect,true,13,38,"RELAUNCH ON EXPIRY","direct relaunch on expiry",
         "practice-meter=9850/10000 at accepted tick=38","practice-direct-expiring-relaunch=AIRBORNE / RELAUNCH ON EXPIRY"},
    {Replay::ForceExpiringDirect,TimedPickerInputs::ExpiringDirect,false,16,125,"RELAUNCH ON EXPIRY","direct relaunch on expiry",
         "accepted native tick=125 G-Red-position=-755.24/10.00/-3150.42",
         "direct-expiring-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=0 timer698=0.0",true},
    {Replay::OwnedExpiringEntryPush,TimedPickerInputs::ExpiringEntryPush,true,15,56,"ENTRY PUSH ON EXPIRY","relaunch entry push on expiry",
         "practice-meter=9850/10000 at accepted tick=56","practice-direct-expiring-entry-push=AIRBORNE / RELAUNCH ENTRY PUSH ON EXPIRY",true},
    {Replay::ForceExpiringEntryPush,TimedPickerInputs::ExpiringEntryPush,false,14,145,"ENTRY PUSH ON EXPIRY","relaunch entry push on expiry",
         "accepted native tick=145 G-Red-position=-419.40/10.00/-3301.88",
         "direct-expiring-push-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=1 relaunch-fall-pushes=0 timer698=0.0"},
    {Replay::OwnedAdjacentEntryPush,TimedPickerInputs::AdjacentEntryPush,true,15,56,"ADJACENT ENTRY PUSH","adjacent entry push on expiry",
        "accepted native tick=56 G-Red-position=-771.56/1404.49/-3368.72","practice-direct-expiring-entry-push=AIRBORNE / RELAUNCH ENTRY PUSH ON EXPIRY"},
    {Replay::ForceAdjacentEntryPush,TimedPickerInputs::AdjacentEntryPush,false,14,145,"ADJACENT ENTRY PUSH","adjacent entry push on expiry",
        "accepted native tick=145 G-Red-position=-419.40/10.00/-3302.19",
        "direct-expiring-push-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=1 relaunch-fall-pushes=0 timer698=0.0",true},
    {Replay::OwnedExpiringAirDash,TimedPickerInputs::ExpiringAirDash,true,13,38,"AIR DASH ON EXPIRY","air dash on expiry",
        "practice-meter=10000/10000 at accepted tick=38","practice-expiring-walk-air-dash=AIRBORNE / DASH WIND-UP ON EXPIRY",true},
    {Replay::ForceExpiringAirDash,TimedPickerInputs::ExpiringAirDash,false,16,175,"AIR DASH ON EXPIRY","air dash on expiry",
        "accepted native tick=175 G-Red-position=1296.58/10.00/-3473.49","windup-pushes=2 active-frames=31 relaunches=0 timer698=0.0"},
    {Replay::OwnedExpiringDashRelaunch,TimedPickerInputs::ExpiringDashRelaunch,true,17,44,"EXPIRY WIND-UP RELAUNCH","expiry-step wind-up relaunch",
        "practice-meter=9550/10000 at accepted tick=44","practice-windup-handoff=SHIFT/B RELAUNCH / NEUTRAL DASH"},
    {Replay::ForceExpiringDashRelaunch,TimedPickerInputs::ExpiringDashRelaunch,false,12,145,"EXPIRY WIND-UP RELAUNCH","expiry-step wind-up relaunch",
        "accepted native tick=145 G-Red-position=-736.86/10.00/-3446.59","windup-pushes=2 active-frames=0 relaunches=1 timer698=0.0",true},
    {Replay::OwnedTimedReleasePush,TimedPickerInputs::ReleasePush,true,18,33,"TIMED RELEASE PUSH","timed relaunch-fall push",
         "practice-meter=9850/10000 at accepted tick=33","practice-timed-release-push=AIRBORNE / TIMED RELAUNCH-FALL PUSH"},
    {Replay::ForceTimedReleasePush,TimedPickerInputs::ReleasePush,false,11,125,"TIMED RELEASE PUSH","timed relaunch-fall push",
         "accepted native tick=125 G-Red-position=752.13/10.00/-3156.15","relaunch-fall-pushes=1 relaunches=1 timer698=0.0 landings=1",true},
    {Replay::OwnedTimedReleaseTwoPushes,TimedPickerInputs::ReleaseTwoPushes,true,10,34,"TWO TIMED RELEASE PUSHES","two timed relaunch-fall pushes",
         "practice-meter=9850/10000 at accepted tick=34","practice-timed-release-push=AIRBORNE / SECOND TIMED RELAUNCH-FALL PUSH",true},
    {Replay::ForceTimedReleaseTwoPushes,TimedPickerInputs::ReleaseTwoPushes,false,19,125,"TWO TIMED RELEASE PUSHES","two timed relaunch-fall pushes",
         "accepted native tick=125 G-Red-position=-1279.39/10.00/-3166.36","relaunch-fall-pushes=2 relaunches=1 timer698=0.0 landings=1"},
};
constexpr gf::u32 expiring_entry_inputs(unsigned tick,bool push){
    gf::u32 buttons=0;
    if(tick>=6&&tick<=(push?25u:7u))buttons|=1u<<7;
    if(push?(tick>=56&&tick<=59):tick==38)buttons|=(1u<<5)|(1u<<1);
    return buttons;
}
void expiring_air_dash_inputs(gf::RawInputState& pad,unsigned tick,bool relaunch){
    if(tick==6||tick==7)pad.buttons|=1u<<7;
    if(tick==38)pad.axis[0]=-20000;
    if(tick==45&&relaunch)pad.buttons|=1u<<1;
}
constexpr gf::u32 timed_release_inputs(unsigned tick,bool two){
    gf::u32 buttons=0;
    if(tick>=6&&tick<=(two?7u:8u))buttons|=(1u<<8)|(two?1u<<6:0u);
    if(tick==(two?20u:23u))buttons|=1u<<1;
    return buttons;
}

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
        case Replay::RecoverySeamRestart:
            snapshot_.frame_step=f==0||f==2||f==4||f==7||f==8;
            if(f>=4&&f<=6)raw.buttons|=1u<<3; // Restart wins over a simultaneous step.
            break;
        case Replay::InclineDashJump:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick==6)raw.axis[1]=20000; // S on the desktop keyboard adapter.
            if(tick>=14&&tick<=37)raw.buttons|=1u<<1;
            break;
        }
        case Replay::TimedInclineLandingDash:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=9)raw.buttons|=1u<<7;
            if(tick==34)raw.axis[0]=-20000; // A on the desktop keyboard adapter.
            break;
        }
        case Replay::DirectExpiryRelaunch:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=25)raw.buttons|=1u<<7;
            if(tick>=57&&tick<=76)raw.buttons|=(1u<<5)|(1u<<1);
            break;
        }
        case Replay::TimedSnapDash:
        case Replay::FloorWindupRelaunch:
        case Replay::FloorWindupDash:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=122)raw.buttons|=(1u<<6)|(1u<<8);
            if(tick>=148&&tick<=150)raw.buttons|=1u<<1;
            if(replay_==Replay::FloorWindupRelaunch&&tick==156)raw.buttons|=1u<<1;
            if(tick==150)raw.axis[0]=raw.axis[1]=20000;
            break;
        }
        case Replay::PositiveFloorHandoff:
        case Replay::ActiveSnapRelaunch:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=8)raw.buttons|=1u<<5;
            if(tick==25||tick==26||tick==(replay_==Replay::ActiveSnapRelaunch?33u:32u))raw.buttons|=1u<<1;
            if(tick==26)raw.axis[1]=-20000; // W.
            break;
        }
        case Replay::ExpiringFloorHandoff:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=25)raw.buttons|=1u<<5;
            if(tick==46||tick==47||tick==55)raw.buttons|=1u<<1;
            if(tick==47)raw.axis[0]=-20000; // A.
            break;
        }
        case Replay::DirectTimedRelaunch:
        case Replay::DirectTimedUpRelaunch:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick>=6&&tick<=7)raw.buttons|=(1u<<5)|(1u<<7);
            if(tick>=25&&tick<=32){
                raw.buttons|=1u<<1;
                if(replay_==Replay::DirectTimedUpRelaunch)raw.buttons|=1u<<5;
            }
            break;
        }
        case Replay::ExpiringDirectRelaunch:
        case Replay::ExpiringDirectRelaunchPush:
        case Replay::ExpiringAdjacentRelaunchPush:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            raw.buttons|=expiring_entry_inputs(tick,replay_!=Replay::ExpiringDirectRelaunch);
            break;
        }
        case Replay::ExpiringAirDash:
        case Replay::ExpiringAirDashRelaunch:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            expiring_air_dash_inputs(raw,tick,replay_==Replay::ExpiringAirDashRelaunch);
            break;
        }
        case Replay::ExpiringShortAirDashRelaunch:{
            const unsigned tick=1+f/2;
            snapshot_.frame_step=f%2==0;
            if(tick==6||tick==7)raw.buttons|=1u<<7;
            if(tick==38)raw.axis[0]=raw.axis[1]=-20000;
            if(tick==44)raw.buttons|=1u<<1;
            if(tick>=45&&tick<=50)raw.buttons|=1u<<6;
            break;
        }
        case Replay::TimedReleasePush:
        case Replay::TimedReleaseTwoPushes:{
            const unsigned tick=1+f/2;
            const bool two=replay_==Replay::TimedReleaseTwoPushes;
            snapshot_.frame_step=f%2==0;
            raw.buttons|=timed_release_inputs(tick,two);
            break;
        }
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
            if(f==21)raw.buttons|=1u<<9; // Select the expiry foothold without moving the Force cursor.
            break;
        case Replay::ForceExpiryAirDash:
            if(f==0||f==2)raw.buttons|=1u<<9; // Standard -> expiry push -> expiry air dash.
            if(f>=4&&f<=6)raw.buttons|=1u<<3;
            if(f>=10&&f<30)raw.buttons|=1u<<7;
            if(f==61)raw.axis[0]=-32768; // The live step immediately after accepted tick 56.
            if(f>=306&&f<=308)raw.quit=true;
            break;
        case Replay::OwnedRecoverySeam:
        case Replay::ForceRecoverySeam:
            if(f==0)raw.buttons|=1u<<(replay_==Replay::OwnedRecoverySeam?8:10);
            if(f>=2&&f<=4)raw.buttons|=1u<<3;
            if(f>=7&&f<407){
                const unsigned tick=1+(f-7)/2;
                snapshot_.frame_step=(f-7)%2==0;
                if(tick>=6&&tick<=39)raw.buttons|=1u<<1;
                if(tick==43)raw.axis[1]=-32768;
                if(tick>=105&&tick<=109)raw.buttons|=1u<<8;
            }
            if(f>=408&&f<=410)raw.quit=true;
            if(f==413)raw.buttons|=1u<<(replay_==Replay::OwnedRecoverySeam?6:5); // Disabled copy or overlapping slot.
            if(f==415)raw.buttons|=1u<<3;
            if(f==418)raw.buttons|=1u<<(replay_==Replay::OwnedRecoverySeam?5:6); // Back to the original selection.
            break;
        case Replay::OwnedTimedHandoff:
        case Replay::ForceExpiryHandoff:{
            const bool expiry=replay_==Replay::ForceExpiryHandoff;
            const unsigned routes=expiry?3:4,ticks=expiry?200:180;
            if(f<2*routes&&f%2==0)raw.buttons|=1u<<(expiry?9:7); // Previous route from Standard.
            if(f>=2*routes&&f<=2*routes+2)raw.buttons|=1u<<3;
            if(f>=13&&f<13+2*ticks){
                const unsigned tick=1+(f-13)/2;
                snapshot_.frame_step=(f-13)%2==0;
                if(tick>=6&&tick<=(expiry?25:8))raw.buttons|=1u<<(expiry?7:8);
                if(tick==(expiry?57:30))raw.axis[expiry?0:1]=-32768;
                if(tick==(expiry?63:36))raw.buttons|=1u<<1;
            }
            if(f>=14+2*ticks&&f<=16+2*ticks)raw.quit=true;
            break;
        }
        case Replay::OwnedTimedFloor:
        case Replay::ForceTimedFloor:
        case Replay::OwnedInclineJump:
        case Replay::ForceInclineJump:{
            const bool owned=replay_==Replay::OwnedTimedFloor||replay_==Replay::OwnedInclineJump;
            const bool floor=replay_==Replay::OwnedTimedFloor||replay_==Replay::ForceTimedFloor;
            const unsigned routes=floor?2:3,ticks=floor?(owned?200:152):(owned?14:120);
            if(f<2*routes&&f%2==0)raw.buttons|=1u<<(owned?8:10); // Next route, not a different copy or slot.
            if(f>=2*routes&&f<=2*routes+2)raw.buttons|=1u<<3;
            if(f>=13&&f<13+2*ticks){
                const unsigned tick=1+(f-13)/2;
                snapshot_.frame_step=(f-13)%2==0;
                if(floor){
                    if(tick>=6&&tick<=122)raw.buttons|=(1u<<6)|(1u<<8);
                    if(tick>=147&&tick<=(owned?157:151))raw.buttons|=1u<<1;
                }else{
                    if(tick==6)raw.axis[1]=20000;
                    if(tick>=14&&tick<=37)raw.buttons|=1u<<1;
                }
            }
            if(f>=14+2*ticks&&f<=16+2*ticks)raw.quit=true;
            break;
        }
        case Replay::OwnedLandingDash:
        case Replay::ForceLandingDash:
        case Replay::OwnedDirectExpiry:
        case Replay::ForceDirectExpiry:{
            const bool owned=replay_==Replay::OwnedLandingDash||replay_==Replay::OwnedDirectExpiry;
            const bool expiry=replay_==Replay::OwnedDirectExpiry||replay_==Replay::ForceDirectExpiry;
            const unsigned routes=expiry?5:4,ticks=expiry?(owned?56:170):(owned?34:70);
            if(f<2*routes&&f%2==0)raw.buttons|=1u<<(owned?8:10);
            if(f>=2*routes&&f<=2*routes+2)raw.buttons|=1u<<3;
            if(f>=13&&f<13+2*ticks){
                const unsigned tick=1+(f-13)/2;
                snapshot_.frame_step=(f-13)%2==0;
                if(tick>=6&&tick<=(expiry?25u:9u))raw.buttons|=1u<<7;
                if(expiry){
                    if(tick>=57&&tick<=76)raw.buttons|=(1u<<5)|(1u<<1);
                }else if(tick==34)raw.axis[0]=-20000;
            }
            if(f>=14+2*ticks&&f<=16+2*ticks)raw.quit=true;
            break;
        }
        case Replay::OwnedPositiveFloor:
        case Replay::ForcePositiveFloor:
        case Replay::OwnedExpiredFloor:
        case Replay::ForceExpiredFloor:
        case Replay::OwnedSnapDash:
        case Replay::ForceSnapDash:
        case Replay::OwnedExpiryStep:
        case Replay::ForceExpiryStep:
        case Replay::OwnedDirectTimed:
        case Replay::ForceDirectTimed:
        case Replay::OwnedDirectTimedUp:
        case Replay::ForceDirectTimedUp:
        case Replay::OwnedActiveSnap:
        case Replay::ForceActiveSnap:
        case Replay::OwnedExpiringDirect:
        case Replay::ForceExpiringDirect:
        case Replay::OwnedExpiringEntryPush:
        case Replay::ForceExpiringEntryPush:
        case Replay::OwnedAdjacentEntryPush:
        case Replay::ForceAdjacentEntryPush:
        case Replay::OwnedExpiringAirDash:
        case Replay::ForceExpiringAirDash:
        case Replay::OwnedExpiringDashRelaunch:
        case Replay::ForceExpiringDashRelaunch:
        case Replay::OwnedTimedReleasePush:
        case Replay::ForceTimedReleasePush:
        case Replay::OwnedTimedReleaseTwoPushes:
        case Replay::ForceTimedReleaseTwoPushes:{
            const auto* spec=std::find_if(std::begin(timed_picker_replays),std::end(timed_picker_replays),
                [&](const auto& route){return route.sequence==replay_;});
            if(spec==std::end(timed_picker_replays))break;
            const unsigned select=2*spec->advances;
            if(f<select&&f%2==0)raw.buttons|=1u<<(spec->owned?(spec->previous?7:8):(spec->previous?9:10));
            if(f>=select&&f<=select+2)raw.buttons|=1u<<3;
            if(f>=spec->first_step()&&f<spec->after_steps()){
                const unsigned at=f-spec->first_step(),tick=1+at/2;
                snapshot_.frame_step=at%2==0;
                switch(spec->inputs){
                case TimedPickerInputs::PositiveFloor:
                case TimedPickerInputs::ActiveSnap:{
                    const bool active=spec->inputs==TimedPickerInputs::ActiveSnap;
                    if(tick>=6&&tick<=8)raw.buttons|=1u<<5;
                    if(tick==25||tick==26||(active&&tick==33)||(!active&&spec->owned&&tick==32))raw.buttons|=1u<<1;
                    if(active&&!spec->owned&&tick==33)raw.buttons|=1u<<5;
                    if(tick==26)raw.axis[1]=-20000;
                    break;
                }
                case TimedPickerInputs::ExpiredFloor:
                case TimedPickerInputs::SnapDash:
                    if(tick>=6&&tick<=122)raw.buttons|=(1u<<6)|(1u<<8);
                    if((tick>=148&&tick<=150)||(replay_==Replay::ForceExpiredFloor&&tick==156))raw.buttons|=1u<<1;
                    if(tick==150)raw.axis[0]=raw.axis[1]=20000;
                    break;
                case TimedPickerInputs::ExpiryStep:
                    if(tick>=6&&tick<=25)raw.buttons|=1u<<5;
                    if(tick==46||tick==47||(spec->owned&&tick==55))raw.buttons|=1u<<1;
                    if(tick==47)raw.axis[0]=-20000;
                    break;
                case TimedPickerInputs::DirectTimed:
                case TimedPickerInputs::DirectTimedUp:
                    if(tick>=6&&tick<=7)raw.buttons|=(1u<<5)|(1u<<7);
                    if(tick>=25&&tick<=32){
                        raw.buttons|=1u<<1;
                        if(spec->inputs==TimedPickerInputs::DirectTimedUp)raw.buttons|=1u<<5;
                    }
                    break;
                case TimedPickerInputs::ReleasePush:
                case TimedPickerInputs::ReleaseTwoPushes:
                    raw.buttons|=timed_release_inputs(tick,spec->inputs==TimedPickerInputs::ReleaseTwoPushes);
                    break;
                case TimedPickerInputs::ExpiringDirect:
                case TimedPickerInputs::ExpiringEntryPush:
                case TimedPickerInputs::AdjacentEntryPush:
                    raw.buttons|=expiring_entry_inputs(tick,spec->inputs!=TimedPickerInputs::ExpiringDirect);
                    break;
                case TimedPickerInputs::ExpiringAirDash:
                case TimedPickerInputs::ExpiringDashRelaunch:
                    expiring_air_dash_inputs(raw,tick,spec->inputs==TimedPickerInputs::ExpiringDashRelaunch);
                    break;
                }
            }
            if(f>spec->after_steps()&&f<=spec->after_steps()+3)raw.quit=true;
            break;
        }
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
    const bool seam=replay==Replay::RecoverySeamRestart;
    const bool incline=replay==Replay::InclineDashJump;
    const bool timed_incline=replay==Replay::TimedInclineLandingDash;
    const bool direct_expiry=replay==Replay::DirectExpiryRelaunch;
    const bool direct_expiring=replay==Replay::ExpiringDirectRelaunch;
    const bool direct_expiring_push=replay==Replay::ExpiringDirectRelaunchPush;
    const bool adjacent_expiring_push=replay==Replay::ExpiringAdjacentRelaunchPush;
    const bool expiring_air_dash=replay==Replay::ExpiringAirDash;
    const bool expiring_air_relaunch=replay==Replay::ExpiringAirDashRelaunch;
    const bool expiring_short_relaunch=replay==Replay::ExpiringShortAirDashRelaunch;
    const bool snap_handoff=replay==Replay::FloorWindupRelaunch||replay==Replay::FloorWindupDash;
    const bool positive_handoff=replay==Replay::PositiveFloorHandoff;
    const bool expiring_handoff=replay==Replay::ExpiringFloorHandoff;
    const bool direct_timed=replay==Replay::DirectTimedRelaunch;
    const bool direct_timed_up=replay==Replay::DirectTimedUpRelaunch;
    const bool active_snap_relaunch=replay==Replay::ActiveSnapRelaunch;
    const bool timed_release_push=replay==Replay::TimedReleasePush;
    const bool timed_release_two=replay==Replay::TimedReleaseTwoPushes;
    const bool snap_dash=replay==Replay::TimedSnapDash;
    const bool fixture=replay==Replay::FixtureWalk||replay==Replay::FixtureRestart||seam||incline||timed_incline||
        direct_expiry||direct_expiring||direct_expiring_push||adjacent_expiring_push||snap_dash||snap_handoff||positive_handoff||expiring_handoff||
        direct_timed||direct_timed_up||active_snap_relaunch||expiring_air_dash||expiring_air_relaunch||expiring_short_relaunch||
        timed_release_push||timed_release_two;
    char program[]="gf_native",practice[]="--gred-movement",expiry[]="--gred-walk-ledge-expiry-push",
         recovery[]="--gred-recovery-first-contact-seam",incline_jump[]="--gred-inclined-dash-jump",
           incline_landing[]="--gred-timed-incline-landing-dash",expiry_direct[]="--gred-walk-ledge-expiry-direct-relaunch",
           expiring_direct[]="--gred-walk-ledge-expiring-direct-relaunch",expiring_push[]="--gred-walk-ledge-expiring-relaunch-push",
           expiring_adjacent_push[]="--gred-walk-ledge-expiring-adjacent-relaunch-push",
           expiring_dash[]="--gred-walk-ledge-expiring-air-dash",
           expiring_dash_relaunch[]="--gred-walk-ledge-expiring-air-dash-handoff",
           expiring_short_dash_relaunch[]="--gred-walk-ledge-expiring-diagonal-dash-handoff",
          timed_snap_dash[]="--gred-walk-ledge-timed-snap-dash",floor_handoff[]="--gred-walk-ledge-timed-snap-handoff",
          positive_floor_handoff[]="--gred-walk-ledge-positive-snap-handoff",
          expiring_floor_handoff[]="--gred-walk-ledge-expiring-snap-handoff",
          direct_timed_relaunch[]="--gred-walk-ledge-direct-timed-relaunch",
          direct_timed_up_relaunch[]="--gred-walk-ledge-direct-timed-up-relaunch",
           after_active_snap[]="--gred-walk-ledge-active-snap-relaunch",release_push[]="--gred-walk-ledge-timed-release-push",
           release_two[]="--gred-walk-ledge-timed-release-two-pushes";
    char audio[]="--no-audio",start[]="--practice-start";
     char* args[]={program,expiring_short_relaunch?expiring_short_dash_relaunch:expiring_air_relaunch?expiring_dash_relaunch:expiring_air_dash?expiring_dash:
         timed_release_two?release_two:timed_release_push?release_push:active_snap_relaunch?after_active_snap:
        direct_timed_up?direct_timed_up_relaunch:direct_timed?direct_timed_relaunch:
        expiring_handoff?expiring_floor_handoff:positive_handoff?positive_floor_handoff:
        snap_handoff?floor_handoff:snap_dash?timed_snap_dash:
         direct_expiry?expiry_direct:direct_expiring?expiring_direct:adjacent_expiring_push?expiring_adjacent_push:
         direct_expiring_push?expiring_push:timed_incline?incline_landing:
        incline?incline_jump:seam?recovery:fixture?expiry:practice,audio,start};
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
bool recovery_seam_completed(const std::string& log){
    return once(log,"recovery seam guide: paused at tick=0")&&once(log,"practice stepped to tick=200")&&
        !has(log,"recovery first-contact seam demo:")&&!has(log,"motion acceptance refused")&&
        !has(log,"practice restarted")&&has(log,"accepted native tick=200 G-Red-position=1134.61/10.00/-611.42")&&
        has(log,"recovery-recontact landings=1 resource=10000 selector=0");
}
bool picker_guide_played(const std::string& log,const std::string& guide,unsigned ticks){
    return once(log,(guide+" guide: paused at tick=0").c_str())&&
        once(log,("practice stepped to tick="+std::to_string(ticks)+"\n").c_str())&&
        !has(log,(guide+" demo:").c_str())&&!has(log,"practice stopped")&&!has(log,"practice restarted");
}
bool timed_picker_played(const std::string& log,const TimedPickerReplay& spec){
    const auto launch=std::string(spec.owned?"launched practice for owned instance=8 route=":
        "launched practice for Force 1 slot 2 instance=8 route=")+spec.route;
    const auto returned=std::string(spec.owned?"returned to owned instance=8 after practice route=":
        "returned to Force 1 slot 2 after practice route=")+spec.route;
    const bool two=spec.inputs==TimedPickerInputs::ReleaseTwoPushes;
    const bool release=two||spec.inputs==TimedPickerInputs::ReleasePush;
    const bool ending=spec.owned?has(log,two?"practice-ledge-timer-ticks=4":"practice-ledge-timer-ticks=6"):
        has(log,"practice-meter=10000/10000 at accepted tick=125")&&has(log,"practice-phase=IDLE")&&
        !has(log,"practice-timed-release-push=");
    const bool entry_push=spec.inputs==TimedPickerInputs::ExpiringEntryPush||spec.inputs==TimedPickerInputs::AdjacentEntryPush;
    const bool expiring=entry_push||spec.inputs==TimedPickerInputs::ExpiringDirect;
    const bool expiry_entry=has(log,"practice-direct-expiring-relaunch=");
    const bool expiry_push=has(log,"practice-direct-expiring-entry-push=");
    const bool expiry_ending=spec.owned?expiry_entry!=entry_push&&expiry_push==entry_push:
        !expiry_entry&&!expiry_push&&has(log,"practice-phase=IDLE")&&
        has(log,entry_push?"practice-meter=10000/10000 at accepted tick=145":"practice-meter=10000/10000 at accepted tick=125");
    const bool dash_relaunch=spec.inputs==TimedPickerInputs::ExpiringDashRelaunch;
    const bool expiry_dash=dash_relaunch||spec.inputs==TimedPickerInputs::ExpiringAirDash;
    const bool dash_ending=spec.owned?has(log,"practice-phase=DASH WIND-UP\n")&&
        has(log,"practice-windup-handoff=")==dash_relaunch&&
        has(log,"practice-expiring-walk-air-dash=")!=dash_relaunch:
        !has(log,"practice-windup-handoff=")&&!has(log,"practice-expiring-walk-air-dash-relaunch=")&&
        has(log,"practice-phase=IDLE")&&
        has(log,dash_relaunch?"practice-meter=10000/10000 at accepted tick=145":"practice-meter=10000/10000 at accepted tick=175");
    return once(log,launch.c_str())&&once(log,returned.c_str())&&picker_guide_played(log,spec.guide,spec.ticks)&&
        has(log,spec.expected)&&has(log,spec.receipt)&&(!release||ending)&&(!expiring||expiry_ending)&&(!expiry_dash||dash_ending)&&
        (has(log,"practice-floor-windup-handoff=")==(spec.sequence==Replay::OwnedExpiredFloor));
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
    char enabled[]="8",short_limit[]="32",long_limit[]="205",seam_limit[]="426",handoff_limit[]="390";
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
    log.text.clear();input=ReplayInput(Replay::OwnedRecoverySeam);
    check(run_picker(enabled,seam_limit)==426&&recovery_seam_completed(log.text)&&
        once(log.text,"launched practice for owned instance=8 route=RECOVERY SEAM")&&
        once(log.text,"returned to owned instance=8 after practice route=RECOVERY SEAM")&&
        once(log.text,"practice unavailable for instance=9: the requested owned copy is disabled"),
        "owned recovery seam guide runs on live step input and returns to the exact copy and route",log.text);
    log.text.clear();input=ReplayInput(Replay::OwnedTimedHandoff);
    check(run_picker(enabled,handoff_limit)==390&&once(log.text,"launched practice for owned instance=8 route=TIMED HANDOFF")&&
        once(log.text,"timed wind-up relaunch guide: paused at tick=0")&&once(log.text,"practice stepped to tick=180")&&
        !has(log.text,"timed wind-up relaunch demo:")&&!has(log.text,"motion acceptance refused")&&
        has(log.text,"accepted native tick=180 G-Red-position=791.15/10.00/-3357.36")&&
        once(log.text,"returned to owned instance=8 after practice route=TIMED HANDOFF"),
        "owned timed handoff runs from live inputs and returns to the same copy and route",log.text);
    log.text.clear();input=ReplayInput(Replay::OwnedTimedFloor);
    char floor_limit[]="426",incline_limit[]="54";
    check(run_picker(enabled,floor_limit)==426&&picker_guide_played(log.text,"timed relaunch floor",200)&&
        once(log.text,"launched practice for owned instance=8 route=TIMED RELAUNCH FLOOR")&&
        has(log.text,"accepted native tick=200 G-Red-position=-2656.36/10.00/537.58")&&
        has(log.text,"floors=6")&&has(log.text,"practice-meter=10000/10000 at accepted tick=200")&&
        once(log.text,"returned to owned instance=8 after practice route=TIMED RELAUNCH FLOOR"),
        "owned floor guide lifts off after six snaps, lands, and returns to the same exact copy and route",log.text);
    log.text.clear();input=ReplayInput(Replay::OwnedInclineJump);
    check(run_picker(enabled,incline_limit)==54&&picker_guide_played(log.text,"incline dash jump",14)&&
        once(log.text,"launched practice for owned instance=8 route=INCLINE DASH JUMP")&&
        has(log.text,"accepted native tick=14 G-Red-position=812.74/481.75/2374.44")&&
        has(log.text,"practice-meter=10000/10000 at accepted tick=14")&&
        once(log.text,"practice-dash-takeoff=DASH CANCELED")&&
        once(log.text,"returned to owned instance=8 after practice route=INCLINE DASH JUMP"),
        "owned incline guide publishes the accepted cancel and returns to the same exact copy and route",log.text);
    log.text.clear();input=ReplayInput(Replay::OwnedLandingDash);
    char landing_limit[]="96",direct_limit[]="140";
    check(run_picker(enabled,landing_limit)==96&&picker_guide_played(log.text,"timed incline landing dash",34)&&
        once(log.text,"launched practice for owned instance=8 route=TIMED LANDING DASH")&&
        once(log.text,"practice-landing-dash=DASH FROM LANDING")&&has(log.text,"practice-ledge-timer-ticks=5")&&
        once(log.text,"returned to owned instance=8 after practice route=TIMED LANDING DASH"),
        "owned timed landing dash enters on the incline and returns to the same copy and route",log.text);
    log.text.clear();input=ReplayInput(Replay::OwnedDirectExpiry);
    check(run_picker(enabled,direct_limit)==140&&picker_guide_played(log.text,"direct expiry relaunch",56)&&
        once(log.text,"launched practice for owned instance=8 route=DIRECT EXPIRY RELAUNCH")&&
        once(log.text,"practice-direct-expiry-request=UP+SHIFT/B TO REQUEST RELAUNCH")&&
        has(log.text,"relaunches=0 air-dash-starts=0 floors=0 timer698=0.0")&&
        !has(log.text,"practice-direct-expiry-relaunch=")&&
        once(log.text,"returned to owned instance=8 after practice route=DIRECT EXPIRY RELAUNCH"),
        "owned expiry guide offers the next input without dispatching it and returns to the exact copy and route",log.text);
    for(const auto& route:timed_picker_replays)if(route.owned){
        log.text.clear();input=ReplayInput(route.sequence);
        auto count=std::to_string(route.budget());
        check(run_picker(enabled,count.data())==route.budget()&&timed_picker_played(log.text,route),
            "owned timed guide runs from live input and returns to the exact copy and route",log.text);
    }
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
    const auto packages=gf::pc::scan_borg_packages("borgs");std::string error;
    auto snapshot=gf::pc::BorgForceBrowseSnapshot::open(root,{7,11},"borgs",packages,std::move(retail),&error);
    check(bool(snapshot),"verified mixed fixture",error);
    auto run=[&](Replay sequence,char* count){
        input=ReplayInput(sequence);log.text.clear();Options options;
        char program[]="gf_native",mode[]="--browse-borg-force",account[]="7",save[]="11",force[]="1",slot[]="2";
        char select[]="--force-slot",practice[]="--practice-from-picker",catalogue[]="--retail-catalog",
             off[]="--headless",frames[]="--frames";
        char* args[]={program,mode,root.data(),account,save,force,select,slot,practice,catalogue,retail_path.data(),off,frames,count};
        check(parse(int(std::size(args)),args,&options),"Force browser options",log.text);
        gf::Context context(backends,60);gf::RenderEngine render(context.render());
        check(render.initialize({640,480,false,true,"Force picker replay"},nullptr,&error),"Force browser renderer",error);
        check(run_borg_force_browser(context,render,options,packages,*snapshot)==0&&context.frame_index()==options.frames,
            "Force browser has a single frame budget across handoffs",log.text);
    };
    char short_limit[]="44",long_limit[]="325",seam_limit[]="426",handoff_limit[]="430";
    run(Replay::ForcePicker,short_limit);
    check(occurrences(log.text,"launched practice for Force 1 slot 2 instance=8")==2&&
        occurrences(log.text,"returned to Force 1 slot 2 after practice")==2&&
        has(log.text,"launched practice for Force 1 slot 2 instance=8 route=STANDARD")&&
        has(log.text,"returned to Force 1 slot 2 after practice route=LEDGE TIMER EXPIRY")&&
        has(log.text,"G-Red-position=-1250.97/")&&
        occurrences(log.text,"Force 1 slot 2 supplies owned instance=8; one practice copy, not Force admission")==2&&
        once(log.text,"practice unavailable for Force 1 slot 1: the requested Force slot overlaps retail and package claims")&&
        !has(log.text,"practice restarted at tick=0")&&snapshot->current(),
        "exact Force slot launch and return, held input isolation, overlap refusal, and unchanged scope",log.text);
    run(Replay::ForceExpiryAirDash,long_limit);
    check(once(log.text,"launched practice for Force 1 slot 2 instance=8 route=EXPIRY AIR DASH")&&
        once(log.text,"practice start: demo placement, live input, no scripted replay")&&
        !has(log.text,"expiry air-dash demo:")&&!has(log.text,"motion acceptance refused")&&
        has(log.text,"G-Red-position=1383.11/10.00/-3360.84")&&
        has(log.text,"walk-ledge-fall-pushes=2 last-angle-s16=13653 landings=1 resource=10000 selector=0")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=EXPIRY AIR DASH")&&snapshot->current(),
        "live input follows the expiry air dash and returns to the same exact slot and route",log.text);
    run(Replay::ForceRecoverySeam,seam_limit);
    check(recovery_seam_completed(log.text)&&
        once(log.text,"launched practice for Force 1 slot 2 instance=8 route=RECOVERY SEAM")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=RECOVERY SEAM")&&
        once(log.text,"practice unavailable for Force 1 slot 1: the requested Force slot overlaps retail and package claims")&&
        snapshot->current(),
        "Force recovery seam guide runs on live step input and returns to the exact slot and route",log.text);
    run(Replay::ForceExpiryHandoff,handoff_limit);
    check(once(log.text,"launched practice for Force 1 slot 2 instance=8 route=EXPIRY HANDOFF")&&
        once(log.text,"expiry wind-up relaunch guide: paused at tick=0")&&once(log.text,"practice stepped to tick=200")&&
        !has(log.text,"expiry wind-up relaunch demo:")&&!has(log.text,"motion acceptance refused")&&
        has(log.text,"accepted native tick=200 G-Red-position=-588.61/10.00/-3366.51")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=EXPIRY HANDOFF")&&snapshot->current(),
        "Force expiry handoff runs from live inputs and returns to the exact slot and route",log.text);
    char floor_limit[]="330",incline_limit[]="267";
    run(Replay::ForceTimedFloor,floor_limit);
    check(picker_guide_played(log.text,"timed relaunch floor",152)&&
        once(log.text,"launched practice for Force 1 slot 2 instance=8 route=TIMED RELAUNCH FLOOR")&&
        has(log.text,"practice-phase=TOUCHDOWN")&&has(log.text,"practice-meter=9450/10000 at accepted tick=152")&&
        !has(log.text,"practice-relaunch-floor=")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=TIMED RELAUNCH FLOOR")&&snapshot->current(),
        "Force floor guide accepts early release instead of holding the snap, then returns to the exact slot and route",log.text);
    run(Replay::ForceInclineJump,incline_limit);
    check(picker_guide_played(log.text,"incline dash jump",120)&&
        once(log.text,"launched practice for Force 1 slot 2 instance=8 route=INCLINE DASH JUMP")&&
        has(log.text,"accepted native tick=120 G-Red-position=812.74/481.75/2374.44")&&
        has(log.text,"practice-meter=10000/10000 at accepted tick=120")&&has(log.text,"practice-phase=IDLE")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=INCLINE DASH JUMP")&&snapshot->current(),
        "Force incline guide completes live and returns to the same exact slot and route",log.text);
    char landing_limit[]="167",direct_limit[]="366";
    run(Replay::ForceLandingDash,landing_limit);
    check(picker_guide_played(log.text,"timed incline landing dash",70)&&
        once(log.text,"launched practice for Force 1 slot 2 instance=8 route=TIMED LANDING DASH")&&
        has(log.text,"accepted native tick=70 G-Red-position=-1408.01/27.15/-935.02")&&
        has(log.text,"landing-dashes=1 loss-landings=1 timer698=0.0 resource=10000")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=TIMED LANDING DASH")&&snapshot->current(),
        "Force timed landing dash finishes recovery and returns to the exact slot and route",log.text);
    run(Replay::ForceDirectExpiry,direct_limit);
    check(picker_guide_played(log.text,"direct expiry relaunch",170)&&
        once(log.text,"launched practice for Force 1 slot 2 instance=8 route=DIRECT EXPIRY RELAUNCH")&&
        has(log.text,"accepted native tick=170 G-Red-position=-440.81/10.00/-2989.32")&&
        has(log.text,"practice-meter=10000/10000 at accepted tick=170")&&has(log.text,"relaunches=1 air-dash-starts=0 floors=0 timer698=0.0")&&
        !has(log.text,"practice-direct-expiry-request=")&&!has(log.text,"practice-direct-expiry-relaunch=")&&
        once(log.text,"returned to Force 1 slot 2 after practice route=DIRECT EXPIRY RELAUNCH")&&snapshot->current(),
        "Force direct expiry guide runs live through landing and returns without leaking either cue",log.text);
    for(const auto& route:timed_picker_replays)if(!route.owned){
        auto count=std::to_string(route.budget());
        run(route.sequence,count.data());
        check(timed_picker_played(log.text,route)&&snapshot->current(),
            "Force timed guide runs from live input and returns to the exact slot and route",log.text);
    }
    std::puts("Force browser practice: exact slot, live routes, and overlap refusal PASS");
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
    using Route=gf::GRedPickerPracticeRoute;
    static_assert(unsigned(Route::Count)==29);
    for(bool previous:{false,true}){
        auto route=Route::Standard;
        std::array<bool,unsigned(Route::Count)> seen{};
        for(unsigned i=0;i<seen.size();++i){
            Options guide;
            check(!seen[unsigned(route)]&&!gf::gred_picker_practice_route_name(route).empty()&&
                picker_practice_options(route,&guide)&&guide.gred_practice&&guide.source_ground_interactive&&
                guide.gred_practice_start==(route!=Route::Standard),"one interactive guide per route before wrap","");
            if(route==Route::TimedReleasePush||route==Route::TimedReleaseTwoPushes)
                check(guide.timed_ledge_followup==(route==Route::TimedReleasePush?TimedLedgeFollowup::TimedReleasePush:
                    TimedLedgeFollowup::TimedReleaseTwoPushes),"release guides keep their distinct scopes","");
            if(route==Route::ExpiringDirectRelaunch||route==Route::ExpiringDirectRelaunchPush)
                check(guide.timed_ledge_followup==(route==Route::ExpiringDirectRelaunch?TimedLedgeFollowup::ExpiringDirectRelaunch:
                    TimedLedgeFollowup::ExpiringDirectRelaunchPush),"expiry-step guides keep their distinct scopes","");
            if(route==Route::ExpiringAdjacentRelaunchPush||route==Route::ExpiringAirDash||route==Route::ExpiringAirDashRelaunch)
                check(guide.timed_ledge_followup==(route==Route::ExpiringAdjacentRelaunchPush?TimedLedgeFollowup::ExpiringAdjacentRelaunchPush:
                    route==Route::ExpiringAirDash?TimedLedgeFollowup::ExpiringAirDash:TimedLedgeFollowup::ExpiringAirDashRelaunch),
                    "adjacent entry, dash, and paid handoff use their existing distinct modes","");
            seen[unsigned(route)]=true;
            route=gf::cycle_gred_picker_practice_route(route,previous);
        }
        check(route==Route::Standard,"all 29 choices wrap in both directions","");
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
    const auto guide=run(Replay::RecoverySeamRestart,10,"paused recovery seam restart");
    check(occurrences(guide,"recovery seam guide: paused at tick=0")==2&&
        occurrences(guide,"practice stepped to tick=1")==2&&once(guide,"practice stepped to tick=2")&&
        once(guide,"practice restarted at tick=0")&&has(guide,"frames=10 ticks=1")&&
        has(guide,"accepted native tick=1 G-Red-position=-240.00/10.00/300.00"),
        "guide starts and restarts paused without injecting movement; held step does not repeat",guide);
    const auto canceled=run(Replay::InclineDashJump,28,"incline dash-jump takeoff");
    check(once(canceled,"incline dash jump guide: paused at tick=0")&&!has(canceled,"incline dash jump demo:")&&
        has(canceled,"accepted native tick=14 G-Red-position=812.74/481.75/2374.44")&&
        has(canceled,"practice-meter=10000/10000 at accepted tick=14")&&once(canceled,"practice-dash-takeoff=DASH CANCELED"),
        "live S and Shift cancel dash recovery on the accepted slope without charging lift early",canceled);
    const auto jump_landed=run(Replay::InclineDashJump,240,"incline dash-jump landing");
    check(has(jump_landed,"accepted native tick=120 G-Red-position=812.74/481.75/2374.44")&&
        has(jump_landed,"practice-meter=10000/10000 at accepted tick=120")&&has(jump_landed,"practice-phase=IDLE")&&
        !has(jump_landed,"practice-dash-takeoff=")&&!has(jump_landed,"practice stopped"),
        "live guide lands and refills on its original slope without retaining the takeoff cue",jump_landed);
    const auto timed_dash=run(Replay::TimedInclineLandingDash,68,"timed incline landing dash entry");
    check(picker_guide_played(timed_dash,"timed incline landing dash",34)&&
        once(timed_dash,"practice-landing-dash=DASH FROM LANDING")&&has(timed_dash,"practice-ledge-timer-ticks=5")&&
        has(timed_dash,"practice-meter=10000/10000 at accepted tick=34"),
        "live advance must accept the distinct landing entry before the timed incline dash",timed_dash);
    const auto timed_dash_idle=run(Replay::TimedInclineLandingDash,140,"timed incline landing dash idle");
    check(picker_guide_played(timed_dash_idle,"timed incline landing dash",70)&&
        has(timed_dash_idle,"accepted native tick=70 G-Red-position=-1408.01/27.15/-935.02")&&
        has(timed_dash_idle,"landing-dashes=1 loss-landings=1 timer698=0.0 resource=10000")&&
        has(timed_dash_idle,"resets=1 root-updates=25")&&!has(timed_dash_idle,"practice-landing-dash="),
        "countdown expiry, dash recovery and idle are reached under live input without a stale cue",timed_dash_idle);
    const auto direct=run(Replay::DirectExpiryRelaunch,114,"direct expiry entry");
    check(picker_guide_played(direct,"direct expiry relaunch",57)&&
        once(direct,"practice-direct-expiry-relaunch=DIRECT EXPIRY RELAUNCH")&&
        has(direct,"practice-meter=9850/10000 at accepted tick=57")&&
        has(direct,"relaunches=1 air-dash-starts=0 floors=0 timer698=0.0"),
        "live Up+Shift enters immediately after the accepted walking expiry push",direct);
    const auto direct_idle=run(Replay::DirectExpiryRelaunch,340,"direct expiry settled");
    check(picker_guide_played(direct_idle,"direct expiry relaunch",170)&&
        has(direct_idle,"accepted native tick=170 G-Red-position=-440.81/10.00/-2989.32")&&
        has(direct_idle,"practice-meter=10000/10000 at accepted tick=170")&&
        has(direct_idle,"relaunches=1 air-dash-starts=0 floors=0 timer698=0.0")&&
        !has(direct_idle,"practice-direct-expiry-relaunch="),
        "live release and landing settle without dash charges, floor snaps or a stale entry cue",direct_idle);
    const auto snap=run(Replay::TimedSnapDash,300,"timed relaunch snap to air-dash entry");
    check(picker_guide_played(snap,"timed snap to air dash",150)&&has(snap,"practice-meter=9750/10000 at accepted tick=150")&&
        has(snap,"practice-ledge-timer-ticks=2 cue=LEDGE TIMER 2 TICKS")&&
        once(snap,"practice-relaunch-snap-windup=AIRBORNE / RELAUNCH TO WIND-UP"),
        "live held Shift and D+S take the existing stationary wind-up branch while the timer is positive",snap);
    const auto snap_idle=run(Replay::TimedSnapDash,440,"timed relaunch snap to air-dash idle");
    check(picker_guide_played(snap_idle,"timed snap to air dash",220)&&
        has(snap_idle,"accepted native tick=220 G-Red-position=-3094.35/10.00/1244.92")&&
        has(snap_idle,"practice-meter=10000/10000 at accepted tick=220")&&
        has(snap_idle,"snap-dash-starts=1 windup-entry-floors=1 windup-floors=6 active-floors=30")&&
        !has(snap_idle,"practice-relaunch-snap-windup="),
        "live finish reaches the later callback, separate landing and idle without a stale handoff cue",snap_idle);
    const auto floor_windup=run(Replay::FloorWindupRelaunch,310,"accepted paid floor wind-up choice");
    check(picker_guide_played(floor_windup,"floor wind-up relaunch",155)&&
        once(floor_windup,"practice-floor-windup-handoff=SHIFT/B RELAUNCH / NEUTRAL DASH")&&
        has(floor_windup,"practice-meter=9300/10000 at accepted tick=155"),
        "only accepted end of uninterrupted floor wind-up advertises the choice",floor_windup);
    const auto floor_relaunch=run(Replay::FloorWindupRelaunch,440,"floor wind-up to relaunch with live input");
    check(picker_guide_played(floor_relaunch,"floor wind-up relaunch",220)&&
        has(floor_relaunch,"accepted native tick=220 G-Red-position=-2668.02/10.00/594.38")&&
        has(floor_relaunch,"controller-loss-relaunches=2 timer698=0.0")&&has(floor_relaunch,"active-floors=0")&&
        !has(floor_relaunch,"practice-floor-windup-handoff="),
        "fresh neutral Shift replaces active dash, then the route settles without a stale choice",floor_relaunch);
    const auto floor_dash=run(Replay::FloorWindupDash,312,"ordinary active entry from the same floor wind-up");
    check(picker_guide_played(floor_dash,"floor wind-up relaunch",156)&&
        has(floor_dash,"practice-meter=9250/10000 at accepted tick=156")&&has(floor_dash,"practice-phase=AIR DASH / FLOOR SNAP")&&
        !has(floor_dash,"practice-floor-windup-handoff="),
        "neutral still starts the unchanged active dash and pays its own entry cost",floor_dash);
    const auto timed_floor=run(Replay::PositiveFloorHandoff,62,"paid floor wind-up with a positive timer");
    check(picker_guide_played(timed_floor,"positive floor wind-up relaunch",31)&&
        has(timed_floor,"practice-ledge-timer-ticks=7 cue=LEDGE TIMER 7 TICKS")&&
        once(timed_floor,"practice-floor-windup-handoff=SHIFT/B RELAUNCH / NEUTRAL DASH"),
        "accepted completion advertises the same choice with time still left",timed_floor);
    const auto timed_floor_idle=run(Replay::PositiveFloorHandoff,220,"positive floor handoff through live input to idle");
    check(picker_guide_played(timed_floor_idle,"positive floor wind-up relaunch",110)&&
        has(timed_floor_idle,"accepted native tick=110 G-Red-position=2628.75/10.00/-2029.20")&&
        has(timed_floor_idle,"practice-meter=10000/10000 at accepted tick=110")&&
        has(timed_floor_idle,"controller-loss-relaunches=2 timer698=0.0")&&has(timed_floor_idle,"active-floors=0")&&
        !has(timed_floor_idle,"practice-floor-windup-handoff="),
        "fresh Shift relaunches while the timer runs, then lands without an active dash or stale cue",timed_floor_idle);
    const auto last_tick=run(Replay::ExpiringFloorHandoff,108,"paid floor wind-up at the last tick");
    check(picker_guide_played(last_tick,"expiring floor wind-up relaunch",54)&&
        has(last_tick,"practice-ledge-timer-ticks=1 cue=LEDGE TIMER 1 TICKS")&&
        has(last_tick,"practice-meter=9400/10000 at accepted tick=54")&&
        once(last_tick,"practice-floor-windup-handoff=SHIFT/B RELAUNCH / NEUTRAL DASH"),
        "only the new guide offers the choice on an accepted final timer tick",last_tick);
    const auto last_tick_idle=run(Replay::ExpiringFloorHandoff,270,"expiry-step floor handoff settles");
    check(picker_guide_played(last_tick_idle,"expiring floor wind-up relaunch",135)&&
        has(last_tick_idle,"accepted native tick=135 G-Red-position=3503.92/10.00/-1597.90")&&
        has(last_tick_idle,"practice-meter=10000/10000 at accepted tick=135")&&
        has(last_tick_idle,"controller-loss-relaunches=2 timer698=0.0")&&has(last_tick_idle,"active-floors=0")&&
        !has(last_tick_idle,"practice-floor-windup-handoff="),
        "live Shift enters on the expiry step, skips active dash and finishes the route",last_tick_idle);
    const auto timed_direct=run(Replay::DirectTimedRelaunch,50,"live immediate timed relaunch");
    check(picker_guide_played(timed_direct,"direct timed relaunch",25)&&
        has(timed_direct,"practice-meter=9850/10000 at accepted tick=25")&&
        has(timed_direct,"practice-ledge-timer-ticks=13 cue=LEDGE TIMER 13 TICKS")&&
        once(timed_direct,"practice-direct-timed-relaunch=AIRBORNE / DIRECT TIMED RELAUNCH"),
        "live neutral Shift lifts straight out of the third walking push without any intermediate fall",timed_direct);
    const auto timed_direct_idle=run(Replay::DirectTimedRelaunch,300,"live timed direct entry through landing");
    check(picker_guide_played(timed_direct_idle,"direct timed relaunch",150)&&
        has(timed_direct_idle,"accepted native tick=150 G-Red-position=-3665.00/10.00/-3197.56")&&
        has(timed_direct_idle,"practice-meter=10000/10000 at accepted tick=150")&&
        has(timed_direct_idle,"direct-timed-relaunches=1 air-dash-starts=0 floors=0 timer698=0.0")&&
        !has(timed_direct_idle,"practice-direct-timed-relaunch="),
        "live release lands, and the cue does not survive entry",timed_direct_idle);
    const auto timed_up=run(Replay::DirectTimedUpRelaunch,50,"live soft Up timed entry");
    check(picker_guide_played(timed_up,"direct timed Up relaunch",25)&&
        has(timed_up,"practice-meter=9850/10000 at accepted tick=25")&&
        has(timed_up,"practice-ledge-timer-ticks=13 cue=LEDGE TIMER 13 TICKS")&&
        once(timed_up,"practice-direct-timed-relaunch=AIRBORNE / DIRECT TIMED UP RELAUNCH"),
        "live Up+Shift uses the directional entry at the same cost and timer",timed_up);
    const auto timed_up_idle=run(Replay::DirectTimedUpRelaunch,300,"live timed Up entry through landing");
    check(picker_guide_played(timed_up_idle,"direct timed Up relaunch",150)&&
        has(timed_up_idle,"accepted native tick=150 G-Red-position=-3539.49/10.00/-3042.58")&&
        has(timed_up_idle,"practice-meter=10000/10000 at accepted tick=150")&&
        has(timed_up_idle,"direct-timed-relaunches=1 air-dash-starts=0 floors=0 timer698=0.0")&&
        !has(timed_up_idle,"practice-direct-timed-relaunch="),
        "directional travel gives a distinct landing position without changing admission or the cue after entry",timed_up_idle);
    const auto post_active=run(Replay::ActiveSnapRelaunch,66,"live relaunch after active snap");
    check(picker_guide_played(post_active,"active snap relaunch",33)&&
        has(post_active,"practice-meter=9200/10000 at accepted tick=33")&&has(post_active,"practice-ledge-timer-ticks=5")&&
        once(post_active,"practice-active-snap-relaunch=AIRBORNE / ACTIVE DASH TO RELAUNCH")&&
        !has(post_active,"practice-floor-windup-handoff="),
        "wait for the paid active snap, then relaunch without refunding either charge",post_active);
    const auto post_active_idle=run(Replay::ActiveSnapRelaunch,220,"live post-active relaunch to idle");
    check(picker_guide_played(post_active_idle,"active snap relaunch",110)&&
        has(post_active_idle,"accepted native tick=110 G-Red-position=1970.01/10.00/-1610.82")&&
        has(post_active_idle,"practice-meter=10000/10000 at accepted tick=110")&&has(post_active_idle,"active-floors=1")&&
        !has(post_active_idle,"practice-active-snap-relaunch="),
        "post-active lift reaches normal idle without carrying its entry marker into landing",post_active_idle);
    const auto release_contact=run(Replay::TimedReleasePush,66,"live timed release contact");
    check(picker_guide_played(release_contact,"timed relaunch-fall push",33)&&
        has(release_contact,"practice-meter=9850/10000 at accepted tick=33")&&has(release_contact,"practice-ledge-timer-ticks=6")&&
        once(release_contact,"practice-timed-release-push=AIRBORNE / TIMED RELAUNCH-FALL PUSH")&&
        has(release_contact,"relaunch-fall-pushes=1 relaunches=1 timer698=6.0 landings=0"),
        "a live tap and release produce one timed inter-gate contact without grounding",release_contact);
    const auto release_ground=run(Replay::TimedReleasePush,250,"live timed release contact to idle");
    check(picker_guide_played(release_ground,"timed relaunch-fall push",125)&&
        has(release_ground,"accepted native tick=125 G-Red-position=752.13/10.00/-3156.15")&&
        has(release_ground,"practice-meter=10000/10000 at accepted tick=125")&&
        !has(release_ground,"practice-timed-release-push="),
        "neutral release contact leads to an ordinary landing and clears the one-frame cue",release_ground);
    const auto second_contact=run(Replay::TimedReleaseTwoPushes,68,"live second timed release contact");
    check(picker_guide_played(second_contact,"two timed relaunch-fall pushes",34)&&
        has(second_contact,"practice-meter=9850/10000 at accepted tick=34")&&has(second_contact,"practice-ledge-timer-ticks=4")&&
        once(second_contact,"practice-timed-release-push=AIRBORNE / SECOND TIMED RELAUNCH-FALL PUSH")&&
        has(second_contact,"relaunch-fall-pushes=2 relaunches=1 timer698=4.0 landings=0"),
        "two live steps earn consecutive timed contacts without grounding",second_contact);
    const auto two_ground=run(Replay::TimedReleaseTwoPushes,250,"live second timed release contact to idle");
    check(picker_guide_played(two_ground,"two timed relaunch-fall pushes",125)&&
        has(two_ground,"accepted native tick=125 G-Red-position=-1279.39/10.00/-3166.36")&&
        has(two_ground,"practice-meter=10000/10000 at accepted tick=125")&&!has(two_ground,"practice-timed-release-push="),
        "the same-face contact ends before landing and clears the cue",two_ground);
    const auto expiry_step=run(Replay::ExpiringDirectRelaunch,76,"live direct relaunch on expiry");
    check(picker_guide_played(expiry_step,"direct relaunch on expiry",38)&&
        has(expiry_step,"practice-meter=9850/10000 at accepted tick=38")&&
        once(expiry_step,"practice-direct-expiring-relaunch=AIRBORNE / RELAUNCH ON EXPIRY")&&
        !has(expiry_step,"practice-direct-expiring-request=")&&
        has(expiry_step,"direct-expiring-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=0 timer698=0.0"),
        "soft Up and Shift on a single live frame enter the plain 1 to 0 branch",expiry_step);
    const auto expiry_idle=run(Replay::ExpiringDirectRelaunch,250,"live direct relaunch on expiry to idle");
    check(picker_guide_played(expiry_idle,"direct relaunch on expiry",125)&&
        has(expiry_idle,"accepted native tick=125 G-Red-position=-755.24/10.00/-3150.42")&&
        has(expiry_idle,"practice-meter=10000/10000 at accepted tick=125")&&!has(expiry_idle,"practice-direct-expiring-relaunch="),
        "release returns to normal falling and landing without a stale cue",expiry_idle);
    const auto expiry_push=run(Replay::ExpiringDirectRelaunchPush,112,"live projected relaunch entry on expiry");
    check(picker_guide_played(expiry_push,"relaunch entry push on expiry",56)&&
        has(expiry_push,"practice-meter=9850/10000 at accepted tick=56")&&
        once(expiry_push,"practice-direct-expiring-entry-push=AIRBORNE / RELAUNCH ENTRY PUSH ON EXPIRY")&&
        !has(expiry_push,"practice-direct-expiring-relaunch=")&&
        has(expiry_push,"direct-expiring-push-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=1 relaunch-fall-pushes=0 timer698=0.0"),
        "the live 1 to 0 handoff has a separate projected-entry receipt and cue",expiry_push);
    const auto expiry_push_idle=run(Replay::ExpiringDirectRelaunchPush,290,"live projected expiry entry to idle");
    check(picker_guide_played(expiry_push_idle,"relaunch entry push on expiry",145)&&
        has(expiry_push_idle,"accepted native tick=145 G-Red-position=-419.40/10.00/-3301.88")&&
        has(expiry_push_idle,"practice-meter=10000/10000 at accepted tick=145")&&!has(expiry_push_idle,"practice-direct-expiring-entry-push="),
        "the short hold and release finish normally after the projected entry",expiry_push_idle);
    const auto adjacent_push=run(Replay::ExpiringAdjacentRelaunchPush,112,"live adjacent projected entry on expiry");
    check(picker_guide_played(adjacent_push,"adjacent entry push on expiry",56)&&
        has(adjacent_push,"accepted native tick=56 G-Red-position=-771.56/1404.49/-3368.72")&&
        has(adjacent_push,"practice-meter=9850/10000 at accepted tick=56")&&
        once(adjacent_push,"practice-direct-expiring-entry-push=AIRBORNE / RELAUNCH ENTRY PUSH ON EXPIRY")&&
        has(adjacent_push,"direct-expiring-push-relaunches=1 air-dash-starts=0 floors=0 entry-pushes=1 relaunch-fall-pushes=0 timer698=0.0"),
        "live input projects against the adjacent face instead of reusing the original route",adjacent_push);
    const auto adjacent_idle=run(Replay::ExpiringAdjacentRelaunchPush,290,"live adjacent expiry entry to idle");
    check(picker_guide_played(adjacent_idle,"adjacent entry push on expiry",145)&&
        has(adjacent_idle,"accepted native tick=145 G-Red-position=-419.40/10.00/-3302.19")&&
        has(adjacent_idle,"practice-meter=10000/10000 at accepted tick=145")&&!has(adjacent_idle,"practice-direct-expiring-entry-push="),
        "adjacent contact still finishes via normal fall and landing",adjacent_idle);
    const auto expiry_dash=run(Replay::ExpiringAirDash,76,"live air-dash wind-up on expiry");
    check(picker_guide_played(expiry_dash,"air dash on expiry",38)&&
        has(expiry_dash,"practice-meter=10000/10000 at accepted tick=38")&&
        once(expiry_dash,"practice-expiring-walk-air-dash=AIRBORNE / DASH WIND-UP ON EXPIRY")&&
        has(expiry_dash,"expiring-air-dash-starts=1 windup-pushes=0 active-frames=0 relaunches=0 timer698=0.0"),
        "one live A flick advances the clock but neither moves nor pays for active dash",expiry_dash);
    const auto expiry_dash_idle=run(Replay::ExpiringAirDash,350,"live air dash on expiry to idle");
    check(picker_guide_played(expiry_dash_idle,"air dash on expiry",175)&&
        has(expiry_dash_idle,"accepted native tick=175 G-Red-position=1296.58/10.00/-3473.49")&&
        has(expiry_dash_idle,"practice-meter=10000/10000 at accepted tick=175")&&
        has(expiry_dash_idle,"expiring-air-dash-starts=1 windup-pushes=2")&&!has(expiry_dash_idle,"practice-expiring-walk-air-dash="),
        "the unchanged wind-up pushes and separate dash and landing phases finish at idle",expiry_dash_idle);
    const auto expiry_relaunch=run(Replay::ExpiringAirDashRelaunch,90,"live relaunch after paid expiry-step wind-up");
    check(picker_guide_played(expiry_relaunch,"expiry-step wind-up relaunch",45)&&
        has(expiry_relaunch,"practice-meter=9400/10000 at accepted tick=45")&&
        once(expiry_relaunch,"practice-expiring-walk-air-dash-relaunch=AIRBORNE / EXPIRY WIND-UP TO RELAUNCH")&&
        has(expiry_relaunch,"expiring-air-dash-starts=1 windup-pushes=2 active-frames=0 relaunches=1 timer698=0.0")&&
        !has(expiry_relaunch,"practice-windup-handoff="),
        "live Shift keeps the 450 charge, spends 150 and skips active dash",expiry_relaunch);
    const auto expiry_relaunch_idle=run(Replay::ExpiringAirDashRelaunch,290,"live expiry-step wind-up relaunch to idle");
    check(picker_guide_played(expiry_relaunch_idle,"expiry-step wind-up relaunch",145)&&
        has(expiry_relaunch_idle,"accepted native tick=145 G-Red-position=-736.86/10.00/-3446.59")&&
        has(expiry_relaunch_idle,"practice-meter=10000/10000 at accepted tick=145")&&
        !has(expiry_relaunch_idle,"practice-expiring-walk-air-dash-relaunch="),"the existing fall and landing finish the live relaunch",expiry_relaunch_idle);
    const auto short_entry=run(Replay::ExpiringShortAirDashRelaunch,88,"live diagonal wind-up relaunch");
    check(picker_guide_played(short_entry,"diagonal expiry-step wind-up relaunch",44)&&
        has(short_entry,"practice-meter=9400/10000 at accepted tick=44")&&
        once(short_entry,"practice-expiring-walk-air-dash-relaunch=AIRBORNE / EXPIRY WIND-UP TO RELAUNCH")&&
        has(short_entry,"windup-pushes=2 active-frames=0 relaunches=1 timer698=0.0"),
        "live diagonal flick reaches the separate five-tick handoff",short_entry);
    const auto short_idle=run(Replay::ExpiringShortAirDashRelaunch,290,"live short expiry-step relaunch to idle");
    check(picker_guide_played(short_idle,"diagonal expiry-step wind-up relaunch",145)&&
        has(short_idle,"accepted native tick=145 G-Red-position=-768.57/10.00/-3396.78")&&
        has(short_idle,"practice-meter=10000/10000 at accepted tick=145")&&!has(short_idle,"practice-expiring-walk-air-dash-relaunch="),
        "release drift clears the face and reaches landing through live controls",short_idle);
    std::puts("G-Red practice replay: forty-nine desktop-loop scenarios PASS without opening devices");
}
