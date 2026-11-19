#include <chrono>
#include <cstdio>
#include <cstdlib>
#include "platform/pc/borg_catalog_load.hpp"

namespace {
namespace fs=std::filesystem;
void check(bool ok,const char* why){
    if(!ok){std::fprintf(stderr,"Borg package: %s\n",why);std::abort();}
}
struct Temp {
    fs::path path;
    Temp(){
        path=fs::temp_directory_path()/("gf-borg-package-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        check(fs::create_directory(path),"create owned temporary folder");
    }
    ~Temp(){std::error_code ignored;fs::remove_all(path,ignored);}
};
void write(const fs::path& path,std::string_view text){
    std::ofstream file(path,std::ios::binary);
    check(bool(file.write(text.data(),std::streamsize(text.size()))),"write test fixture");
}
std::string manifest(std::string_view id,std::string_view name){
    return "GFBORG 1\nid="+std::string(id)+"\nname="+std::string(name)+
        "\nmodel=visual/model.glb\nanimations=visual/animations.glb\nrules=rules/borg.rules\n";
}
void package(const fs::path& folder,std::string_view id,std::string_view name,bool with_map=false){
    fs::create_directories(folder/"visual");
    fs::create_directories(folder/"rules");
    auto text=manifest(id,name);
    if(with_map){text+="animation-map=rules/movement.map\n";write(folder/"rules/movement.map","test");}
    write(folder/"borg.manifest",text);
    for(auto name:{"visual/model.glb","visual/animations.glb","rules/borg.rules"})write(folder/name,"test");
}
std::vector<gf::u8> editor_icon(gf::u16 width){
    using namespace gf;
    std::vector<u8> bytes(56+(width/4)*64,0);
    store_be32(bytes.data(),kTplMagic);store_be32(bytes.data()+4,1);store_be32(bytes.data()+8,12);
    store_be32(bytes.data()+12,20);store_be16(bytes.data()+20,4);store_be16(bytes.data()+22,width);
    store_be32(bytes.data()+24,6);store_be32(bytes.data()+28,56);
    for(std::size_t block=56;block<bytes.size();block+=64){
        for(unsigned i=0;i<16;++i){
            bytes[block+2*i]=255;bytes[block+2*i+1]=210;
            bytes[block+32+2*i]=80;bytes[block+33+2*i]=32;
        }
    }
    return bytes;
}
}

int main(){
    using namespace gf;
    BorgPackageManifest parsed;
    const auto valid=manifest("test:crane","Crane \xc3\x89lite");
    check(borg_package_read_manifest(valid,&parsed)&&parsed.identity.name=="Crane \xc3\x89lite"&&parsed.animation_map.empty(),
        "valid UTF-8 name and optional animation map");
    for(auto bad:{"../x","/x","C:/x","visual//x","visual/./x","visual\\x","visual/x."}){
        check(!borg_package_asset_path_valid(bad),"paths cannot escape or alias relative components");
    }
    for(auto bad:{"x","test:","test:bad:id","Test:crane","retail:g-red","test:.."}){
        check(!borg_package_id_valid(bad),"malformed or reserved ID refuses");
    }
    for(const auto& malformed:{valid+"model=other.glb\n",valid+"unknown=value\n",
                                manifest("test:crane","bad\x1f"),std::string(kBorgManifestBytes+1,'x')}){
        check(!borg_package_read_manifest(malformed,&parsed)&&parsed.identity.id=="test:crane",
            "invalid manifest leaves the prior result untouched");
    }
    Temp temp;const auto root=temp.path/"borgs";
    check(pc::scan_borg_packages(root).status==BorgPackageScanStatus::NoDirectory,"absent package folder is not a failed scan");
    package(root/"Fancy folder","test:crane","Crane",true);
    package(root/"Other","test:ant","Ant");
    package(root/"Broken","test:missing","Unavailable");
    fs::remove(root/"Broken/visual/animations.glb");
    auto catalog=pc::scan_borg_packages(root);
    check(catalog.status==BorgPackageScanStatus::Complete&&catalog.discovered.size()==2&&
        catalog.discovered.front().manifest.identity.id=="test:ant"&&catalog.issues.size()==1&&
        catalog.issues.front().kind==BorgPackageIssueKind::MissingAsset,"sorted discovery excludes incomplete packages");
    const auto* crane=catalog.find("test:crane");check(crane!=nullptr,"look up a discovered Borg by ID");
    const auto saved=crane->manifest.identity;
    fs::rename(root/"Fancy folder",root/"New folder");
    write(root/"New folder/borg.manifest",manifest("test:crane","Renamed Crane"));
    catalog=pc::scan_borg_packages(root);
    check(catalog.find(saved.id)&&catalog.find(saved.id)->folder=="New folder"&&saved.name=="Crane"&&
        catalog.find(saved.id)->manifest.identity.name=="Renamed Crane","stable identity survives folder and display-name changes");
    package(root/"Copy","test:crane","Third name");
    catalog=pc::scan_borg_packages(root);
    check(!catalog.find(saved.id)&&catalog.issues.size()==3&&
        catalog.issues[1].kind==BorgPackageIssueKind::DuplicateId&&catalog.issues[2].kind==BorgPackageIssueKind::DuplicateId,
        "duplicate IDs quarantine both packages instead of choosing by directory order");
    fs::remove_all(root/"Copy");
    fs::remove(root/"New folder/borg.manifest");
    catalog=pc::scan_borg_packages(root);
    check(!catalog.find(saved.id)&&saved==BorgIdentity{"test:crane","Crane"},"the identity snapshot still has a name after files vanish");
    std::error_code error;
    fs::create_symlink(root/"Other/visual/model.glb",root/"Broken/visual/symlink.glb",error);
    if(!error){
        write(root/"Broken/visual/animations.glb","test");
        auto text=manifest("test:missing","Symlink");
        text.replace(text.find("model.glb"),9,"symlink.glb");
        write(root/"Broken/borg.manifest",text);
        check(!pc::scan_borg_packages(root).find("test:missing"),"asset symlinks do not count as package files");
    }
    check(pc::scan_borg_packages(root/"Other/borg.manifest").status==BorgPackageScanStatus::Unavailable,
        "non-directory root is distinct from missing");

    BorgCatalogCard card;std::string diagnostic;catalog=pc::scan_borg_packages(root);
    check(!pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&
        diagnostic=="no catalogue profile declared"&&catalog.find("test:ant"),"old manifest is discoverable but not catalogue-ready");
    const std::string profile="GFBORG-CATALOG 1\ndescription=A clockwork ant.\ntribe=test:machine\ndefault-variant=normal\n"
        "variant=normal|Standard|240|visual/ant.tpl\nvariant=bronze|Bronze|250|placeholder\n";
    BorgCatalogProfile metadata;
    check(borg_catalog_read_profile(profile,&metadata)&&metadata.find("bronze")&&metadata.find("bronze")->placeholder_icon(),
        "variant metadata retains explicit placeholder");
    for(std::string_view row:{"variant=normal|Duplicate|10|placeholder","variant=m|No cost|-1|placeholder",
          "variant=m|Large cost|65536|placeholder","variant=m|Bad path|5|../ant.tpl","variant=m|Wrong codec|0|ant.png",
          "description=Duplicate","unknown=value"}){
        check(!borg_catalog_read_profile(profile+std::string(row),&metadata)&&metadata.variants.size()==2,
            "unreviewed or duplicate catalogue data cannot replace the previously validated view");
    }
    auto broken_default=profile;broken_default.replace(broken_default.find("default-variant=normal"),22,"default-variant=absent");
    check(!borg_catalog_read_profile(broken_default,&metadata),"default variant must be a live row");
    write(root/"Other/borg.manifest",manifest("test:ant","Ant")+"catalog=catalog.txt\n");
    catalog=pc::scan_borg_packages(root);
    const auto unresolved=profile+"variant=tbd|Unknown art|100|unresolved\n";
    write(root/"Other/catalog.txt",unresolved);
    check(borg_catalog_read_profile(unresolved,&metadata)&&metadata.find("tbd")->unresolved_icon(),
        "an unresolved icon has a structural slot");
    auto bytes=editor_icon(4);
    write(root/"Other/visual/ant.tpl",{reinterpret_cast<const char*>(bytes.data()),bytes.size()});
    check(!pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&
        diagnostic=="editor icon is unresolved for variant tbd","unresolved artwork is not a deliberate placeholder");
    write(root/"Other/catalog.txt",profile);
    fs::remove(root/"Other/visual/ant.tpl");
    check(!pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&catalog.find("test:ant"),
        "missing icon blocks the catalogue view only");
    write(root/"Other/visual/ant.tpl",{reinterpret_cast<const char*>(bytes.data()),bytes.size()});
    check(pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&card.profile.default_variant=="normal"&&
        card.identity.id=="test:ant"&&card.icons.size()==2&&card.icons[0]&&card.icons[0]->width==4&&
        card.icons[0]->rgba[0]==210&&card.icons[0]->rgba[1]==80&&card.icons[0]->rgba[3]==255&&!card.icons[1],
        "actual editor tile decodes through shared texture code and one explicit placeholder remains");
    bytes=editor_icon(132);
    write(root/"Other/visual/ant.tpl",{reinterpret_cast<const char*>(bytes.data()),bytes.size()});
    check(!pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&card.icons[0]->width==4,
        "oversize tile refuses atomically instead of replacing a prior catalogue card");
    write(root/"Other/borg.manifest",manifest("test:other-ant","Not Ant")+"catalog=catalog.txt\n");
    check(!pc::load_borg_catalog_card(root,catalog,"test:ant",&card,&diagnostic)&&diagnostic=="package manifest changed; rescan required",
        "stale directory cannot load a card on behalf of a different ID");
    std::puts("Borg package: discovery, identity, catalogue variants and real editor icon PASS");
}
