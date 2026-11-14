#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/battle_character_binding.hpp"
#include "tests/unit/character_model_fixture.hpp"
#include "tests/unit/attachment_alternate_fixture.hpp"
namespace {
unsigned checks=0;void check(bool ok,const char* why){++checks;if(!ok){std::fprintf(stderr,"battle bank binding: %s\n",why);std::abort();}}
gf::ByteView view(const std::vector<gf::u8>& b){return {b.data(),b.size()};}
bool metadata_equal(const gf::HsdSkinAsset& a,const std::vector<gf::HsdSkinJoint>& j,const std::vector<gf::HsdSkinMesh>& m){
    if(a.joints.size()!=j.size()||a.meshes.size()!=m.size())return false;
    for(unsigned i=0;i<j.size();++i){if(a.joints[i].has_inverse!=j[i].has_inverse)return false;for(unsigned k=0;k<16;++k)if(gf::f32_to_bits(a.joints[i].inverse.m[k])!=gf::f32_to_bits(j[i].inverse.m[k]))return false;}
    for(unsigned i=0;i<m.size();++i){if(a.meshes[i].kind!=m[i].kind||a.meshes[i].palettes.size()!=m[i].palettes.size())return false;for(unsigned k=0;k<m[i].palettes.size();++k){const auto& x=a.meshes[i].palettes[k].influences;const auto& y=m[i].palettes[k].influences;if(x.size()!=y.size())return false;for(unsigned n=0;n<x.size();++n)if(x[n].joint!=y[n].joint||gf::f32_to_bits(x[n].weight)!=gf::f32_to_bits(y[n].weight))return false;}}
    return true;
}
}
int main(){
    using namespace gf;auto bytes=character_model_fixture::archive(),prefix=character_model_fixture::prefix(),pose=character_model_fixture::profile();std::string error;
    HsdRecordAnimationBank bank;check(bank.load(view(bytes),&error,true),"owned decoded bank");CharacterModelScene legacy,shared;check(legacy.load(view(bytes),view(prefix),view(pose),&error)&&shared.load_with_record_bank(bank,view(prefix),view(pose),&error),"old and explicit shared model loaders");
    check(!legacy.ready()&&!shared.ready()&&legacy.instance_binding()!=shared.instance_binding()&&legacy.instance_binding()->model_hash==shared.instance_binding()->model_hash,"preparation has separate identities and no published world");
    const auto& a=legacy.asset();const auto& b=shared.asset();check(a.scene.meshes.size()==b.scene.meshes.size()&&a.scene.joints.size()==b.scene.joints.size()&&metadata_equal(a,b.joints,b.meshes),"same selected body/skin metadata kernel");
    HsdJointInstanceBank instances;const auto pbytes=attachment_alternate_fixture::profile();AttachmentAlternateProfile alternate;check(attachment_alternate_load(view(pbytes),&alternate,&error)&&instances.construct(bank,alternate,&error),"same bank constructs all states");
    check(instances.source_texture(0)&&!b.scene.textures.empty()&&instances.source_texture(0)->pixels().data()==b.scene.textures[0].pixels().data(),"body and instances share image storage");
    check(b.scene.meshes[0].deformation_required&&instances.source_mesh(0)->deformation_required,"source skin requirements remain deferred, not cleared for sharing");
    HsdScene scene;hsd_detail::Reader reader;check(hsd_load(view(bytes),&scene,&error)&&hsd_anim_detail::open_container(view(bytes),&reader,&error),"owned metadata input");std::vector<HsdSkinJoint> joints;std::vector<HsdSkinMesh> meshes;HsdSkinAsset full;check(hsd_skin_load(view(bytes),&full,&error)&&hsd_skin_metadata(scene,reader,&joints,&meshes,&error)&&metadata_equal(full,joints,meshes),"extracted metadata equals legacy owned path");
    auto bad=scene;bad.mesh_joint[0]=kHsdNoJoint;check(!hsd_skin_metadata(bad,reader,&joints,&meshes,&error)&&joints.empty()&&meshes.empty(),"bad owner clears metadata outputs");
    bad=scene;bad.joints.push_back(bad.joints[0]);check(!hsd_skin_metadata(bad,reader,&joints,&meshes,&error)&&joints.empty()&&meshes.empty(),"ambiguous identity still refuses");
    bad=scene;bad.joints[bad.mesh_joint[0]].flags&=~2u;check(!hsd_skin_metadata(bad,reader,&joints,&meshes,&error)&&joints.empty()&&meshes.empty(),"non-root envelope remains unsupported");
    auto old_identity=shared.instance_binding();check(!shared.load_with_record_bank(HsdRecordAnimationBank{},view(prefix),view(pose),&error)&&!shared.ready()&&!shared.instance_binding()&&old_identity,"invalid bank clears old model despite external identity keepalive");
    check(shared.load_with_record_bank(bank,view(prefix),view(pose),&error),"explicit fresh source reload");bank={};bytes.clear();check(!shared.ready()&&!shared.asset().scene.textures.empty()&&instances.source_texture(0)->pixels().data()==shared.asset().scene.textures[0].pixels().data(),"body/instances survive original loader and bytes without world publication");
    BattleCharacterBinding unbound;const std::array<const ForceInventory*,2> owners{};check(!unbound.bank_instances()&&!unbound.bank_material_context()&&!unbound.bank_instance_receipt(0).current(owners)&&unbound.pending().all_bank_instances,"unbound bank owner closed");
    ChallengeCharacterDescriptorReceipt fake;BattleNormalClassProfile classes;check(!unbound.bind_with_bank_instances(fake,owners,classes,{},{},{},alternate,&error)&&!unbound.prepared(),"fake allocation cannot construct native-bound instances");check(!unbound.bind_declared_with_bank_instances({},classes,{},{},{},alternate,&error),"one-shot failed native owner cannot retry as declared");
    BattleCharacterBinding declared;check(!declared.bind_declared_with_bank_instances({},classes,{},{},{},alternate,&error)&&!declared.prepared()&&!declared.bank_instances(),"empty declared seam never grants native instance authority");
    unbound.retire();check(!unbound.bank_instance_receipt(0).current(owners)&&!unbound.bank_material_context(),"closed retirement");
    std::printf("battle bank binding: %u owned checks PASS\n",checks);
}
