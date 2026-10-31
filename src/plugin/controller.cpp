#include "controller.hpp"

#include "engine/dn1_presets.hpp"
#include "param_ids.hpp"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/vstpresetkeys.h"
#include "vstgui/plugin-bindings/vst3editor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace dn1::vst3 {
namespace {

constexpr std::int32_t kStateVersion = 2;
constexpr std::int32_t kLegacyStateVersion = 1;

void writeText(Steinberg::Vst::String128 destination, const char* text)
{
    Steinberg::UString(destination, 128).fromAscii(text);
}

bool isOneOf(ParameterId id, ParameterId first, ParameterId last) noexcept
{
    const auto value = static_cast<std::uint32_t>(id);
    return value >= static_cast<std::uint32_t>(first) &&
           value <= static_cast<std::uint32_t>(last);
}

bool isOperatorTime(ParameterId id) noexcept
{
    return isOneOf(id, ParameterId::OpA_Attack, ParameterId::OpD_Attack) ||
           isOneOf(id, ParameterId::OpA_Decay, ParameterId::OpD_Decay) ||
           isOneOf(id, ParameterId::OpA_Release, ParameterId::OpD_Release) ||
           id == ParameterId::AmpAttack || id == ParameterId::AmpDecay ||
           id == ParameterId::AmpRelease;
}

} // namespace

Steinberg::tresult PLUGIN_API Controller::initialize(Steinberg::FUnknown* context)
{
    const auto result = EditControllerEx1::initialize(context);
    if (result != Steinberg::kResultTrue)
        return result;

    // Export the same bank as a real VST3 program list. Hosts can therefore
    // show named presets while the processor still receives one normalized
    // program-change parameter and expands it into the engine catalog.
    addUnit(new Steinberg::Vst::Unit(
        USTRING("DN1FM"), Steinberg::Vst::kRootUnitId,
        Steinberg::Vst::kNoParentUnitId, kPresetParameter));

    auto* presetList = new Steinberg::Vst::ProgramList(
        USTRING("Presets"), kPresetParameter, Steinberg::Vst::kRootUnitId);
    for (const auto& preset : dn1::presetBank()) {
        const auto programIndex = presetList->addProgram(USTRING(preset.name));
        presetList->setProgramInfo(programIndex,
                                   Steinberg::Vst::PresetAttributes::kInstrument,
                                   USTRING("Synthesizer|FM"));
        presetList->setProgramInfo(programIndex,
                                   Steinberg::Vst::PresetAttributes::kStyle,
                                   USTRING(preset.category));
        presetList->setProgramInfo(programIndex,
                                   Steinberg::Vst::PresetAttributes::kCharacter,
                                   USTRING(preset.description));
    }
    addProgramList(presetList);
    parameters.addParameter(presetList->getParameter());

    for (const auto& spec : dn1::kParameterSpecs) {
        parameters.addParameter(
            USTRING(spec.name), USTRING(spec.unit), spec.stepCount,
            spec.defaultNormalized, Steinberg::Vst::ParameterInfo::kCanAutomate,
            toVstParameter(spec.id));
    }
    return result;
}

Steinberg::IPlugView* PLUGIN_API Controller::createView(
    Steinberg::FIDString name)
{
    if (name && std::strcmp(name, Steinberg::Vst::ViewType::kEditor) == 0)
        return new VSTGUI::VST3Editor(this, "view", "editor.uidesc");
    return nullptr;
}

