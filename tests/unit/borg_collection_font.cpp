#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/borg_installed_browser_view.hpp"
#include "platform/pc/portable_font_load.hpp"

namespace {
using namespace gf;
namespace fs=std::filesystem;
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Collection font: %s\n",why);std::abort();}
}
struct Temp {
    fs::path root=fs::canonical(fs::temp_directory_path())/
        ("gf-collection-font-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Temp(){check(fs::create_directory(root),"create new fixture directory");}
    ~Temp(){std::error_code ec;fs::remove_all(root,ec);}
};
void write(const fs::path& file,std::string_view data){
    std::ofstream out(file,std::ios::binary);check(bool(out.write(data.data(),std::streamsize(data.size()))),"write fixture");
}
constexpr std::string_view kManifest=
    "GFFONT 1\natlas=art/face.a8\nsize=8|8\nline-height=7\nascent=5\n"
    "glyph=U+0020|0|0|0|0|0|0|2\n"
    "glyph=U+FFFD|0|0|2|3|0|3|3\n"
    "glyph=U+00E9|4|0|2|5|0|7|3\n";
class RecordingRender final:public IRenderBackend {
public:
    std::vector<Vertex> vertices;
    std::vector<DrawCall> calls;
    unsigned uploads=0;
    bool initialize(const RenderConfig&,IWindowBackend*,std::string*)override{return true;}
    void shutdown()override{}
    bool upload_vertices(const Vertex* v,std::size_t count)override{
        if(count)vertices.assign(v,v+count);else vertices.clear();
        return true;
    }
    bool upload_texture(u32 id,const TextureDesc& desc)override{
        check(id==71&&desc.width==8&&desc.height==8,"exact chosen atlas reaches the shared texture path");++uploads;return true;
    }
    bool upload_matrix(u32,const f32*)override{return true;}
    void begin_frame(const FrameDesc&)override{vertices.clear();calls.clear();}
    void submit(const DrawCall& call)override{calls.push_back(call);}
    void end_frame()override{}
    void present()override{}
};
}
int main(){
    Temp temp;
    const auto face=temp.root/"font";
    fs::create_directories(face/"art");
    const auto manifest=face/"font.manifest",atlas=face/"art/face.a8";
    RecordingRender device;RenderEngine render(device);PortableFont font;
    std::string error;
    check(render.initialize({},nullptr,&error),"initialize renderer");
    auto load=[&](const fs::path& path){return pc::load_local_portable_font(path,render,71,&font,&error);};
    check(!load({})&&!load(temp.root/"missing")&&!load(face)&&!device.uploads,"no implicit font search");
    write(manifest,kManifest);
    write(atlas,std::string(63,'\xFF'));
    check(!load(face)&&!device.uploads,"short atlas refused before GPU upload");
    write(atlas,std::string(65,'\xFF'));
    check(!load(face)&&!device.uploads,"long atlas refused before GPU upload");
    write(atlas,std::string(64,'\xFF'));
    check(load(face/"")&&font.available(render)&&font.metrics(render)->ink_top()==-2&&font.metrics(render)->ink_height()==7&&
        device.uploads==1,"explicit font loads with bounded overhang and a trailing path separator");
    check(!load(face/".."/"font")&&font.available(render),"reject traversal without touching a prior binding");
    std::error_code ec;
    fs::create_directory_symlink(face,temp.root/"link",ec);
    if(!ec)check(!load(temp.root/"link")&&!load(temp.root/"link"/"art"),"reject selected root links and linked ancestors");
    fs::rename(atlas,face/"elsewhere.a8");
    fs::create_symlink(face/"elsewhere.a8",atlas,ec);
    if(!ec)check(!load(face),"reject symlink atlas");
    fs::remove(atlas);
    fs::rename(manifest,face/"elsewhere.manifest");
    fs::create_symlink(face/"elsewhere.manifest",manifest,ec);
    if(!ec)check(!load(face),"reject symlink manifest");
    check(device.uploads==1&&font.available(render),"loaded font remains pinned without filesystem assets");

    BorgPackageManifest package;
    check(borg_package_read_manifest("GFBORG 2\nid=example:font\nname=Caf\xC3\xA9\ncatalog=card.txt\n",&package),"package with UTF-8 label");
    BorgPackageCatalog catalog{BorgPackageScanStatus::Complete,{{"folder",package}},{}};
    BorgCatalogSnapshot snapshot;snapshot.package=catalog.discovered.front();
    check(borg_catalog_read_profile("GFBORG-CATALOG 1\ndescription=Unicode test.\ntribe=none\n"
        "default-variant=normal\nvariant=normal|Normal|1|placeholder\n",&snapshot.profile),"fixture catalogue");
    BorgInstalledBrowser browser;
    check(browser.open_collection(catalog,{snapshot}),"open read-only Collection");
    BorgInstalledBrowserView view;
    auto draw=[&](const PortableFont* selected,bool cheat=false,const BorgPlacementPanel* placement=nullptr){
        render.begin_frame({});render.reset_arena();
        check(view.draw(render,640,480,browser,{},false,false,{},placement,nullptr,nullptr,cheat,selected)>0&&
            render.end_frame(),"shared browser renderer drew");
        bool used=false,unicode=false;
        for(const auto& call:device.calls)if(call.texture==71){
            used=true;
            for(std::size_t i=call.vertex_first;i<std::size_t(call.vertex_first)+call.vertex_count;++i)
                if(device.vertices[i].u==.5f&&device.vertices[i].v==0)unicode=true;
        }
        return std::pair{used,unicode};
    };
    check(draw(&font)==std::pair{true,true},"installed names reach the dynamic path without ASCII substitution");
    const PackageBorgRoster owned{{{9,{"example:font","Saved Caf\xC3\xA9"},false,SavedPackageVariant{"normal","Saved Variant"}}},{}};
    check(browser.open_collection(catalog,{snapshot},{},&owned)&&draw(&font)==std::pair{true,true},
        "owned Collection also draws saved UTF-8 text");
    const BorgPlacementPanel placement;
    check(draw(nullptr)==std::pair{false,false}&&draw(&font,true)==std::pair{false,false}&&
        draw(&font,false,&placement)==std::pair{false,false},"fallback, cheat and arrangement do not bind the atlas");
    std::puts("Collection font: explicit bounded local assets, UTF-8 rows and unchanged sibling modes PASS");
}
