#pragma once
#include <array>
#include <optional>
#include "engine/core/types.hpp"
namespace gf {
class ChallengeFinalSetup;class BattlePreparationBootstrap;
enum class SceneDeferredCallback {BattleCleanup};
struct SceneDeferredWord {
    enum class Kind {Callback,ArgumentCount};
    Kind kind=Kind::ArgumentCount;SceneDeferredCallback callback=SceneDeferredCallback::BattleCleanup;u32 count=0;
};
//7FD5C resets cursors, not backing words. 7FD6C stores callback/count and
//advances the 64-word ring. Typed callback identity never stores a host address
//in a guest word. Dispatch/29C54 teardown is a separate, still unsupported step.
class SceneDeferredCallbacks {
public:
    bool initialized()const{return initialized_;}
    u8 read_cursor()const{return read_;}u8 write_cursor()const{return write_;}
    const std::optional<SceneDeferredWord>& word(unsigned i)const{return words_.at(i);}
    bool battle_cleanup_pending()const{return initialized_&&read_==0&&write_==2&&words_[0]&&words_[1]&&words_[0]->kind==SceneDeferredWord::Kind::Callback&&words_[0]->callback==SceneDeferredCallback::BattleCleanup&&words_[1]->kind==SceneDeferredWord::Kind::ArgumentCount&&words_[1]->count==0;}
private:
    friend class ChallengeFinalSetup;friend class BattlePreparationBootstrap;
    void reset_cursors(){read_=write_=0;initialized_=true;}
    bool register_battle_cleanup(){
        //Only the fresh post-finalization empty-ring path is joined so far.
        if(!initialized_||read_||write_)return false;
        words_[write_]={SceneDeferredWord::Kind::Callback,SceneDeferredCallback::BattleCleanup,0};
        write_=u8((write_+1)&63);
        words_[write_]={SceneDeferredWord::Kind::ArgumentCount,SceneDeferredCallback::BattleCleanup,0};
        write_=u8((write_+1)&63);return true;
    }
    std::array<std::optional<SceneDeferredWord>,64> words_;u8 read_=0,write_=0;bool initialized_=false;
};
} // namespace gf
