#pragma once
// Source-qualified POBJ rigid/shared-joint/envelope palette metadata and CPU
// world-space evaluation. No renderer admission, clip selection or guest state.
#include <array>
#include <limits>
#include <optional>
#include <span>
#include "engine/assets/hsd_anim.hpp"
#include "engine/graphics/transform.hpp"
namespace gf {
enum class HsdSkinKind{Rigid,Shared,Envelope};
struct HsdSkinInfluence {u32 joint=0;f32 weight=0;};
struct HsdSkinPalette {std::vector<HsdSkinInfluence> influences;};
struct HsdSkinMesh {HsdSkinKind kind=HsdSkinKind::Rigid;std::vector<HsdSkinPalette> palettes;};
struct HsdSkinJoint {bool has_inverse=false;Mat4 inverse;};
struct HsdSkinAsset {HsdScene scene;std::vector<HsdSkinJoint> joints;std::vector<HsdSkinMesh> meshes;bool loaded=false;};
struct HsdSkinGeometry {std::vector<std::vector<HsdVertex>> world_vertices;};
namespace hsd_skin_detail {
inline constexpr unsigned kMaxSlots=10,kMaxInfluences=32;
inline constexpr std::size_t kMaxVertices=1u<<20,kMaxBindings=1u<<20;
inline bool matrix_valid(const Mat4& m){
    for(f32 v:m.m)if(!std::isfinite(v)||std::abs(v)>1e7f)return false;
    return m.m[12]==0&&m.m[13]==0&&m.m[14]==0&&m.m[15]==1;
}
inline bool read_matrix(const hsd_detail::Reader& r,u32 at,Mat4* out){
    if(!r.has(at,48))return false;
    Mat4 next;for(unsigned i=0;i<12;++i)next.m[i]=r.block.bef32(at+i*4);
    if(!matrix_valid(next))return false;
    *out=next;return true;
}
inline bool normal(const Mat4& m,const f32 in[3],f32 out[3]){
    f64 c[3][3];for(unsigned r=0;r<3;++r)for(unsigned s=0;s<3;++s){const unsigned r1=(r+1)%3,r2=(r+2)%3,s1=(s+1)%3,s2=(s+2)%3;
        c[r][s]=f64(m.at(r1,s1))*m.at(r2,s2)-f64(m.at(r1,s2))*m.at(r2,s1);}
    const f64 det=f64(m.at(0,0))*c[0][0]+f64(m.at(0,1))*c[0][1]+f64(m.at(0,2))*c[0][2];
    if(!std::isfinite(det)||std::abs(det)<1e-12)return false;
    f64 n[3];for(unsigned i=0;i<3;++i)n[i]=(c[i][0]*in[0]+c[i][1]*in[1]+c[i][2]*in[2])*(det<0?-1:1);
    const f64 length=std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);if(!std::isfinite(length))return false;
    for(unsigned i=0;i<3;++i)out[i]=length>0?f32(n[i]/length):0;
    return true;
}
}
//Same metadata parser for the legacy owned scene and an explicitly retained
//decoded bank. No second geometry/image decode on the bank-backed path.
inline bool hsd_skin_metadata(const HsdScene& scene,const hsd_detail::Reader& r,std::vector<HsdSkinJoint>* out_joints,std::vector<HsdSkinMesh>* out_meshes,std::string* error,
                              std::optional<u32> selected_record=std::nullopt){
    if(out_joints)out_joints->clear();
    if(out_meshes)out_meshes->clear();
    auto fail=[&](const char* why){if(error)*error=std::string("hsd skin: ")+why;return false;};
    if(!out_joints||!out_meshes)return fail("metadata destination");
    HsdSkinAsset next;
    if(scene.meshes.size()>kHsdMaxNodes)return fail("mesh metadata budget");
    if(selected_record&&*selected_record>=scene.model_roots.size())return fail("selected record missing");
    auto selected=[&](const HsdJoint& joint){return !selected_record||joint.model_root==*selected_record;};
    std::vector<std::pair<u32,u32>> joint_offsets;for(u32 i=0;i<scene.joints.size();++i)
        if(selected(scene.joints[i]))joint_offsets.push_back({scene.joints[i].archive_offset,i});
    std::sort(joint_offsets.begin(),joint_offsets.end());
    for(unsigned i=1;i<joint_offsets.size();++i)if(joint_offsets[i-1].first==joint_offsets[i].first)return fail("ambiguous joint identity");
    auto joint_index=[&](u32 offset,u32* result){const auto found=std::lower_bound(joint_offsets.begin(),joint_offsets.end(),std::pair<u32,u32>{offset,0});
        if(found==joint_offsets.end()||found->first!=offset)return false;
        *result=found->second;return true;};
    next.joints.resize(scene.joints.size());
    for(u32 i=0;i<next.joints.size();++i){const auto& j=scene.joints[i];u32 inverse=0;
        if(!selected(j))continue;
        u32 klass=0;if(!r.ptr(j.archive_offset,&klass)||klass)return fail("custom joint class requires separate matrix semantics");
        if(!r.ptr(j.archive_offset+0x38,&inverse))return fail("inverse-bind pointer lacks relocation");
        if(inverse){if(!hsd_skin_detail::read_matrix(r,inverse,&next.joints[i].inverse))return fail("inverse-bind matrix extent/domain");next.joints[i].has_inverse=true;}}
    next.meshes.resize(scene.meshes.size());std::size_t vertices=0,bindings=0;
    for(u32 i=0;i<next.meshes.size();++i){const auto& mesh=scene.meshes[i];auto& skin=next.meshes[i];const u32 kind=mesh.pobj_flags&0x3000;
        if(i>=scene.mesh_joint.size()||scene.mesh_joint[i]>=next.joints.size())return fail("mesh owner");
        const u32 owner=scene.mesh_joint[i];if(!selected(scene.joints[owner]))continue;
        u32 binding=0;if(!r.ptr(mesh.pobj_offset+0x14,&binding))return fail("POBJ binding lacks relocation");
        u32 klass=0;if(!r.ptr(mesh.pobj_offset,&klass)||klass)return fail("custom POBJ class requires separate matrix semantics");
        if(kind==0){skin.palettes.push_back({{{owner,1}}});
            if(binding){u32 shared=0;if(!joint_index(binding,&shared))return fail("shared joint outside published skeleton");skin.kind=HsdSkinKind::Shared;skin.palettes.push_back({{{shared,1}}});}
        }else if(kind==0x2000){
            skin.kind=HsdSkinKind::Envelope;
            // Original 80253274 returns no right-side correction ONLY for a
            // current SKELETON_ROOT (bit1). Non-root/ancestor correction remains
            // unsupported; ENVELOPE_MODEL bit2 alone is not a sufficient test.
            if(!binding||!(scene.joints[owner].flags&2))return fail("missing envelope or unsupported non-root envelope correction");
            bool ended=false;
            for(unsigned slot=0;slot<=hsd_skin_detail::kMaxSlots;++slot){u32 list=0;if(!r.ptr(binding+slot*4,&list))return fail("envelope table pointer/extent");
                if(!list){ended=true;break;}if(slot==hsd_skin_detail::kMaxSlots)return fail("palette budget");
                HsdSkinPalette palette;bool terminated=false;f64 sum=0;
                for(unsigned n=0;n<=hsd_skin_detail::kMaxInfluences;++n){u32 joint=0;if(!r.has(list+n*8,8)||!r.ptr(list+n*8,&joint))return fail("influence pointer/extent");
                    if(!joint){terminated=true;break;}if(n==hsd_skin_detail::kMaxInfluences)return fail("influence budget");
                    u32 index=0;const f32 weight=r.block.bef32(list+n*8+4);
                    if(!joint_index(joint,&index)||!std::isfinite(weight)||weight<0||weight>1)return fail("influence joint/weight domain");
                    if(++bindings>hsd_skin_detail::kMaxBindings)return fail("global influence-work budget");
                    palette.influences.push_back({index,weight});sum+=weight;}
                if(!terminated||palette.influences.empty()||std::abs(sum-1)>1e-4)return fail("unsupported non-unit/empty influence list");
                skin.palettes.push_back(std::move(palette));}
            if(!ended||skin.palettes.empty())return fail("unterminated/empty palette table");
        }else return fail("shape/unknown POBJ type requires separate evaluation");
        std::vector<hsd_detail::VtxDesc> descs;if(!hsd_detail::read_descriptors(r,r.block.be32(mesh.pobj_offset+8),&descs,error))return false;
        for(const auto& d:descs)if(d.attr<=8&&d.type!=0&&d.type!=1)return fail("unsupported non-direct matrix selector");
        vertices+=mesh.vertices.size();if(vertices>hsd_skin_detail::kMaxVertices)return fail("vertex budget");
        for(const auto& v:mesh.vertices){
            if(v.matrix_index%3||v.matrix_index/3>=skin.palettes.size())return fail("vertex palette row index");
            for(auto index:v.texture_matrix_indices)if(index!=60&&(index<30||index>=60||(index-30)%3||unsigned((index-30)/3)>=skin.palettes.size()))return fail("texture palette row index");
        }
    }
    *out_joints=std::move(next.joints);*out_meshes=std::move(next.meshes);return true;
}
// A selected-record result is preparation only. The scene owner must remap
// and publish a complete subset before passing it to hsd_skin_sample.
inline bool hsd_skin_load(ByteView archive,HsdSkinAsset* out,std::string* error,std::optional<u32> selected_record=std::nullopt,
                          HsdStaticMaterialPolicy material_policy=HsdStaticMaterialPolicy::Legacy){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd skin: ")+why;return false;};
    if(!out||!archive.data||archive.size<32||archive.size>(64u<<20))return fail("archive budget/destination");
    HsdSkinAsset next;if(!hsd_load(archive,&next.scene,error,material_policy))return false;
    if(next.scene.meshes.size()>kHsdMaxNodes)return fail("mesh metadata budget");
    hsd_detail::Reader r;if(!hsd_anim_detail::open_container(archive,&r,error))return false;
    if(!hsd_skin_metadata(next.scene,r,&next.joints,&next.meshes,error,selected_record))return false;
    next.loaded=!selected_record;*out=std::move(next);return true;
}
inline bool hsd_skin_sample(const HsdSkinAsset& asset,std::span<const Mat4> world,HsdSkinGeometry* out,std::string* error){
    if(out)*out={};
    auto fail=[&](const char* why){if(error)*error=std::string("hsd skin sample: ")+why;return false;};
    if(!out||!asset.loaded||world.empty()||world.size()>kHsdMaxNodes||asset.meshes.size()>kHsdMaxNodes||world.size()!=asset.joints.size()||asset.scene.joints.size()!=world.size()||asset.meshes.size()!=asset.scene.meshes.size())return fail("asset/pose binding");
    for(const auto& m:world)if(!hsd_skin_detail::matrix_valid(m))return fail("pose matrix domain");
    HsdSkinGeometry next;next.world_vertices.resize(asset.meshes.size());std::size_t total=0,bindings=0;
    for(unsigned m=0;m<asset.meshes.size();++m){const auto& skin=asset.meshes[m];
        if((skin.kind!=HsdSkinKind::Rigid&&skin.kind!=HsdSkinKind::Shared&&skin.kind!=HsdSkinKind::Envelope)||
           (skin.kind==HsdSkinKind::Rigid&&skin.palettes.size()!=1)||(skin.kind==HsdSkinKind::Shared&&skin.palettes.size()!=2))return fail("mesh skin kind/slot domain");
        if(skin.kind==HsdSkinKind::Envelope&&(m>=asset.scene.mesh_joint.size()||asset.scene.mesh_joint[m]>=asset.scene.joints.size()||!(asset.scene.joints[asset.scene.mesh_joint[m]].flags&2)))return fail("non-root envelope correction not supported");
        if(skin.palettes.empty()||skin.palettes.size()>hsd_skin_detail::kMaxSlots)return fail("palette budget");
        std::vector<Mat4> matrices;for(const auto& palette:skin.palettes){
            if(palette.influences.empty()||palette.influences.size()>hsd_skin_detail::kMaxInfluences)return fail("influence budget");
            bindings+=palette.influences.size();if(bindings>hsd_skin_detail::kMaxBindings)return fail("global influence-work budget");
            if(skin.kind!=HsdSkinKind::Envelope&&(palette.influences.size()!=1||palette.influences[0].weight!=1))return fail("rigid/shared palette must name one joint");
            f64 sum=0;for(const auto& influence:palette.influences){if(influence.joint>=world.size()||!std::isfinite(influence.weight)||influence.weight<0||influence.weight>1)return fail("influence domain");sum+=influence.weight;}
            if(std::abs(sum-1)>1e-4)return fail("non-unit weights (not silently renormalized)");
            Mat4 matrix;
            // Original 8024C4C4: full first weight uses joint world directly when
            // the source SKELETON_ROOT correction is absent. Only blended weights use
            // jointWorld*inverseBind. This is NOT generic inverse-bind on all slots.
            // This game's DOL threshold is exactly 1.0 (8043D2E8), NOT the
            // 1-FLT_EPSILON threshold in the separate Melee format reference.
            if(skin.kind!=HsdSkinKind::Envelope||palette.influences[0].weight>=1.0f)matrix=world[palette.influences[0].joint];
            else{for(unsigned i=0;i<12;++i)matrix.m[i]=0;
                for(const auto& influence:palette.influences){const auto& bind=asset.joints[influence.joint];if(!bind.has_inverse||!hsd_skin_detail::matrix_valid(bind.inverse))return fail("missing/invalid inverse bind");
                    const auto bone=mat4_multiply(world[influence.joint],bind.inverse);for(unsigned i=0;i<12;++i)matrix.m[i]+=bone.m[i]*influence.weight;}}
            if(!hsd_skin_detail::matrix_valid(matrix))return fail("computed palette matrix domain");
            matrices.push_back(matrix);
        }
        const auto& source=asset.scene.meshes[m].vertices;total+=source.size();if(total>hsd_skin_detail::kMaxVertices)return fail("vertex budget");
        auto& target=next.world_vertices[m];target=source;
        for(unsigned v=0;v<source.size();++v){const auto raw=source[v].matrix_index;if(raw%3||raw/3>=matrices.size())return fail("vertex palette index");
            for(auto index:source[v].texture_matrix_indices)if(index!=60&&(index<30||index>=60||(index-30)%3||unsigned((index-30)/3)>=matrices.size()))return fail("texture palette index");
            for(unsigned k=0;k<3;++k)if(!std::isfinite(source[v].position[k])||!std::isfinite(source[v].normal[k])||std::abs(source[v].position[k])>1e7f||std::abs(source[v].normal[k])>1e7f)return fail("vertex domain");
            const auto& matrix=matrices[raw/3];mat4_transform_point(matrix,source[v].position,target[v].position);
            if(!hsd_skin_detail::normal(matrix,source[v].normal,target[v].normal))return fail("singular/nonfinite normal matrix");
            for(f32 x:target[v].position)if(!std::isfinite(x)||std::abs(x)>1e7f)return fail("deformed position budget");}
    }
    *out=std::move(next);return true;
}
} // namespace gf
