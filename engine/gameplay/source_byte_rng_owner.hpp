#pragma once
#include <limits>
#include <memory>
#include "engine/gameplay/source_byte_rng.hpp"
namespace gf {
class ChallengeInitialPartyAssembly;class SourceByteRngOwner;
namespace source_byte_rng_detail {struct Scope {SourceByteRngState state;u64 revision=0,draws=0;};}
class SourceByteRngSnapshot {
public:
    SourceByteRngState state()const{return state_;}
    bool live()const{return !scope_.expired();}
private:
    friend class SourceByteRngOwner;friend class ChallengeInitialPartyAssembly;
    std::weak_ptr<const source_byte_rng_detail::Scope> scope_;SourceByteRngState state_;u64 revision_=0;
};
//Serialized native owner, not a thread-safe global or original seed/phase proof.
//The upstream caller explicitly supplies its current stream. No clock/default
//seed, entry reset, save/restore policy or implicit battle acknowledgement.
class SourceByteRngOwner {
public:
    SourceByteRngOwner()=default;SourceByteRngOwner(const SourceByteRngOwner&)=delete;SourceByteRngOwner& operator=(const SourceByteRngOwner&)=delete;
    SourceByteRngOwner(SourceByteRngOwner&&)=default;SourceByteRngOwner& operator=(SourceByteRngOwner&&)=default;
    bool initialize_explicit(SourceByteRngState state){if(scope_)return false;scope_=std::make_shared<source_byte_rng_detail::Scope>();scope_->state=state;return true;}
    bool ready()const{return bool(scope_);}SourceByteRngState state()const{return scope_?scope_->state:SourceByteRngState{};}
    u64 revision()const{return scope_?scope_->revision:0;}u64 draws()const{return scope_?scope_->draws:0;}
    SourceByteRngSnapshot snapshot()const{SourceByteRngSnapshot s;if(scope_){s.scope_=scope_;s.state_=scope_->state;s.revision_=scope_->revision;}return s;}
    bool same_owner(const SourceByteRngSnapshot& s)const{return scope_&&s.scope_.lock()==scope_;}
    bool current(const SourceByteRngSnapshot& s)const{return scope_&&s.scope_.lock()==scope_&&s.revision_==scope_->revision&&s.state_==scope_->state;}
    bool next_byte(u8* out){if(!out||!scope_||scope_->revision==std::numeric_limits<u64>::max()||scope_->draws==std::numeric_limits<u64>::max())return false;*out=source_byte_rng_step(scope_->state);++scope_->revision;++scope_->draws;return true;}
private:
    friend class ChallengeInitialPartyAssembly;
    friend class BattlePreparationBootstrap;
    friend class BattleActorConstruction;
    bool commit(const SourceByteRngSnapshot& s,SourceByteRngState next,u32 draws){if(!current(s)||scope_->revision==std::numeric_limits<u64>::max()||draws>std::numeric_limits<u64>::max()-scope_->draws)return false;scope_->state=next;++scope_->revision;scope_->draws+=draws;return true;}
    std::shared_ptr<source_byte_rng_detail::Scope> scope_;
};
} // namespace gf
