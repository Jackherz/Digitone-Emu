#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace dn1::vst3 {

// Stable IDs for this plug-in. They are newly generated for DN1FM and do not
// identify or reuse an Elektron component.
inline const Steinberg::FUID kProcessorUID(0x6A1F02D4, 0xB7C14E3A,
                                           0x928D10F6, 0x45A8CC21);
inline const Steinberg::FUID kControllerUID(0xA43D8B17, 0x5E2C49F0,
                                            0xB1D63A78, 0xC90E245B);

#define DN1FM_VST3_CATEGORY Steinberg::Vst::PlugType::kInstrumentSynth

} // namespace dn1::vst3
