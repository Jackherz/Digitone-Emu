#pragma once

#include "cids.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace dn1::vst3 {

class Controller final : public Steinberg::Vst::EditController {
public:
    static Steinberg::FUnknown* createInstance(void*)
    {
        return static_cast<Steinberg::Vst::IEditController*>(new Controller());
    }

    Steinberg::tresult PLUGIN_API initialize(Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setComponentState(
        Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView(
        Steinberg::FIDString name) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamStringByValue(
        Steinberg::Vst::ParamID tag,
        Steinberg::Vst::ParamValue valueNormalized,
        Steinberg::Vst::String128 string) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamValueByString(
        Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string,
        Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

    OBJ_METHODS(Controller, Steinberg::Vst::EditController)
    DEFINE_INTERFACES
    END_DEFINE_INTERFACES(Steinberg::Vst::EditController)
    REFCOUNT_METHODS(Steinberg::Vst::EditController)
};

} // namespace dn1::vst3
