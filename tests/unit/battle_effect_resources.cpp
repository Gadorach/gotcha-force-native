#include <cstdio>
#include <cstdlib>
#include <functional>
#include "engine/gameplay/battle_effect_resources.hpp"
namespace {unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"effect resources: %s\n",why);std::abort();}}
struct Files:gf::IFileSystem {
    std::vector<gf::u8> data;unsigned reads=0;std::function<void()> stat_action,read_action;
    bool file_size(const std::string&,gf::u64* n)override{*n=data.size();if(stat_action)stat_action();return true;}
    bool read(const std::string&,std::vector<gf::u8>& out)override{++reads;out=data;if(read_action)read_action();return true;}
    bool write(const std::string&,const gf::u8*,std::size_t)override{return false;}bool erase(const std::string&)override{return false;}bool exists(const std::string&)override{return false;}
};
gf::BattleEffectResourceProfile profile(const std::vector<gf::u8>& data){
    using namespace gf;std::array<u8,64> bytes{};const std::array<u32,16> words{0x47464550,1,64,697,u32(data.size()),challenge_payload_hash({data.data(),data.size()}),96,36,40,0x7a3c00,0x7ac400,0x8008bed8,4,1,0,0};for(unsigned i=0;i<16;++i)store_be32(bytes.data()+4*i,words[i]);BattleEffectResourceProfile p;std::string e;check(p.load({bytes.data(),bytes.size()},&e),"owned three-block recipe");return p;
}}
int main(){
    using namespace gf;std::string error;Files files;files.data.resize(40+96+36+40);auto put=[&](unsigned at,u32 value){store_be32(files.data.data()+at,value);};
    put(0,0x47464e42);put(4,1);put(8,3);put(12,u32(files.data.size()-16));for(unsigned i=0;i<3;++i){put(16+8*i,i);put(20+8*i,std::array<u32,3>{96,36,40}[i]);}
    put(40,kTplMagic);put(44,1);put(48,12);put(52,20);put(60,0x00040004);put(64,4);put(68,64);
    for(unsigned i=0;i<16;++i){store_be16(files.data.data()+104+2*i,0xf800);}
    put(136,32);put(172,32);const auto original=files.data;const auto recipe=profile(original);
    for(u8 mode:{u8(0),u8(1)}){
        BattleEffectResources effects;const auto reads=files.reads;
        check(effects.begin(recipe,mode,"owned",&error)&&effects.current()&&files.reads==reads&&!effects.texture()&&!effects.model_input().data,"enqueue owns destinations, no I/O/texture preparation");
        check(effects.selected_model_block()==(mode?2:1)&&effects.cells()->at(mode?1:2).kind==SceneResourceCellKind::Skip&&!effects.bind_textures(&error),"unselected model skipped; callback cannot precede load");
        check(effects.load(files,&error)&&files.reads==reads+1&&effects.model_input().size==32&&!effects.texture()&&!effects.archive_record(),"actual byte ownership before independent texture callback");
        files.data[104]=0;check(effects.bind_textures(&error)&&effects.texture()->image.width==4&&effects.texture()->image.height==4&&effects.texture()->image.format==4,"shared TPL decoder reads owned bytes");
        const auto* texture=effects.texture();for(unsigned i=0;i<16;++i)check(std::array<u8,4>{texture->image.rgba[4*i],texture->image.rgba[4*i+1],texture->image.rgba[4*i+2],texture->image.rgba[4*i+3]}==std::array<u8,4>{255,0,0,255},"RGB565 red pixels");
        check(texture->source_pixels.size==32&&!texture->source_palette.data&&texture->wrap_s==0&&texture->wrap_t==0&&texture->min_filter==1&&texture->mag_filter==1&&!texture->mipmaps&&texture->min_lod==0&&texture->max_lod==0,"source-created texture descriptor fields");
        check(!effects.load(files,&error)&&!effects.bind_textures(&error)&&files.reads==reads+1,"no read/callback replay");
        BattleEffectResources moved=std::move(effects);check(moved.texture()==texture&&!effects.current(),"stable texture owner move");moved.retire();check(!moved.current()&&!moved.texture(),"retirement closes texture views");files.data=original;
    }
    for(unsigned route=0;route<4;++route){BattleEffectResources effects,moved;check(effects.begin(recipe,0,"owned",&error),"fresh refusal case");const auto reads=files.reads;
        if(route==0){files.data.back()^=1;}if(route==1){files.stat_action=[&](){effects.retire();};}if(route==2){files.read_action=[&](){effects.retire();};}if(route==3){files.read_action=[&](){moved=std::move(effects);};}
        check(!effects.load(files,&error)&&!effects.current()&&!moved.current()&&files.reads==reads+(route==1?0u:1u),"corruption/stat-retire/read-retire/move refusal");
        files.stat_action={};files.read_action={};files.data=original;
    }
    put(44,2);BattleEffectResources too_many;check(too_many.begin(profile(files.data),0,"owned",&error)&&too_many.load(files,&error)&&!too_many.bind_textures(&error)&&!too_many.texture(),"source one-image destination capacity remains guarded");
    std::printf("battle effect resources: %u checks PASS\n",checks);
}
