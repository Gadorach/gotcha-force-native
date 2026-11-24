#include <cstdio>
#include <cstdlib>
#include "tests/unit/scene_archive_pool_fixture.hpp"
namespace {unsigned checks=0;void check(bool b,const char* why){++checks;if(!b){std::fprintf(stderr,"preparation resource order: %s\n",why);std::abort();}}
struct Scene {
    std::shared_ptr<gf::SceneDescriptorPool> descriptors=scene_object_fixture::owner(256);
    std::shared_ptr<gf::SceneObjectPool> objects=std::make_shared<gf::SceneObjectPool>();
    gf::SceneArchivePool archives;
    bool open(std::string* error){return objects->begin_fresh(scene_object_fixture::objects(64),descriptors,error)&&archives.begin_fresh(scene_archive_fixture::profile(4),descriptors,error);}
};}
int main(){
    using namespace gf;std::string error;const auto bytes=scene_archive_fixture::bank();const auto raw=preparation_model_fixture::bytes(bytes);
    BattlePreparationModelProfile profile;check(profile.load({raw.data(),raw.size()},&error),"owned bank profile");
    Scene scene;check(scene.open(&error),"one descriptor domain");preparation_model_fixture::Files files;files.data=bytes;
    const auto old_objects=*scene.objects->snapshot(),unused_objects=old_objects;const auto old_models=*scene.descriptors->snapshot();
    BattlePreparationModels models;BattlePreparationModelProfile missing;
    check(!models.begin_resources(missing,files,"owned",scene.archives,&error)&&!files.stats&&!files.reads,"invalid profile does not claim/read");
    check(models.begin_resources(profile,files,"owned",scene.archives,&error)&&models.resources_current()&&!models.current()&&models.bank()&&!models.instances()&&models.bindings().empty(),"resource-first binding does not mint widgets or root copies");
    check(scene.archives.snapshot()->free_count==3&&*scene.objects->snapshot()==unused_objects&&*scene.descriptors->snapshot()==old_models&&files.reads==1&&files.stats==1,"only actual archive pool consumed");
    auto bank=*models.bank();check(!models.archive_pool1_join_pending()&&!models.advance_archive_binding(scene.archives,&error),"already bound resource cannot allocate twice");
    Scene foreign;check(foreign.open(&error),"foreign domain");BattlePreparationWidgets foreign_widgets;check(foreign_widgets.begin(preparation_widget_fixture::profile(),foreign.objects,&error),"foreign actual widgets");
    check(!models.prepare_widget_instances(foreign_widgets,&error)&&models.resources_current()&&!models.current(),"foreign widget domain cannot borrow briefing authority");
    BattlePreparationWidgets widgets;check(widgets.begin(preparation_widget_fixture::profile(),scene.objects,&error),"widgets allocated after briefing");const auto objects=*scene.objects->snapshot();const auto descriptors=*scene.descriptors->snapshot();
    check(models.prepare_widget_instances(widgets,&error)&&models.current()&&models.resources_current()&&models.instances()->instances()==38&&models.bank()->same_source(bank),"later component normal instances reuse same bank and archive");
    check(files.reads==1&&*scene.objects->snapshot()==objects&&*scene.descriptors->snapshot()==descriptors&&scene.archives.snapshot()->free_count==3,"no second read, decode, archive or object allocation");
    check(!models.prepare_widget_instances(widgets,&error),"one widget-model claim");
    const auto receipt=models.instances()->receipt(0);BattlePreparationModels moved=std::move(models);check(moved.current()&&!models.resources_current()&&receipt.current(),"stable owner move");
    widgets.retire();check(!moved.current()&&!moved.resources_current()&&!moved.bank()&&!moved.instances(),"joined models keep widget lifetime restriction");
    for(unsigned route=0;route<6;++route){
        Scene s;check(s.open(&error),"failure domain");preparation_model_fixture::Files f;f.data=bytes;BattlePreparationModels owner,destination;
        if(route==0)f.data.back()^=1;
        if(route==1)f.on_read=[&](){owner.retire();};
        if(route==2)f.on_read=[&](){s.archives.retire();};
        if(route==3)f.on_read=[&](){destination=std::move(owner);};
        if(route==4){for(unsigned i=0;i<4;++i){SceneArchiveLease lease;check(s.archives.acquire(&lease,&error)==SceneArchiveAcquire::Acquired,"actual exhaustion");}}
        if(route==5)f.wrong_size=true;
        check(!owner.begin_resources(profile,f,"owned",s.archives,&error)&&!owner.resources_current()&&!owner.bank()&&!destination.resources_current(),"corrupt/self-retire/pool-retire/move/exhaustion/size refusal");
        const auto reads=f.reads;check(!owner.begin_resources(profile,f,"owned",s.archives,&error)&&f.reads==reads&&!f.writes,"failed read not replayed");
    }
    preparation_model_fixture::Widgets existing;check(existing.open(&error),"legacy component widgets");BattlePreparationModels retiring;preparation_model_fixture::Files reentrant;reentrant.data=bytes;reentrant.on_read=[&](){retiring.retire();};
    check(!retiring.begin(existing.widgets,profile,reentrant,"owned",&error)&&!retiring.current(),"legacy entry shares self-retirement-safe loader");
    std::printf("preparation resource order: %u checks PASS\n",checks);
}
