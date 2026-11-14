#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <limits>
#include "engine/gameplay/story_force_card_scene.hpp"
#include "engine/gameplay/frame_digest.hpp"
#include "tests/unit/force_inventory_fixture.hpp"
#include "tests/unit/story_force_cards_fixture.hpp"
namespace {
using namespace gf;unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"Story cards: %s\n",why);std::abort();}}
std::vector<u8> read(const char* path){std::ifstream f(path,std::ios::binary);check(bool(f),"input open");return {std::istreambuf_iterator<char>(f),{}};}
void owned(){auto art=story_force_cards_fixture::art();const auto& layout=art.layout();SourceForceCardMember m;
    check(art.ready()&&art.bank_hash()==7,"owned art");
    for(unsigned count=1;count<=30;++count)for(unsigned slot=0;slot<30;++slot){check(source_force_card_slot(layout,count,slot,30,&m),"physical slot accepts holes");const auto row=count<=8?0u:unsigned(slot>=8)+unsigned(slot>=15)+unsigned(slot>=23);check(m.row==row&&m.source_rgb==255-unsigned(25.5f*row)&&m.alpha==1,"8/7/8/7 row and colour");check(source_force_card_slot(layout,count,slot,0,&m)&&m.source_rgb==0&&m.alpha==.25f,"rejected appearance");}
    SourceForceCardsArt::Format format;for(unsigned count:{1u,15u,16u,23u,24u,30u})check(source_force_card_format(layout,count,false,&format)&&format.record==unsigned(count>=16)+unsigned(count>=24),"format cuts separate from layout rows");
    check(!source_force_card_slot(layout,0,0,0,&m)&&!source_force_card_slot(layout,31,0,0,&m)&&!source_force_card_slot(layout,1,30,0,&m)&&!source_force_card_slot(layout,1,0,31,&m),"layout bounds");auto bad=layout;bad.row_span=1000000;check(!source_force_card_slot(bad,30,29,30,&m),"float-to-index budget guard");bad=layout;bad.single_row[1][0]=std::numeric_limits<f32>::quiet_NaN();check(!source_force_card_slot(bad,1,0,0,&m),"nonfinite source profile");
    auto a=story_force_cards_fixture::cards(),b=story_force_cards_fixture::presentation();std::string error;StoryForceCardArt decoded;
    check(decoded.load({a.data(),a.size()},{b.data(),b.size()},&error),"profile load");b[11]^=1;check(!decoded.load({a.data(),a.size()},{b.data(),b.size()},&error)&&!decoded.ready(),"foreign presentation bank clears art");b=story_force_cards_fixture::presentation();b[83]=1;check(!decoded.load({a.data(),a.size()},{b.data(),b.size()},&error),"reserved schema");b=story_force_cards_fixture::presentation();store_be_f32(b.data()+48,35);check(!decoded.load({a.data(),a.size()},{b.data(),b.size()},&error),"Story start35 not card41");
    const auto lib=force_inventory_fixture::catalog();ForceInventory owner;check(owner.create(lib)&&owner.place(19,29,2)==ForceChange::Applied,"holes slot20 fixture");const auto revision=owner.revision();StoryForceCardPlan plan;
    check(story_force_card_plan(art,owner,19,75,&plan)&&plan.current(owner)&&plan.draws().size()==2&&plan.draws()[0].source_slot==29&&plan.draws()[0].box_slot==2&&plan.draws()[0].model==0x1003,"physical source slot/provenance");check(plan.draws()[0].mask==0x45&&plan.draws()[1].mask==1&&plan.prefix()==30,"source draw masks and prefix");
    check(story_force_card_plan(art,owner,19,0,&plan)&&plan.prefix()==-1&&plan.draws()[0].icon_appearance.alpha==.25f&&plan.draws()[0].icon_appearance.multiply==std::array<u8,3>{0,0,0},"signed display prefix -1 is not confirmation semantics");ForceBattleRoster roster;check(owner.battle_roster(19,0,ForceCostMode::Full,&roster)&&roster.members().size()==1,"same zero budget confirms unlimited despite card colour");
    const auto old_tag=plan.art_tag();check(!story_force_card_plan(art,owner,1,75,&plan)&&plan.art_tag()==old_tag&&plan.current(owner),"invalid empty candidate leaves old output unchanged");check(owner.revision()==revision,"render planning does not mutate inventory");ForceInventory foreign;check(foreign.create(lib)&&!plan.current(foreign),"foreign plan");check(owner.rename_force(19,"NEW")==ForceChange::Applied&&!plan.current(owner),"stale plan");
    HsdRecordAnimationBank empty;ForceEditorArt mapping;StoryForceCardScene scene;check(!scene.load_prepared(art,empty,empty,mapping,&error)&&!scene.ready(),"missing banks refuse no fake drawing");
    std::printf("Story cards: %u owned checks PASS; no selector/battle authority\n",checks);
}
void corpus(const StoryForceCardArt& art,const char* path){FILE* file=std::fopen(path,"wx");check(file,"exclusive layout test wire");unsigned rows=0;
    auto word=[&](u32 x){u8 b[4];store_be32(b,x);check(std::fwrite(b,1,4,file)==4,"wire write");};
    for(unsigned count=1;count<=30;++count)for(unsigned slot=0;slot<30;++slot)for(unsigned prefix=0;prefix<=30;++prefix){
        SourceForceCardMember m;check(source_force_card_slot(art.layout(),count,slot,prefix,&m),"source numeric layout");
        for(f32 f:m.position)word(f32_to_bits(f));
        for(f32 f:m.scale)word(f32_to_bits(f));
        word(m.row);word(m.source_rgb);word(f32_to_bits(m.alpha));++rows;
    }
    check(std::fclose(file)==0&&rows==27900,"corpus close");std::printf("source numeric rows %u test bytes %u\n",rows,rows*36);
}
void source(int argc,char** argv){check(argc==8,"source arguments");const auto cards=read(argv[1]),presentation=read(argv[2]),bank_bytes=read(argv[3]),icons_bytes=read(argv[4]),catalog_bytes=read(argv[5]),mapping_bytes=read(argv[6]);
    std::string error;StoryForceCardArt art;check(art.load({cards.data(),cards.size()},{presentation.data(),presentation.size()},&error),error.c_str());corpus(art,argv[7]);
    HsdRecordAnimationBank bank,icons;check(bank.load({bank_bytes.data(),bank_bytes.size()},&error),error.c_str());check(icons.load({icons_bytes.data(),icons_bytes.size()},&error),error.c_str());ForceCatalog catalog;check(catalog.load({catalog_bytes.data(),catalog_bytes.size()},&error),error.c_str());ForceEditorArt mapping;check(mapping.load({mapping_bytes.data(),mapping_bytes.size()},catalog,&error),error.c_str());
    RecordingRenderBackend backend;RenderEngine render(backend);check(render.initialize({640,480,false,true,"Story cards CPU"},nullptr,&error),"recording render");StoryForceCardScene scene;check(scene.load_prepared(art,bank,icons,mapping,&error),error.c_str());
    for(unsigned count:{1u,8u,9u,15u,16u,23u,24u,30u}){ForceInventory owner;check(owner.create(catalog,false),"source catalog owner");for(unsigned i=0;i<count;++i){ForceBorg borg;borg.model=s16(catalog.entries()[i].model);check(owner.acquire(borg)==ForceChange::Applied&&owner.place(19,i,i)==ForceChange::Applied,"declared icon rows not actors");}
        const auto revision=owner.revision();for(s32 energy:{0,300,100000}){StoryForceCardPlan plan;check(story_force_card_plan(art,owner,19,energy,&plan),"source card plan");render.begin_frame({0,0,0,0});render.reset_arena();render.state()={};check(scene.draw(render,plan,owner,&error),error.c_str());check(render.end_frame(),"nonempty submission");render.present();check(backend.draws()>0&&backend.last_frame_digest()!=0,"recorded after present");std::printf("D %u %d %d %u %llu %zu\n",count,energy,plan.prefix(),backend.draws(),static_cast<unsigned long long>(backend.last_frame_digest()),scene.models());check(owner.revision()==revision,"draw did not mutate inventory");}
    }
    render.shutdown();std::printf("source shared card/icon CPU submissions PASS; no GPU/complete page claim\n");
}
}
int main(int argc,char** argv){owned();if(argc!=1)source(argc,argv);return 0;}
