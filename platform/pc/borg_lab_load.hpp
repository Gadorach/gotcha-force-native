#pragma once
#include "engine/gameplay/borg_lab_picker.hpp"
#include "engine/gameplay/native_battle_mvp_view.hpp"
#include "platform/pc/borg_catalog_load.hpp"

namespace gf::pc {
struct BorgLabModes {
    bool jump=false,dash=false,air_dash=false,glide=false,cruise=false,landing_audio=false;
    bool source_pace=false;
};

struct BorgLabMapping {
    BattleMvpConfig config;
    std::array<BattleMvpMotionChoice,2> ground{};
    std::optional<ProvisionalBorgJumpClips> jump;
    std::optional<ProvisionalBorgDashClips> dash;
    std::optional<ProvisionalBorgAirDashClips> air_dash;
    std::optional<ProvisionalBorgGlideClips> glide;
    std::optional<BattleMvpMotionChoice> cruise_entry;
};

inline bool borg_lab_mapping(const BorgPreviewProfile& profile,const BorgLabModes& modes,BorgLabMapping* out,
                             const char** stage=nullptr,std::string* error=nullptr){
    auto refuse=[&](const char* message,const char* reason){
        if(stage)*stage=message;
        if(error)*error=reason;
        return false;
    };
    if(!out)return false;
    const auto clip_for=[&](std::string_view id)->std::optional<BattleMvpMotionChoice>{
        const auto found=std::find_if(profile.clips.begin(),profile.clips.end(),[&](const auto& c){return c.id==id;});
        if(found==profile.clips.end())return std::nullopt;
        return BattleMvpMotionChoice{found->group,found->choice};
    };
    BorgLabMapping next;
    next.config.provisional_walk=true;
    if(modes.cruise){
        if(!profile.whole_body||!profile.lab_cruise||!borg_preview_clips_valid(profile))
            return refuse("cruise mapping refused","exact whole-body cruise declaration required for --lab-cruise");
        next.config.provisional_cruise=profile.lab_cruise;
        if(!profile.static_body)next.cruise_entry=clip_for("cruise-entry");
    }else{
        const auto walk=clip_for("ground-travel");
        if(profile.static_body||!profile.whole_body||!walk)
            return refuse("movement mapping refused","whole-body animated preview with an explicit ground-travel clip required");
        next.ground={BattleMvpMotionChoice{profile.idle_group,profile.idle_choice},*walk};
    }
    if(modes.jump){
        const auto takeoff=clip_for("ground-takeoff"),air=clip_for("air-continuation"),land=clip_for("landing");
        if(!takeoff||!air||!land)
            return refuse("jump mapping refused","exact ground-takeoff, air-continuation and landing clips required for --lab-jump");
        next.jump=ProvisionalBorgJumpClips{*takeoff,*air,*land};next.config.provisional_jump=true;
    }
    if(modes.dash){
        const auto f=clip_for("ground-dash-forward"),l=clip_for("ground-dash-left");
        const auto b=clip_for("ground-dash-backward"),r=clip_for("ground-dash-right");
        if(!f||!l||!b||!r)return refuse("dash mapping refused","four exact directional ground-dash clips required for --lab-dash");
        next.dash=ProvisionalBorgDashClips{*f,*l,*b,*r};next.config.provisional_ground_dash=true;
    }
    if(modes.air_dash){
        const auto windup=clip_for("air-dash-wind-up"),burst=clip_for("air-dash-active"),recover=clip_for("air-dash-recovery");
        if(!windup||!burst||!recover)
            return refuse("air dash mapping refused","exact wind-up, active and recovery clips required for --lab-air-dash");
        next.air_dash=ProvisionalBorgAirDashClips{*windup,*burst,*recover,profile.lab_air_dash_starts};next.config.provisional_air_dash=true;
    }
    if(modes.glide){
        const auto entry=clip_for("glide-entry"),held=clip_for("glide-held");
        if(!entry||!held)return refuse("glide mapping refused","exact entry and held clips required for --lab-glide");
        next.glide=ProvisionalBorgGlideClips{*entry,*held};next.config.provisional_glide=true;
    }
    if(modes.landing_audio&&profile.lab_landing_pcm.empty())
        return refuse("lab audio refused","missing exact lab landing PCM declaration");
    if(modes.source_pace){
        if(modes.cruise||!profile.lab_ground_pace)
            return refuse("ground pace mapping refused","exact source-relative ground pace declaration required");
        next.config.provisional_ground_pace=profile.lab_ground_pace;
    }
    *out=std::move(next);return true;
}

inline std::vector<BorgLabPicker::Entry> borg_lab_rows(const std::filesystem::path& root,const BorgPackageCatalog& catalogue,
                                                      const BorgLabModes& modes){
    std::vector<BorgLabPicker::Entry> rows;
    for(const auto& entry:catalogue.discovered){
        BorgCatalogSnapshot snapshot;
        if(!load_borg_catalog_snapshot(root,catalogue,entry.manifest.identity.id,&snapshot,nullptr))continue;
        for(const auto& binding:snapshot.preview_bindings){
            BorgPreviewProfile profile;BorgLabMapping mapping;
            if(read_borg_preview_declaration(root,snapshot,binding,&profile)&&borg_lab_mapping(profile,modes,&mapping))
                rows.push_back({entry.manifest.identity.id,entry.manifest.identity.name,binding.variant});
        }
    }
    return rows;
}
}
