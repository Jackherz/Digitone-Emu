#include "cids.hpp"
#include "controller.hpp"
#include "processor.hpp"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "DN1FM"

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace dn1::vst3 {

FUnknown* createProcessorInstance(void*)
{
    return Processor::createInstance(nullptr);
}

FUnknown* createControllerInstance(void*)
{
    return Controller::createInstance(nullptr);
}

} // namespace dn1::vst3

BEGIN_FACTORY_DEF("DN1FM contributors", "https://github.com/Jackherz/Digitone-Emu",
                  "")

DEF_CLASS2(INLINE_UID_FROM_FUID(dn1::vst3::kProcessorUID),
           PClassInfo::kManyInstances,
           kVstAudioEffectClass,
           stringPluginName,
           Vst::kDistributable,
           PlugType::kInstrumentSynth,
           FULL_VERSION_STR,
           kVstVersionString,
           dn1::vst3::createProcessorInstance)

DEF_CLASS2(INLINE_UID_FROM_FUID(dn1::vst3::kControllerUID),
           PClassInfo::kManyInstances,
           kVstComponentControllerClass,
           stringPluginName "Controller",
           0,
           "",
           FULL_VERSION_STR,
           kVstVersionString,
           dn1::vst3::createControllerInstance)

END_FACTORY
