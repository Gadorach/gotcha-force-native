#include <cstdio>
#include <cstdlib>
#include "engine/gameplay/native_animation_mosaic.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
namespace {unsigned checks=0;void check(bool ok){++checks;if(!ok)std::abort();}}
int main(){
    using namespace gf;CharacterMotionBank bank;bank.archives.resize(3);bank.groups[0]={0,kCharacterNoMotion,1};bank.groups[1]={0,2};
    const auto catalog=native_animation_catalog(bank);
    check(catalog.size()==3&&catalog[0].id==0&&catalog[1].id==2&&catalog[2].id==257);
    check(catalog[0].aliases==std::vector<std::pair<u8,u16>>{{0,0},{1,0}});
    check(catalog[2].group==1&&catalog[2].choice==1&&catalog[2].archive==2&&!catalog[2].common);
    const auto common=native_animation_catalog(bank,true);check(common[0].id==10000&&common[2].id==10257&&common[2].common);
    check(native_animation_catalog(CharacterMotionBank{}).empty());
    BattleMvpActor player,target;player.position={0,0,0};target.position={0,0,-500};player.facing={0,-1};
    const auto first=NativeBattleMvpView::camera_for_actors(player,target,1152,648);player.facing={1,0};
    const auto rotated=NativeBattleMvpView::camera_for_actors(player,target,1152,648);
    check(first.cache_key()==rotated.cache_key()); //camera does not read body heading
    check(first.eye[2]>player.position[2]&&first.target[2]==target.position[2]);
    NativeBorgMovementFrame before,after;
    check(native_borg_movement_frame({first.target[0]-first.eye[0],first.target[2]-first.eye[2]},&before));
    target.position[2]=500;const auto crossed=NativeBattleMvpView::camera_for_actors(player,target,1152,648);
    check(native_borg_movement_frame({crossed.target[0]-crossed.eye[0],crossed.target[2]-crossed.eye[2]},&after));
    const auto a=native_borg_move_direction(1,0,before),b=native_borg_move_direction(1,0,after);
    check(a.x*b.x+a.z*b.z<-.99f); //held right stays camera-right after opponent crosses
    check(player.facing==BattlePoint{1,0});
    std::printf("animation IDs/aliases and independent camera/body with camera-relative input: %u checks PASS\n",checks);
}
