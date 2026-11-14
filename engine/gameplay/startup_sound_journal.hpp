#pragma once
// Immutable accepted game-frame DATA, not an audio clock or delivery ACK.
// Append never calls a controller, save backend, tone resolver or audio sink.
// One serialized append/view/reset owner. Cross-thread view publication needs
// caller synchronization; immutable storage alone does not make the owner atomic.
#include "engine/gameplay/startup_sound_capture_commands.hpp"
namespace gf {
struct StartupSoundJournalEntry {
    StartupSoundCaptureFrame capture;
    StartupSoundCaptureContext context;
};
namespace startup_sound_journal_detail {
struct Scope {};
// Host storage granularity only, not source timing/capacity. Bounded page depth
// avoids an8192-node recursive shared_ptr destruction chain on reload/shutdown.
struct Page {
    std::array<std::shared_ptr<const StartupSoundJournalEntry>,64> entries;
    unsigned used=0;u32 first=0;
    std::shared_ptr<const Page> previous;
};
}
class StartupSoundJournalView {
public:
    u32 frames()const{return tail_?tail_->first+tail_->used-1:0;}
    bool live()const{return !journal_.expired()&&!capture_.expired();}
    // Returns immutable owned DATA from this exact published prefix. No cursor
    // changes, speculative consumption, source ticking or external completion.
    std::shared_ptr<const StartupSoundJournalEntry> find(u32 frame)const{
        if(!live()||!frame||frame>frames())return {};
        auto page=tail_;while(page&&page->first>frame)page=page->previous;
        if(!page||frame-page->first>=page->used)return {};
        return page->entries[frame-page->first];
    }
private:
    friend class StartupSoundJournal;
    std::shared_ptr<const startup_sound_journal_detail::Page> tail_;
    std::weak_ptr<const startup_sound_journal_detail::Scope> journal_;
    std::weak_ptr<const StartupSoundCaptureScope> capture_;
};
class StartupSoundJournal {
public:
    StartupSoundJournal()=default;
    StartupSoundJournal(const StartupSoundJournal&)=delete;
    StartupSoundJournal& operator=(const StartupSoundJournal&)=delete;
    StartupSoundJournal(StartupSoundJournal&&)=default;
    StartupSoundJournal& operator=(StartupSoundJournal&&)=default;
    bool append(const StartupSoundCaptureFrame& frame,const StartupSoundCaptureContext& context,std::string* error){
        auto fail=[&](const char* why){if(error)*error=std::string("startup sound journal: ")+why;return false;};
        const auto scope=frame.scope.lock(),observed=context.scope.lock();
        if(!scope||scope!=observed||frame.frame!=context.frame||frame.frame!=frames()+1||frame.frame>8192||frame.intents.size()>48||frame.ui.overflow||frame.ui.count>2)return fail("live matching scope/consecutive accepted frame/native budget");
        if(tail_&&scope!=capture_.lock())return fail("source reload/foreign capture cannot extend journal");
        // Copy before publication: caller mutation cannot alter an existing view.
        auto entry=std::make_shared<const StartupSoundJournalEntry>(StartupSoundJournalEntry{frame,context});
        auto candidate=std::make_shared<startup_sound_journal_detail::Page>();
        if(tail_&&tail_->used<tail_->entries.size())*candidate=*tail_;
        else{candidate->first=frame.frame;candidate->previous=tail_;}
        candidate->entries[candidate->used++]=std::move(entry);
        if(!scope_)scope_=std::make_shared<const startup_sound_journal_detail::Scope>();
        capture_=scope;tail_=std::move(candidate);if(error)error->clear();return true;
    }
    u32 frames()const{return tail_?tail_->first+tail_->used-1:0;}
    StartupSoundJournalView view()const{
        StartupSoundJournalView result;result.tail_=tail_;result.journal_=scope_;result.capture_=capture_;return result;
    }
    // Explicit new owner boundary; old views cannot admit data after reset. Their
    // previously returned immutable entries are still DATA, not replay grants.
    void reset(){scope_.reset();capture_.reset();tail_.reset();}
private:
    std::shared_ptr<const startup_sound_journal_detail::Scope> scope_;
    std::weak_ptr<const StartupSoundCaptureScope> capture_;
    std::shared_ptr<const startup_sound_journal_detail::Page> tail_;
};
} // namespace gf
