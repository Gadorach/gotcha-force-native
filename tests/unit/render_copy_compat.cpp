// New legacy-interface witness, compiled against retained before/current headers.
// Never executes an older test binary/main; digest equality is not pixel proof.
#include <cstdio>
#include "engine/graphics/render_engine.hpp"
#include "engine/gameplay/frame_digest.hpp"
int main(){
    gf::RecordingRenderBackend backend;gf::RenderEngine render(backend);std::string error;if(!render.initialize({64,64,false,true,"new compatibility witness"},nullptr,&error))return 2;
    for(unsigned frame=0;frame<16;++frame){render.begin_frame({frame,.1f,.2f,.3f});render.reset_arena();gf::Vertex v[3];v[0].x=1;v[1].x=20;v[2].y=30;v[0].r=gf::u8(frame);const auto base=render.append_vertices(v,3);render.state().post_rgb_add={gf::u8(frame),2,3};if(!render.draw(gf::Primitive::Triangles,base,3)||!render.draw(gf::Primitive::Triangles,base,3))return 3;render.end_frame();render.present();std::printf("%u %llu %u\n",frame,static_cast<unsigned long long>(backend.last_frame_digest()),backend.draws());}
}
