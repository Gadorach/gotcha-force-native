#include <cstdio>
#include <cstdlib>
#include "tests/unit/hsd_record_animation_fixture.hpp"
#include "engine/assets/hsd_record_animation.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;std::string error;HsdRecordAnimationBank bank;HsdRecordAnimationTables tables;
    check(!bank.animation_tables(0,&tables)&&tables==HsdRecordAnimationTables{});
    hsd_record_animation_fixture::Fixture fixture;
    auto load=[&](){const auto bytes=fixture.archive();check(bank.load({bytes.data(),bytes.size()},&error));};
    load();check(bank.animation_tables(0,&tables)&&tables==HsdRecordAnimationTables{0x50,0x60,0,1,1,0});
    //Nonnull, zero-entry tables remain observably different from null pointers.
    fixture.word(0x50,0);fixture.word(0x60,0);
    std::erase(fixture.fields,0x50);std::erase(fixture.fields,0x60);load();
    check(bank.animation_tables(0,&tables)&&tables==HsdRecordAnimationTables{0x50,0x60,0,0,0,0});
    fixture.word(0x34,0);fixture.word(0x38,0);
    std::erase(fixture.fields,0x34);std::erase(fixture.fields,0x38);load();
    check(bank.animation_tables(0,&tables)&&tables==HsdRecordAnimationTables{});
    check(!bank.animation_tables(1,&tables)&&tables==HsdRecordAnimationTables{}&&!bank.animation_tables(0,nullptr));
    auto truncated=fixture.archive();truncated.resize(16);
    check(!bank.load({truncated.data(),truncated.size()},&error)&&!bank.animation_tables(0,&tables));
    std::printf("source animation table identity/null-vs-empty: %u checks PASS\n",checks);
}
