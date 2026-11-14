#include <SDL.h>
#include <cstdio>
#include <cstdlib>
namespace cursor_test {SDL_Window* mouse=nullptr;SDL_Window* keyboard=nullptr;SDL_Window* mouse_focus(){return mouse;}SDL_Window* keyboard_focus(){return keyboard;}}
//Only focus ownership is an owned declaration. Actual SDL cursor query/change
//and init/window/shutdown calls delegate unchanged in a dummy-video process.
#define SDL_GetMouseFocus cursor_test::mouse_focus
#define SDL_GetKeyboardFocus cursor_test::keyboard_focus
#include "platform/pc/window_cursor.hpp"
#undef SDL_GetMouseFocus
#undef SDL_GetKeyboardFocus
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"SDL window cursor: %s\n",why);std::abort();}}}
int main(){check(SDL_Init(SDL_INIT_VIDEO)==0,"owned dummy video");auto* window=SDL_CreateWindow("owned cursor API test",0,0,64,64,SDL_WINDOW_HIDDEN);check(window!=nullptr,"owned hidden window");const int prior=SDL_ShowCursor(SDL_QUERY);check(prior>=0&&SDL_ShowCursor(SDL_ENABLE)>=0,"record actual SDL state");
    {gf::pc::WindowCursor owner(window);check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"no focus means no hide");cursor_test::mouse=window;check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"mouse focus alone insufficient");cursor_test::keyboard=window;check(owner.update(false)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"missing game cursor means no hide");check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE,"owned focus declarations gate real SDL hide");check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE,"steady hidden state");cursor_test::keyboard=nullptr;check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"losing keyboard focus restores real prior state");cursor_test::keyboard=window;check(SDL_ShowCursor(SDL_DISABLE)>=0&&owner.update(true)&&owner.restore()&&SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE,"preexisting hidden state retained");check(SDL_ShowCursor(SDL_ENABLE)>=0&&owner.update(true),"capture visible state");check(SDL_ShowCursor(SDL_ENABLE)>=0&&owner.restore()&&SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"later explicit visible choice retained");check(owner.update(true)&&SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE,"destructor owns one final request");}
    check(SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE,"destructor restores before window teardown");check(SDL_ShowCursor(prior)>=0,"test restores its actual original process state");SDL_DestroyWindow(window);SDL_Quit();std::printf("SDL owned-window cursor API: %u checks PASS (focus declared, not physical focus proof)\n",checks);return 0;
}
