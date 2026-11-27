#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include "engine/core/types.hpp"

namespace gf {
using ProjectileVector=std::array<f32,3>;

// Constant-acceleration lower arc at a given total speed. An unreachable
// point gets the tangent solution, not a guarantee of interception.
inline std::optional<ProjectileVector> projectile_lower_arc(
    const ProjectileVector& origin,const ProjectileVector& point,f32 speed,f32 gravity,f32 pitch_cap){
    const f64 x=f64(point[0])-origin[0],y=f64(point[1])-origin[1],z=f64(point[2])-origin[2];
    const f64 range=std::hypot(x,z),s2=f64(speed)*speed;
    if(!std::isfinite(range)||!std::isfinite(y)||!std::isfinite(speed)||!std::isfinite(gravity)||
       !std::isfinite(pitch_cap)||range<.001||speed<=0||gravity<=0||pitch_cap<=0||pitch_cap>=1.5707964f)
        return {};
    const f64 term=gravity*range*range+2*y*s2,disc=s2*s2-gravity*term;
    // Rationalize the lower root so short, nearly level shots do not
    // lose the pitch to subtracting two nearly equal values.
    const f64 tangent=disc<0?s2/(gravity*range):term/(range*(s2+std::sqrt(disc)));
    const f64 pitch=std::min(f64(pitch_cap),std::atan(tangent));
    const f64 horizontal=speed*std::cos(pitch);
    return ProjectileVector{f32(horizontal*x/range),f32(speed*std::sin(pitch)),f32(horizontal*z/range)};
}

// Gravity needed to reach the sampled height in the remaining horizontal
// travel time. Keep the current heading and skip points behind the shot.
inline std::optional<f32> projectile_gravity_for_height(
    const ProjectileVector& origin,const ProjectileVector& velocity,const ProjectileVector& point,
    f32 minimum,f32 maximum){
    const f64 x=f64(point[0])-origin[0],y=f64(point[1])-origin[1],z=f64(point[2])-origin[2];
    const f64 speed=std::hypot(velocity[0],velocity[2]);
    if(speed<.001||!std::isfinite(speed)||x*velocity[0]+z*velocity[2]<=0)return {};
    const f64 t=std::hypot(x,z)/speed;
    if(t<.001||!std::isfinite(t)||!std::isfinite(y)||!std::isfinite(velocity[1])||
       !std::isfinite(minimum)||!std::isfinite(maximum)||minimum<0||maximum<minimum)return {};
    return f32(std::clamp(2*(velocity[1]*t-y)/(t*t),f64(minimum),f64(maximum)));
}
} // namespace gf
