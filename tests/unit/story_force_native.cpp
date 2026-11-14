#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include "engine/gameplay/story_force_native_presentation.hpp"
#include "engine/gameplay/story_force_editor_bridge.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
#include "tests/unit/story_map_rebuild_fixture.hpp"
namespace {using namespace gf;unsigned checks=0;void check(bool x,const char* why){++checks;if(!x){std::fprintf(stderr,"native force: %s\n",why);std::abort();}}
struct Files final:IFileSystem {
    std::map<std::string,std::vector<u8>> bytes;unsigned sizes=0,ranges=0,whole=0,writes=0;std::string fail,short_read,large;std::function<void()> hook;
    bool read(const std::string&,std::vector<u8>&)override{++whole;return false;}
    bool file_size(const std::string& name,u64* out)override{++sizes;if(name==fail||!out||!bytes.contains(name))return false;*out=name==large?u64(65)<<20:bytes.at(name).size();return true;}
    bool read_range(const std::string& name,u64 at,std::size_t count,std::vector<u8>& out)override{++ranges;out.clear();if(name==fail||!bytes.contains(name)||at||count!=bytes.at(name).size())return false;out=bytes.at(name);if(name==short_read&&!out.empty())out.pop_back();if(hook){auto action=std::move(hook);action();}return true;}
    bool write(const std::string&,const u8*,std::size_t)override{++writes;return false;}bool exists(const std::string& name)override{return bytes.contains(name);}bool erase(const std::string&)override{++writes;return false;}
};
struct Backend final:IRenderBackend {
    RecordingRenderBackend recording;unsigned textures=0,matrices=0;unsigned fail_texture=0,fail_matrix=0;
    bool initialize(const RenderConfig& c,IWindowBackend* w,std::string* e)override{return recording.initialize(c,w,e);}void shutdown()override{recording.shutdown();}RenderCapabilities capabilities()const override{return recording.capabilities();}
    bool upload_vertices(const Vertex* p,std::size_t n)override{return recording.upload_vertices(p,n);}bool upload_matrix(u32 i,const f32* p)override{++matrices;return matrices!=fail_matrix&&recording.upload_matrix(i,p);}bool upload_texture(u32 i,const TextureDesc& t)override{++textures;return textures!=fail_texture&&recording.upload_texture(i,t);}
    void begin_frame(const FrameDesc& d)override{recording.begin_frame(d);}void submit(const DrawCall& d)override{recording.submit(d);}void end_frame()override{recording.end_frame();}void present()override{recording.present();}
};
StoryForceSetupFacts facts(){StoryForceSetupFacts f;f.active_event_row=2;f.source50=1;f.source78=500;check(StoryReplayContext::from_declared_index(5,&f.replay),"declared replay5");return f;}
void owned(){const auto catalog=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(catalog),"owner");const auto profile=story_map_rebuild_fixture::profile();StoryForceNativeSpec empty;StoryForceNativePresentation page;auto f=facts();
    check(!page.open(profile,f,owner,20,0,1,empty)&&!page.open(profile,f,owner,0,4,1,empty)&&!page.open(profile,f,owner,0,0,0,empty)&&!page.open(profile,f,owner,0,1,3,empty),"slot/port/empty or mismatched source mask refuses");
    Backend backend;RenderEngine render(backend);std::string error;check(render.initialize({640,480,false,true,"native owned"},nullptr,&error),"recording init");Files files;std::array<SourceSetupPad,4> pads{};StoryForceNativeEvent event;
    empty.catalog_fingerprint=catalog.fingerprint();check(page.open(profile,f,owner,0,0,1,empty)&&!page.step(files,render,owner,pads,&event,&error)&&page.phase()==StoryForceNativePhase::Failed&&!page.assets_ready()&&!event.request_music_stops&&!files.sizes&&!backend.textures,"missing manifest cannot publish readiness/services");
    f.source50=0;check(page.open(profile,f,owner,0,0,0,empty)&&page.step(files,render,owner,pads,&event,&error)&&event.destination==StoryForceNativeChoiceKind::Bypass&&!files.sizes&&!backend.textures,"disabled selector bypasses without file work or fabricated roster");StoryForceNativeChoice choice;check(page.take_choice(owner,&choice)&&choice.kind()==StoryForceNativeChoiceKind::Bypass&&choice.force()==kForceCount&&choice.current(owner)&&!page.take_choice(owner,&choice),"one-shot bypass");
    f=facts();check(page.open(profile,f,owner,0,0,1,empty)&&owner.create(catalog)&&!page.step(files,render,owner,pads,&event,&error)&&!files.sizes,"reload before loader refuses");
    for(unsigned duration:{1u,4u,30u,32u,60u})for(auto direction:{ScreenFadeDirection::Reveal,ScreenFadeDirection::Cover}){ScreenFade fade;check(fade.request(0,u16(duration),direction),"fade request");for(unsigned i=0;i<duration;++i){fade.advance();check(fade.remaining()==duration-i-1,"count before next callback");}if(direction==ScreenFadeDirection::Cover)check(fade.active()&&fade.draw()&&u8(fade.colour())==255&&!fade.remaining(),"cover stays active opaque at count0");else check(!fade.active()&&!fade.draw(),"reveal terminal suppresses drawing");fade.clear();check(!fade.active()&&!fade.remaining(),"clear preserves only source count/active semantics");fade.advance();check(!fade.draw(),"next advance clears draw flag");}
    render.shutdown();std::printf("Story native force: %u owned checks PASS; no caller fade/resource completion booleans\n",checks);
}
std::vector<u8> read(const char* name){std::ifstream file(name,std::ios::binary);check(bool(file),"source fixture input");return {std::istreambuf_iterator<char>(file),{}};}
u32 fingerprint(const std::vector<u8>& bytes){u32 h=2166136261;for(u8 b:bytes)h=(h^b)*16777619u;return h;}
struct Save final:ISaveBackend {unsigned calls=0;bool load(u32,std::vector<u8>&)override{++calls;return false;}bool store(u32,const u8*,std::size_t)override{++calls;return false;}bool erase(u32)override{++calls;return false;}};
void source(int argc,char** argv){check(argc==11,"source inputs");const auto catalog_bytes=read(argv[1]);ForceCatalog catalog;std::string error;check(catalog.load({catalog_bytes.data(),catalog_bytes.size()},&error),"source catalog");Files original;StoryForceNativeSpec spec;spec.catalog_fingerprint=catalog.fingerprint();for(unsigned i=0;i<StoryForceNativeSpec::Count;++i){spec.paths[i]="native-role-"+std::to_string(i);original.bytes[spec.paths[i]]=read(argv[i+2]);spec.fingerprints[i]=fingerprint(original.bytes.at(spec.paths[i]));}
    FILE* file=std::fopen(argv[10],"wx");check(file,"exclusive native timeline wire");auto word=[&](u32 x){u8 b[4];store_be32(b,x);check(std::fwrite(b,1,4,file)==4,"test wire");};unsigned frames=0;
    const auto profile=story_map_rebuild_fixture::profile();
    for(unsigned scenario=0;scenario<6;++scenario){Files files=original;ForceInventory owner;check(owner.create(catalog)&&owner.place(19,0,1)==ForceChange::Applied&&owner.place(19,1,2)==ForceChange::Applied&&owner.place(7,0,2)==ForceChange::Applied,"source inventory");const auto revision=owner.revision();auto f=facts();if(scenario==4)f.source50=0;if(scenario==5){f.action=StoryStartAction::Special;f.source50=0;f.event_energy_override=0;}
        const unsigned requested=scenario==1||scenario==3?19:scenario==2?7:0;const u8 port=u8(scenario%4);Backend backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"native source CPU"},nullptr,&error),"source render");StoryForceNativePresentation page;check(page.open(profile,f,owner,requested,port,u8(1u<<port),spec),"native open");StoryForceNativeChoice result;unsigned stops=0,starts=0,music_fades=0;
        for(unsigned frame=0;frame<220;++frame){std::array<SourceSetupPad,4> pads{};const u32 action=scenario==2?0x202u:scenario==3?0x500u:scenario==5?0x1000u:0x100u;const u32 raw=frame<68?0x100u:frame==71?action:0u;for(unsigned p=0;p<4;++p)pads[p]={p==port?raw:0x700u,true};StoryForceNativeEvent event;
            render.begin_frame({0,0,0,0});render.reset_arena();render.state()={};check(page.step(files,render,owner,pads,&event,&error),error.c_str());const bool draw=page.draw(render,owner,&error);if(!draw)std::fprintf(stderr,"native frame %u scenario%u: %s\n",frame,scenario,error.c_str());check(draw&&render.end_frame(),"native page/overlay submission");render.present();
            if(page.has_page()||page.fade().draw())check(backend.recording.draws()>0&&backend.recording.last_frame_digest()!=0,"real submitted page/fade work after present");
            stops+=event.request_music_stops;starts+=event.request_setup_music;music_fades+=event.request_music_fade!=0;
            for(u32 x:{scenario,frame,u32(page.phase()),u32(page.callbacks()),u32(page.assets_ready()),u32(page.has_page()),u32(page.fade().remaining()),page.fade().colour(),u32(page.fade().active()),u32(page.fade().draw()),u32(page.selector_phase()),u32(page.shown_force()),u32(page.input().held),u32(page.input().previous),u32(page.input().pressed),u32(page.input().repeated),u32(event.request_source_pools),u32(event.request_music_stops),u32(event.request_setup_music),u32(event.request_music_fade),u32(event.destination),u32(event.sound_count),u32(event.sounds[0]),u32(event.sounds[1])})word(x);
            ++frames;check(owner.revision()==revision&&!files.whole&&!files.writes,"native join never changes inventory or writes files");
            if(frame==67&&scenario!=4)check(page.selector_phase()==StoryForcePhase::Active,"held early A not replayed after reveal/opening");
            if(event.destination!=StoryForceNativeChoiceKind::None){StoryForceNativeChoice duplicate;check(page.take_choice(owner,&result)&&result.current(owner)&&result.facts()==f&&result.port()==port&&!page.take_choice(owner,&duplicate),"context-bound one-shot native choice");
                const auto expected=scenario==2?StoryForceNativeChoiceKind::Back:scenario==3?StoryForceNativeChoiceKind::Editor:scenario==4?StoryForceNativeChoiceKind::Bypass:StoryForceNativeChoiceKind::Stored;check(result.kind()==expected,"source result kind");
                if(expected==StoryForceNativeChoiceKind::Stored)check(result.source_result()==requested+1&&result.force()==requested&&owner.current(result.stored())&&!result.stored().members().empty(),"real stored candidate with current context");
                if(expected==StoryForceNativeChoiceKind::Back)check(result.source_result()==0xffffffffu&&result.force()==kForceCount&&result.stored().members().empty(),"Back does not publish a replacement force");
                if(expected==StoryForceNativeChoiceKind::Editor){check(result.source_result()==0x8001u+requested&&result.editor().current(owner),"editor source result");ForceSupplement names;std::array<std::string,10> digits;for(unsigned i=0;i<10;++i)digits[i]=std::to_string(i);check(names.assign(catalog,{},"PART ",{"N","A","G","S","C","B"},digits,{"A"},&error),"editor names");Save save;ForceInventoryStore store(save);StoryForceEditorBridge editor;check(editor.begin(result.editor(),owner,names,store)&&editor.step(owner,{},0,0x80)&&editor.session(owner)->view()==ForceEditorView::Selection&&!save.calls,"genuine editor entry/return view, no native save");check(!result.current(owner)&&result.editor().valid(owner),"UI choice revision stale after edit; claimed request still live");page.close();check(!result.editor().valid(owner)&&!editor.session(owner),"closing native owner invalidates outstanding editor request");}
                else{page.close();check(!result.current(owner),"native owner close invalidates UI choice");}
                break;
            }
        }
        //Re-run no native callback: delivery was already consumed above. Service
        //requirements are counted, not acknowledged audio/pool operations.
        check(scenario==4?(files.ranges==0&&stops==0&&starts==0&&music_fades==0):(files.ranges==8&&stops==1&&starts==1&&music_fades==1),"exact reads/service requests no fake completion");
        render.shutdown();
    }
    check(std::fclose(file)==0,"native wire close");std::printf("Story native source timelines: %u callbacks PASS; bounded I/O and owned fade, not campaign/battle authority\n",frames);
}
}
int main(int argc,char** argv){owned();if(argc!=1)source(argc,argv);return 0;}
