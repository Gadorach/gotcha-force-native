// SDL2 platform backends. Everything platform-specific for the desktop target
// lives here; engine code only sees the interfaces.
#include "platform/pc/sdl_platform.hpp"
#include "platform/pc/keyboard_layout.hpp"

#include <SDL.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>

namespace gf::sdl {
namespace {

// The device bit layout is shared with the pure keyboard mapper; base code
// still maps raw bits to game buttons through its own table.
using namespace gf::pc;

class Window final : public IWindowBackend {
public:
    bool create(const WindowConfig& config, std::string* error) override {
        if (window_) return true;
        // Lowest version covering the GX TEV/EFB pipeline: GL 2.1 + GLSL 1.20.
        // No core profile, so compatibility-profile drivers and software Mesa work.
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
        if (config.resizable) flags |= SDL_WINDOW_RESIZABLE;
        if (config.hidden) flags |= SDL_WINDOW_HIDDEN;
        window_ = SDL_CreateWindow(config.title.empty() ? "Gotcha Force" : config.title.c_str(),
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   static_cast<int>(config.width), static_cast<int>(config.height),
                                   flags);
        if (!window_) {
            if (error) *error = std::string("SDL_CreateWindow: ") + SDL_GetError();
            return false;
        }
        closed_ = false;
        return true;
    }

