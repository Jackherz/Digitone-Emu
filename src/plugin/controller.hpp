#pragma once

#include "cids.hpp"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstcomponent.h"

namespace dn1::vst3 {

class Controller final : public Steinberg::Vst::EditControllerEx1 {
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
    Steinberg::tresult PLUGIN_API getUnitByBus(
        Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection direction,
        Steinberg::int32 busIndex, Steinberg::int32 channel,
        Steinberg::Vst::UnitID& unitId) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamStringByValue(
        Steinberg::Vst::ParamID tag,
        Steinberg::Vst::ParamValue valueNormalized,
        Steinberg::Vst::String128 string) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getParamValueByString(
        Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string,
        Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

    OBJ_METHODS(Controller, Steinberg::Vst::EditControllerEx1)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Vst::IUnitInfo)
    END_DEFINE_INTERFACES(Steinberg::Vst::EditControllerEx1)
    REFCOUNT_METHODS(Steinberg::Vst::EditControllerEx1)
};

} // namespace dn1::vst3
