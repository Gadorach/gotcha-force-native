#pragma once
#include "engine/core/types.hpp"
namespace gf {
//Only offsets explicitly written by8853C. Names deliberately retain source
//offsets: these are not invented input/camera/clock meanings or a full context.
struct ScenePhysicalControlFields {u8 d1=0,d2=0,d3=0,d4=0;u16 d6=0;u8 d8=0,d9=0,da=0,db=0,eb=0;u16 e8=0;bool operator==(const ScenePhysicalControlFields&)const=default;};
struct ScenePhysicalControlWrite {u16 offset=0;u8 bytes=0;u16 value=0;bool operator==(const ScenePhysicalControlWrite&)const=default;};
} // namespace gf
