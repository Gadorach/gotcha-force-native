#pragma once

#include "engine/assets/borg_trailing_chain.hpp"
#include "engine/gameplay/character_pose.hpp"

namespace gf {

// Native secondary motion. These points never become actor bodies, sockets,
// or colliders. The skin uses the solved copy; simulation keeps its input pose.
inline bool borg_trailing_chain_binds(const BorgTrailingChainSpec& spec,const HsdModelBones& rig){
    if(!borg_trailing_chain_valid(spec))return false;
    for(unsigned i=0;i<spec.count;++i){
        const unsigned b=spec.bones[i];
        if(b>=rig.bones.size()||(i&&rig.bones[b].parent!=spec.bones[i-1]))return false;
        for(unsigned other=0;other<rig.bones.size();++other)
            if(rig.bones[other].parent==b&&(i+1==spec.count||other!=spec.bones[i+1]))return false;
    }
    return true;
}

class BorgTrailingChain {
public:
    using Point=std::array<f32,3>;
    unsigned contacts()const{return contacts_;}
    const Point* tip()const{return valid_?&points_[count_-1]:nullptr;}

    template<class Floor>
    bool step(const BorgTrailingChainSpec& spec,u32 tick,const CharacterWorldPose& posed,Floor floor,CharacterWorldPose* out){
        if(!out||!borg_trailing_chain_valid(spec)||posed.matrices.empty())return false;
        auto next=*this;
        const bool same=valid_&&count_==spec.count&&bones_==spec.bones;
        const bool repeated=same&&tick==tick_;
        const bool advance=same&&tick>tick_&&tick-tick_==1;
        if(!repeated)next.contacts_=0;
        next.count_=spec.count;next.bones_=spec.bones;
        std::array<Point,kBorgTrailingChainJoints> rest;
        for(unsigned i=0;i<spec.count;++i){
            if(spec.bones[i]>=posed.matrices.size())return false;
            const auto& matrix=posed.matrices[spec.bones[i]];
            if(!character_pose_detail::matrix(matrix))return false;
            rest[i]={matrix.at(0,3),matrix.at(1,3),matrix.at(2,3)};
        }
        Point back{-posed.matrices[0].at(0,2),0,-posed.matrices[0].at(2,2)};
        if(!unit(back))back={0,0,-1};
        if(!repeated)next.points_[0]=rest[0];
        for(unsigned i=1;i<spec.count;++i){
            const Point offset=sub(rest[i],rest[i-1]);
            const f32 length=norm(offset);
            if(!std::isfinite(length)||length<.01f||length>2000)return false;
            if(repeated)continue;
            const auto& parent=next.points_[i-1];
            const Point goal=add(parent,offset);
            Point candidate=goal;
            if(advance){
                candidate=add(points_[i],mul(velocities_[i],.65f));
                candidate=add(candidate,mul(sub(goal,candidate),.22f));
                candidate[1]-=length*.035f;
            }
            Point direction=sub(candidate,parent);
            if(!unit(direction))direction=mul(offset,1/length);
            auto end=add(parent,mul(direction,length));
            // Requery after projecting onto the link sphere, so the corrected
            // point samples its own floor, not the root's or the prior point's.
            bool touched=false;
            for(unsigned pass=0;pass<2;++pass){
                f32 ground;
                if(!floor(end,&ground)||!std::isfinite(ground))break;
                const f32 height=ground+length*.12f;
                if(end[1]>=height-.001f||height>=parent[1]+length)break;
                const f32 dy=std::max(-length,height-parent[1]);
                const f32 radius=std::sqrt(std::max(0.f,length*length-dy*dy));
                Point planar{direction[0],0,direction[2]};
                if(!unit(planar))planar=back;
                end=add(parent,mul(planar,radius));end[1]=parent[1]+dy;
                touched=true;
            }
            next.contacts_+=touched;
            next.points_[i]=end;
            next.velocities_[i]=advance?sub(end,points_[i]):Point{};
            if(touched)next.velocities_[i][1]=std::max(0.f,next.velocities_[i][1]);
            const f32 speed=norm(next.velocities_[i]);
            if(speed>length*.8f)next.velocities_[i]=mul(next.velocities_[i],length*.8f/speed);
        }
        auto visual=posed;
        Mat4 swing;
        for(unsigned i=0;i<spec.count;++i){
            if(i+1<spec.count&&
               !rotation(sub(rest[i+1],rest[i]),sub(next.points_[i+1],next.points_[i]),&swing))return false;
            auto matrix=mat4_multiply(swing,posed.matrices[spec.bones[i]]);
            for(unsigned k=0;k<3;++k)matrix.at(k,3)=next.points_[i][k];
            visual.matrices[spec.bones[i]]=matrix;
        }
        next.tick_=tick;next.valid_=true;
        *out=std::move(visual);*this=next;
        return true;
    }
private:
    static Point add(Point a,const Point& b){for(unsigned k=0;k<3;++k)a[k]+=b[k];return a;}
    static Point sub(Point a,const Point& b){for(unsigned k=0;k<3;++k)a[k]-=b[k];return a;}
    static Point mul(Point a,f32 b){for(auto& v:a)v*=b;return a;}
    static f32 norm(const Point& a){return std::hypot(a[0],a[1],a[2]);}
    static bool unit(Point& p){const f32 n=norm(p);if(!std::isfinite(n)||n<1e-5f)return false;p=mul(p,1/n);return true;}
    static Point cross(const Point& a,const Point& b){
        return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    }
    static bool rotation(Point from,Point to,Mat4* out){
        if(!unit(from)||!unit(to))return false;
        f32 cosine=std::clamp(from[0]*to[0]+from[1]*to[1]+from[2]*to[2],-1.f,1.f);
        Point axis=cross(from,to);f32 sine=norm(axis);
        *out={};
        if(sine<1e-5f){
            if(cosine>=0)return true;
            axis=cross(from,std::abs(from[0])<.8f?Point{1,0,0}:Point{0,1,0});
            if(!unit(axis))return false;
            sine=0;cosine=-1;
        }else axis=mul(axis,1/sine);
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){
            Point column{};column[j]=1;
            out->at(i,j)=(i==j?cosine:0)+axis[i]*axis[j]*(1-cosine)+cross(axis,column)[i]*sine;
        }
        return true;
    }
    std::array<Point,kBorgTrailingChainJoints> points_{},velocities_{};
    std::array<u8,kBorgTrailingChainJoints> bones_{};
    u32 tick_=0;
    unsigned contacts_=0,count_=0;
    bool valid_=false;
};

} // namespace gf
