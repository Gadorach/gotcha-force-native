#include <cstdio>
#include <cstdlib>
#include <type_traits>
#include "engine/graphics/render_engine.hpp"
#include "engine/gameplay/frame_digest.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;RecordingRenderBackend backend;RenderFrameReceipt escaped;
    static_assert(!std::is_copy_constructible_v<RenderEngine>);
    static_assert(!std::is_copy_assignable_v<RenderEngine>);
    {
        RenderEngine render(backend);check(!render.frame_receipt().live());
        check(render.initialize({640,448,false,true,"receipt"},nullptr,&error));
        check(render.canvas_size()==std::array<u32,2>{640,448});
        render.begin_frame({});auto first=render.frame_receipt();check(first.live()&&first.same_frame(render.frame_receipt()));
        render.begin_frame({});auto second=render.frame_receipt();check(!first.live()&&second.live()&&!first.same_frame(second));
        check(render.end_frame()&&!second.live()&&!render.frame_receipt().live());
        render.begin_frame({});auto third=render.frame_receipt();render.shutdown();check(!third.live()&&!render.frame_receipt().live());
        check(render.initialize({320,224,false,true,"receipt reinit"},nullptr,&error));
        render.begin_frame({});auto fourth=render.frame_receipt();
        check(render.initialize({640,448,false,true,"receipt replace"},nullptr,&error)&&!fourth.live()&&!render.frame_receipt().live());
        render.begin_frame({});escaped=render.frame_receipt();check(escaped.live());
        RenderEngine other(backend);other.begin_frame({});check(!escaped.same_frame(other.frame_receipt()));
    }
    check(!escaped.live());std::printf("render frame scope: %u checks PASS\n",checks);
}
