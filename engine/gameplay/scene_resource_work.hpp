#pragma once
#include "engine/core/types.hpp"
namespace gf {
enum class SceneResourceCellKind {Skip,Destination,Empty,Loaded};
struct SceneResourceCell {SceneResourceCellKind kind=SceneResourceCellKind::Skip;ByteView bytes;};
//Native typed equivalents of loader argument words. Pointer arguments identify
//destinations in the same owning batch, never host addresses encoded as u32.
enum class SceneResourceArgumentKind {Unknown,Integer,EffectTextureInput,EffectTextureBindings,EffectTextureObject,EffectPaletteObject};
struct SceneResourceArgument {
    SceneResourceArgumentKind kind=SceneResourceArgumentKind::Unknown;u32 integer=0;
    static SceneResourceArgument number(u32 n){return {SceneResourceArgumentKind::Integer,n};}
    bool operator==(u32 n)const{return kind==SceneResourceArgumentKind::Integer&&integer==n;}
    bool operator==(const SceneResourceArgument&)const=default;
};
} // namespace gf
