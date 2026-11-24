#pragma once
#include "engine/assets/source_halfword_rng.hpp"
#include "engine/gameplay/source_byte_rng_owner.hpp"
namespace gf {
//Same reviewed recurrence, DIFFERENT storage/owner and a full16-bit return.
//Never reinterpret the party's C6/C7 stream as the source C4 halfword.
inline u16 source_halfword_rng_step(u16& state){
    SourceByteRngState bytes{u8(state),u8(state>>8)};source_byte_rng_step(bytes);
    state=u16(u16(bytes.high)*256u+bytes.low);return state;
}
class SourceHalfwordRngOwner;
class SourceHalfwordRngSnapshot {
public:
    u16 state()const{const auto s=encoded_.state();return u16(u16(s.high)*256u+s.low);}
    bool live()const{return encoded_.live();}
private:friend class SourceHalfwordRngOwner;SourceByteRngSnapshot encoded_;
};
//Serialized explicit native current stream. No inferred seed, clock, reset,
//replay or relationship to the separately owned byte generator is supplied.
class SourceHalfwordRngOwner {
public:
    SourceHalfwordRngOwner()=default;SourceHalfwordRngOwner(const SourceHalfwordRngOwner&)=delete;SourceHalfwordRngOwner& operator=(const SourceHalfwordRngOwner&)=delete;
    SourceHalfwordRngOwner(SourceHalfwordRngOwner&&)=default;SourceHalfwordRngOwner& operator=(SourceHalfwordRngOwner&&)=default;
    bool initialize_explicit(const SourceHalfwordRngProfile& profile,u16 state){if(attempted_||!profile.ready())return false;attempted_=true;return encoded_.initialize_explicit({u8(state),u8(state>>8)});}
    bool ready()const{return encoded_.ready();}
    u16 state()const{const auto s=encoded_.state();return u16(u16(s.high)*256u+s.low);}
    u64 draws()const{return encoded_.draws();}
    SourceHalfwordRngSnapshot snapshot()const{SourceHalfwordRngSnapshot s;s.encoded_=encoded_.snapshot();return s;}
    bool same_owner(const SourceHalfwordRngSnapshot& s)const{return encoded_.same_owner(s.encoded_);}
    bool current(const SourceHalfwordRngSnapshot& s)const{return encoded_.current(s.encoded_);}
    bool next_halfword(u16* out){u8 ignored=0;if(!out||!encoded_.next_byte(&ignored))return false;*out=state();return true;}
    void retire(){encoded_=SourceByteRngOwner{};attempted_=true;}
private:SourceByteRngOwner encoded_;bool attempted_=false;
};
}
