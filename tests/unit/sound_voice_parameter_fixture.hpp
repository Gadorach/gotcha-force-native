#pragma once
#include "engine/gameplay/sound_voice_parameters.hpp"
#include "tests/unit/sound_voice_lifecycle_fixture.hpp"
namespace sound_voice_parameter_fixture {
inline gf::SoundVoiceControlObservation controls(gf::u64 callback){gf::SoundVoiceControlObservation c;c.callback=callback;c.declared=true;c.volume=c.master_volume=127;for(auto& a:c.aux){a.declared=true;a.present=true;a.gain_byte=255;}return c;}
inline gf::SoundVoiceBankGainObservation bank(gf::u8 id,gf::u64 callback){gf::SoundVoiceBankGainObservation b;b.bank=id;b.callback=callback;b.declared=true;b.volume=127;b.programs.fill(127);return b;}
} // namespace sound_voice_parameter_fixture
