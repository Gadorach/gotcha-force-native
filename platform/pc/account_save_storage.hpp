#pragma once
#include <memory>
#include <string>
#include "engine/core/account_save_storage.hpp"

namespace gf::pc {
struct AccountSaveIoStats {u64 metadata_bytes=0,payload_bytes=0,metadata_reads=0,full_reads=0,commits=0;};
//Explicit opt-in root, never the existing slotN.sav directory implicitly. Opening
//does not import, migrate or enumerate other roots. Linux implementation only;
//unsupported platforms refuse. Caller owns optional stats longer than storage.
std::unique_ptr<IAccountSaveStorage> create_account_save_storage(const std::string& root,bool create,std::string* error,AccountSaveIoStats* stats=nullptr);
}