    void destroy() override {
        if (window_) {
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
    }

    bool pump_events() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) closed_ = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) closed_ = true;
        }
        return !closed_;
    }

    bool should_close() const override { return closed_; }
    void set_title(const std::string& title) override {
        if (window_) SDL_SetWindowTitle(window_, title.c_str());
    }
    void size(unsigned* width, unsigned* height) const override {
        int w = 0, h = 0;
        if (window_) SDL_GL_GetDrawableSize(window_, &w, &h);
        if (width) *width = static_cast<unsigned>(w > 0 ? w : 640);
        if (height) *height = static_cast<unsigned>(h > 0 ? h : 480);
    }
    void logical_size(unsigned* width,unsigned* height)const override{
        int w=0,h=0;if(window_)SDL_GetWindowSize(window_,&w,&h);
        if(width)*width=unsigned(w>0?w:640);
        if(height)*height=unsigned(h>0?h:480);
    }
    bool focused() const override {
        return window_ && (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    void swap_buffers() override {
        if (window_) SDL_GL_SwapWindow(window_);
    }
    void* native_handle() override { return window_; }

private:
    SDL_Window* window_ = nullptr;
    bool closed_ = false;
};

// SDL reports keyboard, mouse and up to four controllers. Touch is not
// available on this target; a mobile port implements the same snapshot from its
// own touch API and needs no change above this seam.
class Input final : public IInputBackend {
public:
    Input() {
        ready_=SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER)==0;
        if(ready_)refresh();
    }
    ~Input() override {close();}
    void close(){
        for(unsigned i=0;i<InputSnapshot::kMaxPorts;++i)if(pads_[i]){
            if(rumbling_[i])SDL_GameControllerRumble(pads_[i],0,0,0);
            SDL_GameControllerClose(pads_[i]);pads_[i]=nullptr;rumbling_[i]=false;
        }
        if(ready_){SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);ready_=false;}
    }
    SDL_GameController* controller(unsigned port)const{
        return port<InputSnapshot::kMaxPorts&&pads_[port]&&SDL_GameControllerGetAttached(pads_[port])?pads_[port]:nullptr;
    }
    bool owns_rumble(unsigned port)const{return port<InputSnapshot::kMaxPorts&&rumbling_[port];}
    void own_rumble(unsigned port,bool active){if(port<InputSnapshot::kMaxPorts)rumbling_[port]=active;}

    void pump() override {
        if(ready_){refresh();SDL_GameControllerUpdate();}
        snapshot_.clear();
        // Keyboard feeds ports 0 and 1 alongside any controller, so local
        // play works without a device. Player 1: arrows/WASD, Space, Shift,
        // E, Q, L, R, Z, Enter. Player 2: IJKL, U, H, O, N, M, comma,
        // period, keypad Enter.
        RawInputState& keyboard = snapshot_.pad(0);
        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        if (keys) {
            snapshot_.frame_step = keys[SDL_SCANCODE_P] != 0;
            snapshot_.camera_toggle = keys[SDL_SCANCODE_C] != 0;
            snapshot_.alt_arrow=(keys[SDL_SCANCODE_LALT]||keys[SDL_SCANCODE_RALT])&&
                (keys[SDL_SCANCODE_UP]!=keys[SDL_SCANCODE_DOWN]||keys[SDL_SCANCODE_LEFT]!=keys[SDL_SCANCODE_RIGHT]);
            if (keys[SDL_SCANCODE_ESCAPE]) keyboard.quit = true;
            KeyboardPad first;
            first.start=keys[SDL_SCANCODE_RETURN]!=0;
            first.a=keys[SDL_SCANCODE_SPACE]!=0;
            first.b=keys[SDL_SCANCODE_LSHIFT]!=0;
            first.x=keys[SDL_SCANCODE_E]!=0;
            first.y=keys[SDL_SCANCODE_Q]!=0;
            first.dpad_up=keys[SDL_SCANCODE_UP]!=0;
            first.dpad_down=keys[SDL_SCANCODE_DOWN]!=0;
            first.dpad_left=keys[SDL_SCANCODE_LEFT]!=0;
            first.dpad_right=keys[SDL_SCANCODE_RIGHT]!=0;
            first.l=keys[SDL_SCANCODE_L]!=0;
            first.r=keys[SDL_SCANCODE_R]!=0;
            first.z=keys[SDL_SCANCODE_Z]!=0;
            first.axis_up=keys[SDL_SCANCODE_W]!=0;
            first.axis_down=keys[SDL_SCANCODE_S]!=0;
            first.axis_left=keys[SDL_SCANCODE_A]!=0;
            first.axis_right=keys[SDL_SCANCODE_D]!=0;
            map_keyboard_pad(first,&keyboard);
            KeyboardPad second;
            second.start=keys[SDL_SCANCODE_KP_ENTER]!=0;
            second.a=keys[SDL_SCANCODE_U]!=0;
            second.b=keys[SDL_SCANCODE_H]!=0;
            second.x=keys[SDL_SCANCODE_O]!=0;
            second.y=keys[SDL_SCANCODE_N]!=0;
            second.dpad_up=second.axis_up=keys[SDL_SCANCODE_I]!=0;
            second.dpad_down=second.axis_down=keys[SDL_SCANCODE_K]!=0;
            second.dpad_left=second.axis_left=keys[SDL_SCANCODE_J]!=0;
            second.dpad_right=second.axis_right=keys[SDL_SCANCODE_L]!=0;
            second.l=keys[SDL_SCANCODE_M]!=0;
            second.r=keys[SDL_SCANCODE_COMMA]!=0;
            second.z=keys[SDL_SCANCODE_PERIOD]!=0;
            map_keyboard_pad(second,&snapshot_.pad(1));
        }
        snapshot_.port_count=2;
        for(unsigned i=0;i<InputSnapshot::kMaxPorts;++i)if(pads_[i])snapshot_.port_count=i+1;

        // Mouse / pointer. Screen space; the game decides what it means.
        int mx = 0, my = 0;
        const Uint32 mask = SDL_GetMouseState(&mx, &my);
        snapshot_.pointer.x = mx;
        snapshot_.pointer.y = my;
        snapshot_.pointer.left = (mask & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        snapshot_.pointer.right = (mask & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
        snapshot_.pointer.middle = (mask & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
        // "Inside the window" is a window-relative flag; SDL reports focus, so
        // record focus and let base code decide what the pointer means.
        snapshot_.pointer.within_window = SDL_GetKeyboardFocus() != nullptr;

        // Controllers, port order preserved.
        for (unsigned i = 0; i < InputSnapshot::kMaxPorts; ++i) {
            SDL_GameController* pad = controller(i);if(!pad)continue;
            RawInputState& state = snapshot_.pad(i);
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_A)) state.buttons |= kButtonA;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_B)) state.buttons |= kButtonB;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X)) state.buttons |= kButtonX;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y)) state.buttons |= kButtonY;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START)) state.buttons |= kButtonStart;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP)) state.buttons |= kButtonDpadUp;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN)) state.buttons |= kButtonDpadDown;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_LEFT)) state.buttons |= kButtonDpadLeft;
            if (SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) state.buttons |= kButtonDpadRight;
            state.axis[0] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
            state.axis[1] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);
            state.axis[2] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTX);
            state.axis[3] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_RIGHTY);
            state.axis[4] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERLEFT);
            state.axis[5] = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_TRIGGERRIGHT);
        }
    }

    const InputSnapshot& snapshot() const override { return snapshot_; }
    const char* describe() const override { return "sdl2 keyboard+mouse+controllers"; }

