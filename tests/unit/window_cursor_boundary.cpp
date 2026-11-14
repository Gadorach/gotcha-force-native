#include <SDL.h>
#include <cstdio>
#include <cstdlib>
namespace cursor_boundary {SDL_Window* window=nullptr;int x=12,y=12;SDL_Window* focus(){return window;}Uint32 mouse(int* px,int* py){*px=x;*py=y;return 0;}}
//Owned focus/position declarations; actual SDL window size and cursor state APIs.
#define SDL_GetMouseFocus cursor_boundary::focus
#define SDL_GetKeyboardFocus cursor_boundary::focus
#define SDL_GetMouseState cursor_boundary::mouse
#include "platform/pc/window_cursor.hpp"
#undef SDL_GetMouseFocus
#undef SDL_GetKeyboardFocus
#undef SDL_GetMouseState
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){check(SDL_Init(SDL_INIT_VIDEO)==0);cursor_boundary::window=SDL_CreateWindow("owned cursor boundary test",0,0,64,64,SDL_WINDOW_HIDDEN);check(cursor_boundary::window!=nullptr);const int prior=SDL_ShowCursor(SDL_QUERY);check(prior>=0&&SDL_ShowCursor(SDL_ENABLE)>=0);{gf::pc::WindowCursor owner(cursor_boundary::window);for(unsigned side=0;side<4;++side){cursor_boundary::x=cursor_boundary::y=12;check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE);cursor_boundary::x=side==0?-1:side==1?64:12;cursor_boundary::y=side==2?-1:side==3?64:12;check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE);check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE);}check(owner.restore());}check(SDL_ShowCursor(prior)>=0);SDL_DestroyWindow(cursor_boundary::window);SDL_Quit();std::printf("SDL cursor outside-window/implicit-capture boundary: %u new checks PASS\n",checks);return 0;}
