#pragma once
#include <memory>
#include "engine/gameplay/borg_library.hpp"

namespace gf {
// The trusted session owner enables this authority after applying its policy.
// A package, script, Borg row or saved CheatSettings bit is not authority.
class BorgCheatPickerAuthority {
    struct Scope {};
public:
    class Permit {
    public:
        Permit()=default;
        bool current()const{return !scope_.expired();}
    private:
        friend class BorgCheatPickerAuthority;
        explicit Permit(const std::shared_ptr<const Scope>& scope):scope_(scope){}
        std::weak_ptr<const Scope> scope_;
    };
    BorgCheatPickerAuthority()=default;
    BorgCheatPickerAuthority(const BorgCheatPickerAuthority&)=delete;
    BorgCheatPickerAuthority& operator=(const BorgCheatPickerAuthority&)=delete;
    void set_authorized(bool allowed){
        if(!allowed)scope_.reset();
        else if(!scope_)scope_=std::make_shared<const Scope>();
    }
    Permit permit()const{return Permit(scope_);}
private:
    std::shared_ptr<const Scope> scope_;
};

enum class BorgCheatPickerResult {OfferReady,PermissionDenied,ExactVariantUnavailable};

// Read-only, pinned installed metadata. There is deliberately no account
// backend, grant API, instance-ID allocator, or actor/roster conversion.
class BorgCheatPicker {
public:
    explicit BorgCheatPicker(BorgLibrary library,BorgCheatPickerAuthority::Permit permit={})
        :library_(std::move(library)),permit_(std::move(permit)){}
    const BorgLibrary& library()const{return library_;}
    const BorgLibraryRow* offer()const{return permit_.current()&&offered_?library_.find(*offered_):nullptr;}
    void cancel(){offered_.reset();}
    BorgCheatPickerResult select(const BorgLibraryKey& key){
        cancel();
        if(!permit_.current())return BorgCheatPickerResult::PermissionDenied;
        const auto* row=library_.find(key);
        if(!row||row->state!=BorgLibraryState::Catalogued||!row->declared_cost)
            return BorgCheatPickerResult::ExactVariantUnavailable;
        offered_=key;return BorgCheatPickerResult::OfferReady;
    }
private:
    const BorgLibrary library_;
    const BorgCheatPickerAuthority::Permit permit_;
    std::optional<BorgLibraryKey> offered_;
};
}