Steinberg::tresult PLUGIN_API Controller::getUnitByBus(
    Steinberg::Vst::MediaType type, Steinberg::Vst::BusDirection direction,
    Steinberg::int32 busIndex, Steinberg::int32 channel,
    Steinberg::Vst::UnitID& unitId)
{
    if (type == Steinberg::Vst::MediaTypes::kEvent &&
        direction == Steinberg::Vst::BusDirections::kInput &&
        busIndex == 0 && channel == 0) {
        unitId = Steinberg::Vst::kRootUnitId;
        return Steinberg::kResultTrue;
    }
    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Controller::setComponentState(
    Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer streamer(state, kLittleEndian);
    Steinberg::int32 version = 0;
    if (!streamer.readInt32(version) ||
        (version != kLegacyStateVersion && version != kStateVersion))
        return Steinberg::kResultFalse;

    if (version == kStateVersion) {
        Steinberg::int32 presetIndex = 0;
        if (!streamer.readInt32(presetIndex) || presetIndex < 0 ||
            presetIndex >= static_cast<Steinberg::int32>(dn1::presetCount()))
            return Steinberg::kResultFalse;
        const auto denominator = static_cast<Steinberg::Vst::ParamValue>(
            std::max<std::size_t>(1, dn1::presetCount() - 1));
        setParamNormalized(kPresetParameter,
                           static_cast<Steinberg::Vst::ParamValue>(presetIndex) /
                               denominator);
    }

    for (const auto& spec : dn1::kParameterSpecs) {
        float value = 0.0f;
        if (!streamer.readFloat(value))
            return Steinberg::kResultFalse;
        setParamNormalized(toVstParameter(spec.id), value);
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API Controller::getParamStringByValue(
    Steinberg::Vst::ParamID tag,
    Steinberg::Vst::ParamValue valueNormalized,
    Steinberg::Vst::String128 string)
{
    if (!isEngineParameter(tag))
        return EditController::getParamStringByValue(tag, valueNormalized, string);

    const auto id = toEngineParameter(tag);
    const float value = static_cast<float>(std::clamp(valueNormalized, 0.0, 1.0));
    char text[64] {};

    if (id == ParameterId::Algorithm) {
        std::snprintf(text, sizeof(text), "%d", static_cast<int>(std::lround(value * 7.0f)) + 1);
    } else if (isOneOf(id, ParameterId::OpA_Ratio, ParameterId::OpD_Ratio)) {
        std::snprintf(text, sizeof(text), "%.9gx", 0.25 * std::pow(64.0, value));
    } else if (isOneOf(id, ParameterId::OpA_Fine, ParameterId::OpD_Fine)) {
        std::snprintf(text, sizeof(text), "%.9g st", (value - 0.5f) * 2.0f);
    } else if (isOperatorTime(id)) {
        std::snprintf(text, sizeof(text), "%.9g s", 0.002 * std::pow(4000.0, value));
    } else if (id == ParameterId::FilterCutoff) {
        std::snprintf(text, sizeof(text), "%.9g Hz", 30.0 * std::pow(600.0, value));
    } else if (id == ParameterId::Pan) {
        std::snprintf(text, sizeof(text), "%+.9g%%", (value - 0.5f) * 200.0f);
    } else {
        std::snprintf(text, sizeof(text), "%.9g%%", value * 100.0f);
    }

    writeText(string, text);
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API Controller::getParamValueByString(
    Steinberg::Vst::ParamID tag, Steinberg::Vst::TChar* string,
    Steinberg::Vst::ParamValue& valueNormalized)
{
    if (!isEngineParameter(tag))
        return EditController::getParamValueByString(tag, string, valueNormalized);

    char ascii[128] {};
    Steinberg::UString(string, 128).toAscii(ascii, sizeof(ascii));
    char* end = nullptr;
    const double parsed = std::strtod(ascii, &end);
    if (end == ascii || !std::isfinite(parsed))
        return Steinberg::kResultFalse;

    const auto id = toEngineParameter(tag);
    double normalizedValue = 0.0;
    if (id == ParameterId::Algorithm) {
        normalizedValue = (std::round(parsed) - 1.0) / 7.0;
    } else if (isOneOf(id, ParameterId::OpA_Ratio, ParameterId::OpD_Ratio)) {
        if (parsed <= 0.0)
            return Steinberg::kResultFalse;
        normalizedValue = std::log(parsed / 0.25) / std::log(64.0);
    } else if (isOneOf(id, ParameterId::OpA_Fine, ParameterId::OpD_Fine)) {
        normalizedValue = parsed / 2.0 + 0.5;
    } else if (isOperatorTime(id)) {
        if (parsed <= 0.0)
            return Steinberg::kResultFalse;
        normalizedValue = std::log(parsed / 0.002) / std::log(4000.0);
    } else if (id == ParameterId::FilterCutoff) {
        if (parsed <= 0.0)
            return Steinberg::kResultFalse;
        normalizedValue = std::log(parsed / 30.0) / std::log(600.0);
    } else if (id == ParameterId::Pan) {
        normalizedValue = parsed / 200.0 + 0.5;
    } else {
        normalizedValue = parsed / 100.0;
    }

    valueNormalized = std::clamp(normalizedValue, 0.0, 1.0);
    return Steinberg::kResultTrue;
}

} // namespace dn1::vst3