private:
    void refresh(){
        for(unsigned i=0;i<InputSnapshot::kMaxPorts;++i)if(pads_[i]&&!SDL_GameControllerGetAttached(pads_[i])){
            if(rumbling_[i])SDL_GameControllerRumble(pads_[i],0,0,0);
            SDL_GameControllerClose(pads_[i]);pads_[i]=nullptr;rumbling_[i]=false;
        }
        // Keep logical backend ports stable across detach; skip unsupported
        // joystick indices instead of letting one block all later controllers.
        const int count=SDL_NumJoysticks();
        for(int index=0;index<count;++index){
            if(!SDL_IsGameController(index))continue;
            const auto id=SDL_JoystickGetDeviceInstanceID(index);bool present=false;
            for(auto* pad:pads_)if(pad&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))==id)present=true;
            if(present)continue;
            auto slot=std::find(std::begin(pads_),std::end(pads_),nullptr);if(slot==std::end(pads_))break;
            *slot=SDL_GameControllerOpen(index);
        }
    }
    SDL_GameController* pads_[InputSnapshot::kMaxPorts]{};
    bool rumbling_[InputSnapshot::kMaxPorts]{};
    bool ready_=false;
    InputSnapshot snapshot_{};
};

// SDL audio sink. Queues interleaved s16; if no device can be opened the backend
// still exists, but reports unavailable/zero consumed frames, not pretend sound.
class Audio final : public IAudioBackend {
public:
    bool open(unsigned sample_rate, unsigned channels) override {
        close();
        if(!sample_rate||sample_rate>192000||(channels!=1&&channels!=2))return false;
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return false;
        SDL_AudioSpec want{};
        want.freq = static_cast<int>(sample_rate);
        want.format = AUDIO_S16SYS;
        want.channels = static_cast<Uint8>(channels);
        want.samples = 1024;
        device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have_, 0);
        if (!device_) return false;
        SDL_PauseAudioDevice(device_, 0);
        return true;
    }
    void close() override {
        if (device_) {
            SDL_CloseAudioDevice(device_);
            device_ = 0;
        }
        frames_=discarded_=0;have_={};
    }
    void submit(const s16* interleaved, std::size_t frames) override {
        (void)try_submit(interleaved,frames);
    }
    bool try_submit(const s16* interleaved,std::size_t frames)override{
        if(!device_||!interleaved||!frames||frames>(1u<<20))return false;
        if(SDL_QueueAudio(device_,interleaved,static_cast<Uint32>(frames*sizeof(s16)*have_.channels))!=0)return false;
        frames_+=frames;return true;
    }
    bool discard_queued()override{
        u64 dropped=0;return discard_queued_exact(&dropped);
    }
    bool discard_queued_exact(u64* dropped)override{
        if(!device_||!dropped)return false;
        SDL_PauseAudioDevice(device_,1);const u64 count=frames_available();SDL_ClearQueuedAudio(device_);
        discarded_+=count;*dropped=count;SDL_PauseAudioDevice(device_,0);return true;
    }
    u64 frames_discarded()const override{return discarded_;}
    u64 frames_available()const override{return device_?SDL_GetQueuedAudioSize(device_)/(sizeof(s16)*have_.channels):0;}
    u64 frames_consumed() const override {const u64 queued=frames_available(),accepted=frames_>=discarded_?frames_-discarded_:0;return accepted>=queued?accepted-queued:0;}
    bool available()const override{return device_!=0;}
private:
    SDL_AudioDeviceID device_ = 0;
    SDL_AudioSpec have_{};
    u64 frames_ = 0,discarded_=0;
};

