#include <cstdio>
#include <fstream>
#include "engine/gameplay/borgs/normal_gred_beam_art.hpp"
#include "engine/gameplay/borgs/normal_gred_beam_transform.hpp"

namespace {
using namespace gf;
struct RecordingBackend final:IRenderBackend {
    std::vector<DrawCall> draws;
    std::vector<Vertex> vertices;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    RenderCapabilities capabilities()const override{return {};}
    bool upload_vertices(const Vertex* p,std::size_t n)override{vertices.assign(p,p+n);return true;}
    bool upload_matrix(u32,const f32*)override{return true;}
    bool upload_texture(u32,const TextureDesc&)override{return false;} // this record has no textures
    void begin_frame(const FrameDesc&)override{draws.clear();vertices.clear();}
    void submit(const DrawCall& call)override{draws.push_back(call);}
    void end_frame()override{}
    void present()override{}
};
}
int main(int argc,char** argv){
    using namespace gf;
    if(argc!=2){std::fprintf(stderr,"usage: %s ORDINARY_EFFECT_BANK.hsd\n",argv[0]);return 2;}
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<u8> bytes(417955);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))||
       file.peek()!=std::char_traits<char>::eof())return 1;
    const ByteView art{bytes.data(),bytes.size()};
    std::string error;
    auto check=[&](bool ok,const char* why){
        if(!ok)std::fprintf(stderr,"G-Red beam art: %s (%s)\n",why,error.c_str());
        return ok;
    };
    constexpr NormalGRedNeutralBSource red{0x0615,0,0};
    NormalGRedBeamArtBank bank;
    AnimatedModelScene beam;
    if(!check(bank.load(art,&error)&&!bank.prepare(red,1,&beam,&error)&&
              !bank.prepare({0x0615,1,0},0,&beam,&error)&&bank.prepare(red,0,&beam,&error),
              "exact ordinary bank/record/team; other selectors refuse"))return 1;
    const auto* model=beam.asset();
    if(!check(model&&model->model_roots==std::vector<u32>{0x828}&&model->joints.size()==2&&
              model->meshes.size()==2&&model->textures.empty()&&beam.animation().frame()==0,
              "authored two-mesh record and material frame zero"))return 1;
    NormalGRedBeamArtBank mutated=bank;
    bytes.back()^=1;
    if(!check(!mutated.load(art,&error)&&!mutated.ready()&&bank.ready()&&beam.ready(),
              "changed block cannot rebind or corrupt a prepared copy"))return 1;

    // Explicit detached render basis, not a collider or a scene allocation.
    const GRedBeamFlightSample flight{{0,0,0},{0,0,81},{0,0,80},80.5f,81};
    GRedBeamScalePlan scale;
    GRedBeamTransformPlan frame;
    if(!check(normal_gred_beam_scale_seed(red,1,flight.distance180,&scale)&&
              normal_gred_beam_transform(red,flight,scale,Mat4{},&frame)&&
              beam.set_root_world(frame.frame114,&error),"shared source-shaped root override"))return 1;
    RecordingBackend backend;RenderEngine render(backend);
    Camera camera;
    camera.eye[0]=140;camera.eye[1]=95;camera.eye[2]=260;camera.target[2]=38;
    camera.viewport_width=640;camera.viewport_height=480;
    if(!check(render.initialize({640,480,false,true,"G-Red beam art"},nullptr,&error),"shared render engine"))return 1;
    render.begin_frame({});
    if(!check(!beam.draw(render,camera),"prepared art does not draw until enabled"))return 1;
    beam.set_enabled(true);
    if(!check(beam.draw(render,camera)==2&&render.end_frame()&&backend.draws.size()==2&&
              !backend.vertices.empty()&&std::all_of(backend.draws.begin(),backend.draws.end(),[](const auto& call){
                  return !call.texture&&call.blend&&call.vertex_count&&call.vertex_count%3==0;
              }),"two authored blended mesh submissions, no texture or debug billboard"))return 1;
    std::printf("G-Red beam art: ordinary record 0, two authored draws, %zu vertices; no actor or projectile\n",
                backend.vertices.size());
}
