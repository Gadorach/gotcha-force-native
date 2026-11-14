#pragma once
#include <SDL.h>
#include "engine/input/window_cursor_visibility.hpp"
namespace gf::pc {
//One SDL window only. SDL_ShowCursor is process-local, not a desktop setting.
//Never hide for an unfocused window or leave the captured state behind on exit.
class WindowCursor {
public:
    explicit WindowCursor(SDL_Window* window):window_(window){}
    WindowCursor(const WindowCursor&)=delete;WindowCursor& operator=(const WindowCursor&)=delete;
    ~WindowCursor(){if(SDL_WasInit(SDL_INIT_VIDEO))restore();}
    bool update(bool custom_available){int x=0,y=0,width=0,height=0;const bool focused=window_&&SDL_GetMouseFocus()==window_&&SDL_GetKeyboardFocus()==window_;if(focused){SDL_GetMouseState(&x,&y);SDL_GetWindowSize(window_,&width,&height);}const bool inside=focused&&width>0&&height>0&&x>=0&&y>=0&&x<width&&y<height;return state_.update(custom_available&&inside,read,set);}
    bool restore(){return state_.restore(read,set);}
private:
    static int read(){return SDL_ShowCursor(SDL_QUERY);}
    static bool set(bool visible){return SDL_ShowCursor(visible?SDL_ENABLE:SDL_DISABLE)>=0;}
    SDL_Window* window_=nullptr;WindowCursorVisibility state_;
};
} //namespace gf::pc