class Haptics final : public IHaptics {
public:
    explicit Haptics(Input& input):input_(input){}
    bool supported(unsigned port) const override {
        auto* pad=input_.controller(port);
#if SDL_VERSION_ATLEAST(2,0,18)
        return pad&&SDL_GameControllerHasRumble(pad)==SDL_TRUE;
#else
        (void)pad;return false; // don't invent capability on older SDL
#endif
    }
    void rumble(unsigned port, f32 low, f32 high) override {
        auto* pad=input_.controller(port);if(!pad)return;
        if(!std::isfinite(low)||!std::isfinite(high))low=high=0;
        const auto l=Uint16(std::clamp(low,0.0f,1.0f)*65535.0f),h=Uint16(std::clamp(high,0.0f,1.0f)*65535.0f);
        if(!l&&!h&&!input_.owns_rumble(port))return;
        // Bounded SDK duration, NOT original game timing or a proven physical
        // failsafe. Active portable requests refresh it; real driver/actuator
        // stop timing (including a stalled event pump) remains unqualified.
        const int result=SDL_GameControllerRumble(pad,l,h,(l||h)?250:0);
        // Remember even a failed nonzero attempt: a driver may have partially
        // applied it before reporting failure. A later stop still targets only
        // this device, never an unrelated controller.
        input_.own_rumble(port,l||h||result!=0);
    }
    void stop_all(){for(unsigned port=0;port<4;++port)rumble(port,0,0);}
private:
    Input& input_;
};

class Paths final : public IUserPaths {
public:
    std::string save_dir() override { return base("GOTCHAFORCE_SAVE_DIR", "save"); }
    std::string config_dir() override { return base("GOTCHAFORCE_CONFIG_DIR", "config"); }
    std::string cache_dir() override { return base("GOTCHAFORCE_CACHE_DIR", "cache"); }
    std::string asset_dir() override { return base("GOTCHAFORCE_ASSET_DIR", "assets"); }
private:
    static std::string base(const char* env, const char* fallback) {
        if (const char* value = std::getenv(env)) return value;
        return fallback;
    }
};

// Plain host filesystem. Archive formats belong to base code layered on top of
// this, not to the platform target.
class FileSystem final : public IFileSystem {
public:
    bool file_size(const std::string& name,u64* out)override{
        if(!out)return false;
        std::error_code ec;const auto size=std::filesystem::file_size(name,ec);if(ec)return false;
        *out=u64(size);return true;
    }
    bool read_range(const std::string& name,u64 offset,std::size_t size,std::vector<u8>& out)override{
        out.clear();if(size>(64u<<20))return false;
        std::ifstream file(name,std::ios::binary|std::ios::ate);if(!file)return false;
        const auto length=file.tellg();if(length<0||offset>u64(length)||u64(size)>u64(length)-offset)return false;
        file.seekg(std::streamoff(offset),std::ios::beg);if(!file)return false;
        out.resize(size);if(size)file.read(reinterpret_cast<char*>(out.data()),std::streamsize(size));
        if(!file){out.clear();return false;}return true;
    }
    bool read(const std::string& name, std::vector<u8>& out) override {
        std::ifstream file(name, std::ios::binary | std::ios::ate);
        if (!file) { out.clear(); return false; }
        const std::streamoff size = file.tellg();
        if (size < 0) { out.clear(); return false; }
        file.seekg(0, std::ios::beg);
        out.resize(static_cast<std::size_t>(size));
        // One bulk read: an istreambuf_iterator copy costs ~9 MB/s here, which
        // turned the 455 MB archive into a 50 second start-up stall.
        if (size != 0) {
            file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(size));
            if (!file) { out.clear(); return false; }
        }
        return true;
    }
    bool write(const std::string& name, const u8* data, std::size_t size) override {
        std::error_code ec;
        std::filesystem::path path(name);
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), ec);
        std::ofstream file(name, std::ios::binary | std::ios::trunc);
        if (!file) return false;
        file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
        return file.good();
    }
    bool exists(const std::string& name) override {
        std::error_code ec;
        return std::filesystem::exists(name, ec);
    }
    bool erase(const std::string& name) override {
        std::error_code ec;
        return std::filesystem::remove(name, ec);
    }
};

