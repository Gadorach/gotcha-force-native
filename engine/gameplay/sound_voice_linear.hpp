#pragma once
// Explicit seed -> shared type1 compressed sample window binding. This is not
// backend allocation/start/stop, event completion, mixing or PCM queue admission.
#include "engine/gameplay/sound_voice_seed.hpp"
#include "engine/audio/dsp_linear_window.hpp"
namespace gf {
inline bool sound_voice_open_linear_window(const SoundVoiceSeed& seed,u32 src_q16,DspLinearWindow* out,std::string* error){
    if(out)*out={};
    if(!out||!seed.resource||seed.dsp>=seed.resource->bank.dsp.size()){if(error)*error="voice linear window: seed/resource/DSP domain";return false;}
    const auto& d=seed.resource->bank.dsp[seed.dsp];DspAdpcmInfo info;
    if(!dsp_voice_header({d.header.data(),d.header.size()},&info,error))return false;
    const u64 bytes=dsp_detail::payload_bytes(info.nibbles);
    if(d.offset>seed.resource->payload.size()||bytes>seed.resource->payload.size()-d.offset){if(error)*error="voice linear window: source payload bounds";return false;}
    return out->load({d.header.data(),d.header.size()},{seed.resource->payload.data()+d.offset,std::size_t(bytes)},src_q16,error);
}
} // namespace gf
