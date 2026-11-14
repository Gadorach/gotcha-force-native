// Owned SDL virtual-device/PCM tests. Never drives a physical controller.
#include <SDL.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <filesystem>
#include "engine/core/headless_backends.hpp"
#include "engine/gameplay/option_output.hpp"
#include "platform/pc/sdl_platform.hpp"
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"SDL output: %s (%s)\n",why,SDL_GetError());std::abort();}}
struct Motor {unsigned calls=0;Uint16 low=0,high=0;};
int virtual_success=0;
int rumble(void* data,Uint16 low,Uint16 high){auto& m=*static_cast<Motor*>(data);++m.calls;m.low=low;m.high=high;return virtual_success;}
struct Request{SDL_GameController* controller=nullptr;Uint16 low=0,high=0;Uint32 duration=0;int result=-1;};
Request last_request;unsigned request_count=0;
}
// Test-only link wrapper records the platform's exact SDK request and still
// delegates to the real SDL implementation. Virtual motor callbacks alone do
// not expose every accepted stop command on all SDL builds.
extern "C" int __real_SDL_GameControllerRumble(SDL_GameController*,Uint16,Uint16,Uint32);
extern "C" int __wrap_SDL_GameControllerRumble(SDL_GameController* c,Uint16 l,Uint16 h,Uint32 duration){
    const int result=__real_SDL_GameControllerRumble(c,l,h,duration);
    last_request={c,l,h,duration,result};++request_count;return result;
}
int main(){
#if !SDL_VERSION_ATLEAST(2,24,0)
    return 77;
#else
    SDL_setenv("SDL_AUDIODRIVER","dummy",1);
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1"); // owned hidden test window
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER)!=0)return 77;
    if(SDL_NumJoysticks()!=0){std::puts("SKIP: unowned joystick present; no physical device test authorized");SDL_Quit();return 77;}
    // Reviewed installed sdl2-compat e4df8a55 directly returns the SDL2 integer
    // virtual callback as SDL3 bool (0 erroneously means failure). Adapt this
    // OWNED callback only for that exact revision; never patch the runtime/SDL.
    const bool compat_quirk=std::strcmp(SDL_GetRevision(),"SDL-release-2.32.74-0-ge4df8a5")==0;
    virtual_success=compat_quirk?1:0;
    std::array<Motor,3> motors{};
    auto attach=[&](unsigned which){SDL_VirtualJoystickDesc desc{};desc.version=SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
        desc.type=SDL_JOYSTICK_TYPE_GAMECONTROLLER;desc.naxes=SDL_CONTROLLER_AXIS_MAX;desc.nbuttons=SDL_CONTROLLER_BUTTON_MAX;
        desc.button_mask=(1u<<SDL_CONTROLLER_BUTTON_MAX)-1;desc.axis_mask=(1u<<SDL_CONTROLLER_AXIS_MAX)-1;
        desc.vendor_id=0x1209;desc.product_id=Uint16(0x6000+which);desc.name="Owned GF output test";desc.userdata=&motors[which];desc.Rumble=rumble;
        return SDL_JoystickAttachVirtualEx(&desc);};
    const int wheel=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_WHEEL,2,2,0);check(wheel>=0,"owned non-controller gap");
    const int first=attach(0),second=attach(1);check(first>=0&&second>=0,"two owned virtual controllers attach");
    SDL_Joystick* js0=SDL_JoystickOpen(first);SDL_Joystick* js1=SDL_JoystickOpen(second);check(js0&&js1,"owned joystick handles");
    gf::headless::Render renderer;
    {
        gf::sdl::Platform platform;std::string error;const auto backends=platform.create({64,64,"Owned output test",false,true},&renderer,&error);
        if(!backends.complete()){std::fprintf(stderr,"SDL test window unavailable: %s\n",error.c_str());return 77;}
        backends.input->pump();
        check(request_count==0,"opening devices sends no motor requests");
        backends.haptics->stop(1);check(request_count==0,"unowned device isn't stopped by a no-op request");
        check(backends.haptics->supported(0)&&backends.haptics->supported(1)&&!backends.haptics->supported(2)&&!backends.haptics->supported(99),"capabilities follow actual open controller ports, skipping wheel");
        motors={};backends.haptics->rumble(1,0.5f,1);check(motors[0].calls==0&&motors[1].calls==1&&motors[1].low==32767&&motors[1].high==65535,"device1 receives correct amplitudes; no null/device0 routing");
        backends.haptics->rumble(0,-1,2);check(motors[0].low==0&&motors[0].high==65535,"finite amplitudes clamp");
        check(last_request.duration==250&&last_request.result==0,"active SDK request has bounded native hardware TTL");
        backends.haptics->rumble(0,NAN,1);
        check(last_request.controller==SDL_GameControllerFromInstanceID(SDL_JoystickInstanceID(js0))&&last_request.low==0&&last_request.high==0&&last_request.duration==0&&last_request.result==0,"nonfinite request becomes accepted SDL stop on device0");
        backends.haptics->stop(1);
        check(last_request.controller==SDL_GameControllerFromInstanceID(SDL_JoystickInstanceID(js1))&&last_request.low==0&&last_request.high==0&&last_request.result==0,"explicit stop accepted for selected device1");
        virtual_success=compat_quirk?0:-1;backends.haptics->rumble(0,0.3f,0.3f);
        check(last_request.result<0,"owned virtual driver failure is observed, not fake success");
        const auto attempted=request_count;backends.haptics->stop(0);
        check(request_count==attempted+1&&last_request.low==0&&last_request.high==0,"failed nonzero attempt still receives a targeted safety stop");
        virtual_success=compat_quirk?1:0;backends.haptics->rumble(0,0.4f,0.4f);backends.haptics->stop(0);
        check(SDL_JoystickSetVirtualButton(js1,SDL_CONTROLLER_BUTTON_A,1)==0,"owned button input set");backends.window->pump_events();backends.input->pump();
        check((backends.input->snapshot().pad(1).buttons&1)!=0,"input and haptic device1 share ownership/port");
        check(SDL_JoystickDetachVirtual(first)==0,"owned controller0 detaches");backends.window->pump_events();backends.input->pump();
        check(!backends.haptics->supported(0)&&backends.haptics->supported(1),"detach doesn't compact controller1 into controller0");
        const auto count=request_count;backends.haptics->rumble(0,1,1);check(request_count==count,"detached port cannot rumble another device");
        const int replacement=attach(2);check(replacement>=0,"owned replacement attaches");backends.window->pump_events();backends.input->pump();
        check(backends.haptics->supported(0),"hotplug fills vacated port");backends.haptics->rumble(0,1,0);check(motors[2].low==65535&&request_count==count+1&&last_request.controller!=SDL_GameControllerFromInstanceID(SDL_JoystickInstanceID(js1)),"replacement is routed without stealing stable device1");
        check(backends.audio->available(),"SDL dummy device is open (not audible playback proof)");
        gf::OptionOutputProfile p;p.level_gain={0,0.25f,0.5f,0.75f,1};p.preview_updates=10;gf::OptionOutput output(*backends.haptics);check(output.configure(p),"portable policy on real SDL seams");
        gf::OptionSettings s;s.mono=1;s.volume[0]=2;s.rumble_mask=3;const auto startup_requests=request_count;
        check(output.apply(s,{0,1,2,3},false)&&request_count==startup_requests,"PCM prefs apply without startup rumble or stopping unrelated owned requests");
        output.update();s.rumble_mask=0;output.apply(s,{0,1,2,3},false);
        check(request_count==startup_requests,"loaded ON does not turn motors on during subsequent update");
        const std::array<gf::s16,4> input={12000,4000,-8000,0};check(output.pcm().begin(2)&&output.pcm().add(gf::AudioBus::Bgm,input)&&output.pcm().submit(*backends.audio),"processed owned PCM queued to SDL");
        check(backends.audio->frames_available()<=2&&backends.audio->frames_consumed()<=2,"queue/consumption counters bounded by submitted frames");
        std::vector<gf::s16> owned_silence(192000*2,0);
        check(backends.audio->try_submit(owned_silence.data(),192000),"checked PCM submission is accepted by SDL");
        check(backends.audio->discard_queued()&&backends.audio->frames_available()==0&&backends.audio->frames_discarded()>0&&
            backends.audio->frames_consumed()+backends.audio->frames_discarded()==192002,"queue discard is not misreported as played samples");
        check(!backends.audio->try_submit(nullptr,1)&&!backends.audio->try_submit(input.data(),(1u<<20)+1),"checked invalid submissions have no effects");
        const auto old_discarded=backends.audio->frames_discarded();gf::u64 exact=0;
        check(backends.audio->try_submit(owned_silence.data(),192000)&&backends.audio->discard_queued_exact(&exact)&&exact>0&&
            backends.audio->frames_available()==0&&backends.audio->frames_discarded()==old_discarded+exact&&
            backends.audio->frames_consumed()+backends.audio->frames_discarded()==384002,"exact paused discard reports precisely the removed frames");
        check(!backends.audio->discard_queued_exact(nullptr)&&backends.audio->frames_discarded()==old_discarded+exact,"missing exact-discard destination refuses without effects");
        s.rumble_mask=2;check(output.apply(s,{0,1,2,3},true),"enable logical player2 preview through policy");output.update();
        check(last_request.controller==SDL_GameControllerFromInstanceID(SDL_JoystickInstanceID(js1))&&last_request.low==65535&&last_request.high==65535&&last_request.result==0,"preference preview reaches the owned SDL device");
        s.rumble_mask=0;check(output.apply(s,{0,1,2,3},true)&&last_request.controller==SDL_GameControllerFromInstanceID(SDL_JoystickInstanceID(js1))&&last_request.low==0&&last_request.high==0&&last_request.result==0,"preference OFF immediately requests stop on that same device");
        backends.audio->close();check(!backends.audio->available()&&backends.audio->frames_consumed()==0,"closed backend reports no output device/consumed frames");
        backends.audio->submit(input.data(),2);check(backends.audio->frames_consumed()==0,"missing device doesn't pretend to consume PCM");
        check(!backends.audio->try_submit(input.data(),2)&&!backends.audio->discard_queued()&&backends.audio->frames_discarded()==0,"closed device refuses checked output and queue discard");
        exact=99;check(!backends.audio->discard_queued_exact(&exact)&&exact==99,"closed exact discard refuses without inventing a count");
        check(!backends.audio->open(0,2)&&!backends.audio->open(48000,9),"invalid device format refuses");
        const auto directory=std::filesystem::temp_directory_path()/("gf-owned-sdl-range-"+std::to_string(backends.clock->monotonic_ns()));
        check(std::filesystem::create_directory(directory),"exclusive owned range-I/O test directory");
        const auto path=(directory/"audio.bin").string();const gf::u8 data[]={1,2,3,4,5,6};gf::u64 size=0;std::vector<gf::u8> range;
        check(backends.files->write(path,data,sizeof(data))&&backends.files->file_size(path,&size)&&size==6,"platform file-size seam");
        check(backends.files->read_range(path,2,3,range)&&range==std::vector<gf::u8>({3,4,5}),"platform exact range read");
        check(!backends.files->read_range(path,5,2,range)&&range.empty()&&!backends.files->read_range(path,~gf::u64(0),1,range),"range overflow/short reads refuse");
        check(backends.files->erase(path)&&std::filesystem::remove(directory),"only owned test files cleaned up");
        output.stop();backends.haptics->stop(0);backends.haptics->stop(1);
        SDL_JoystickClose(js0);SDL_JoystickClose(js1);
        // Device indices shift on detach; remove only the remaining owned
        // virtual devices in reverse order before platform destroys SDL.
        for(int i=SDL_NumJoysticks()-1;i>=0;--i)check(SDL_JoystickDetachVirtual(i)==0,"owned virtual cleanup");
        backends.window->pump_events();backends.input->pump();
    }
    std::printf("{\"sdl_output_checks\":%u,\"virtual_devices_only\":true,\"dummy_audio_only\":true,\"reviewed_compat_callback_adaptation\":%s,\"all_passed\":true}\n",checks,compat_quirk?"true":"false");
#endif
}