// Slot files under the save directory. The save engine owns the record format;
// this only moves bytes.
class Save final : public ISaveBackend {
public:
    explicit Save(IUserPaths* paths) : paths_(paths) {}
    bool load(u32 slot, std::vector<u8>& out) override {
        return files_.read(path(slot), out);
    }
    SaveReadStatus read_slot(u32 slot,std::vector<u8>& out) override {
        std::error_code ec;const bool present=std::filesystem::exists(path(slot),ec);
        if(ec){out.clear();return SaveReadStatus::Error;}
        if(!present){out.clear();return SaveReadStatus::Missing;}
        return load(slot,out)?SaveReadStatus::Loaded:SaveReadStatus::Error;
    }
    bool store(u32 slot, const u8* data, std::size_t size) override {
        // Same-directory replacement: a failed/short write must not truncate the
        // previous slot. C11 exclusive-create prevents clobbering an existing
        // temporary file; rename replaces atomically on the Linux host.
        if(!data&&size)return false;
        const auto target=std::filesystem::path(path(slot));std::error_code ec;
        std::filesystem::create_directories(target.parent_path(),ec);if(ec)return false;
        const auto ticks=std::chrono::steady_clock::now().time_since_epoch().count();
        const auto temp=target.string()+".tmp-"+std::to_string(ticks)+"-"+std::to_string(++serial_);
        std::FILE* file=std::fopen(temp.c_str(),"wbx");if(!file)return false;
        bool ok=std::fwrite(data,1,size,file)==size;
        if(std::fflush(file)!=0)ok=false;
        if(std::fclose(file)!=0)ok=false;
        if(ok){std::filesystem::rename(temp,target,ec);ok=!ec;}
        if(!ok){std::error_code ignored;std::filesystem::remove(temp,ignored);}
        return ok;
    }
    bool erase(u32 slot) override { return files_.erase(path(slot)); }
private:
    std::string path(u32 slot) const {
        return (paths_ ? paths_->save_dir() : std::string("save")) + "/slot" + std::to_string(slot) + ".sav";
    }
    IUserPaths* paths_;
    FileSystem files_;
    u64 serial_=0;
};

// Networking is intentionally disabled on the desktop target until a transport
// is chosen; base code sees a valid but disconnected backend.
class Network final : public INetworkBackend {
public:
    bool open(const std::string&) override { return false; }
    void close() override {}
    bool send(const u8*, std::size_t) override { return false; }
    bool receive(std::vector<u8>&) override { return false; }
    bool connected() const override { return false; }
};

class Log final : public ILogBackend {
public:
    void write(LogLevel level, const char* category, const std::string& message) override {
        static const char* names[] = {"trace", "debug", "info", "warn", "error"};
        const unsigned index = static_cast<unsigned>(level);
        std::fprintf(stderr, "[%s] %s: %s\n", index < 5 ? names[index] : "?", category ? category : "-",
                     message.c_str());
    }
    void flush() override { std::fflush(stderr); }
};

class Clock final : public IClock {
public:
    u64 monotonic_ns() override {
        return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    void sleep_for_ns(u64 ns) override {
        std::this_thread::sleep_for(std::chrono::nanoseconds(ns));
    }
};

} // namespace

std::unique_ptr<IFileSystem> create_file_system(){return std::make_unique<FileSystem>();}
std::unique_ptr<ILogBackend> create_log_backend(){return std::make_unique<Log>();}
std::unique_ptr<IAudioBackend> create_audio_backend(){return std::make_unique<Audio>();}

struct Platform::Impl {
    Window window;
    Input input;
    Audio audio;
    Haptics haptics{input};
    Paths paths;
    FileSystem files;
    Save save{&paths};
    Network network;
    Log log;
    Clock clock;
    bool sdl_ready = false;
    Backends backends{};
};

Platform::Platform() : impl_(std::make_unique<Impl>()) {}

Platform::~Platform() {
    if (impl_) {
        impl_->haptics.stop_all();
        impl_->input.close();
        impl_->audio.close();
        impl_->window.destroy();
        if (impl_->sdl_ready) SDL_Quit();
    }
}

Backends Platform::create(const WindowConfig& config, IRenderBackend* renderer, std::string* error) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER) != 0) {
        if (error) *error = std::string("SDL_Init: ") + SDL_GetError();
        return {};
    }
    impl_->sdl_ready = true;
    if (!impl_->window.create(config, error)) return {};
    if (!renderer) {
        if (error) *error = "a render backend is required";
        return {};
    }
    // The host's RenderEngine owns renderer initialization and shutdown. Creating
    // another context here leaked the first context and its objects.
    impl_->audio.open(48000, 2);   // best effort; silence is acceptable
    impl_->backends = Backends{&impl_->window, renderer, &impl_->audio, &impl_->input, &impl_->haptics,
                               &impl_->save, &impl_->network, &impl_->files, &impl_->paths,
                               &impl_->log, &impl_->clock};
    return impl_->backends;
}

void Platform::pump_events() {
    if (impl_) impl_->window.pump_events();
}

} // namespace gf::sdl
