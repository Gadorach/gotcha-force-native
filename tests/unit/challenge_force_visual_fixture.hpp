#pragma once
#include "engine/gameplay/challenge_force_visual.hpp"
namespace challenge_force_visual_fixture {
inline gf::ChallengeForceArt art(){
    gf::ChallengeForceArt a;a.bank_hash=1;a.card_hidden={100,0,0};a.card_active={0,0,0};a.card_exit={0,-100,0};a.focus_hidden={100,20,0};a.focus_active={0,20,0};a.cost_hidden={20,0,-100};a.cost_active={20,0,0};
    for(unsigned i=0;i<4;++i)a.focus[i]={gf::u16(i),{gf::f32(i),0,0}};
    for(unsigned i=0;i<2;++i)a.cost[i]={gf::u16(i+4),{0,gf::f32(i),0}};
    a.name_origin={1,2,3};a.approach=.5f;a.snap=1;a.alpha_step=.25f;a.focus_alpha=.5f;a.name_extent=8;a.name_advance=4;a.digit_advance=2;a.angle_unit=.0001f;a.cost_angle=100;a.shell_record=6;a.random_name="OWNED";a.random_name+=' ';return a;
}
inline gf::ChallengeForceCardsArt cards(){
    gf::ChallengeForceCardsArt p;p.bank_hash=1;for(unsigned i=0;i<4;++i)p.formats[i]={gf::u16(i+7),gf::s16(i==3?0:(i+1)*100)};
    for(unsigned i=0;i<9;++i)p.single_row[i]={-gf::f32(i),2};
    p.single_z={3,4,5};p.scale_step=.25f;p.scale_min=1;p.scale_max=2;p.y=.5f;p.z_scale=1;p.column_step=2;p.row_span=7.5f;p.row_round=.5f;p.x_even=7;p.x_odd=6;p.z_top=6;p.z_step=4;p.grid_scale=.5f;p.row_colour_step=20;p.rejected_alpha=.25f;p.child_z=2;return p;
}
} // namespace challenge_force_visual_fixture
