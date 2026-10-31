#pragma once

#include "dn1_engine.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace dn1 {

enum class PresetId : std::uint32_t {
    Init = 0,
    SolidBass,
    GlassBell,
    SoftPad,
    MetalPluck,
    OrganStack,
    Count,
};

struct Preset {
    const char* name {nullptr};
    std::array<float, static_cast<std::size_t>(ParameterId::Count)> values {};
};

constexpr std::size_t presetCount() noexcept
{
    return static_cast<std::size_t>(PresetId::Count);
}

inline void setPresetValue(Preset& preset, ParameterId id, float normalized) noexcept
{
    preset.values[static_cast<std::size_t>(id)] = std::clamp(normalized, 0.0f, 1.0f);
}

inline std::array<Preset, presetCount()> makePresetBank()
{
    std::array<Preset, presetCount()> bank {};
    bank[static_cast<std::size_t>(PresetId::Init)].name = "Init";
    bank[static_cast<std::size_t>(PresetId::SolidBass)].name = "Solid Bass";
    bank[static_cast<std::size_t>(PresetId::GlassBell)].name = "Glass Bell";
    bank[static_cast<std::size_t>(PresetId::SoftPad)].name = "Soft Pad";
    bank[static_cast<std::size_t>(PresetId::MetalPluck)].name = "Metal Pluck";
    bank[static_cast<std::size_t>(PresetId::OrganStack)].name = "Organ Stack";

    // Start every patch from the engine's documented defaults. These are
    // original starter sounds, not copies of factory Digitone programs.
    for (auto& preset : bank) {
        for (const auto& spec : kParameterSpecs)
            setPresetValue(preset, spec.id, spec.defaultNormalized);
    }

    auto& init = bank[static_cast<std::size_t>(PresetId::Init)];
    setPresetValue(init, ParameterId::Algorithm, 0.0f);
    setPresetValue(init, ParameterId::FmAmount, 0.45f);
    setPresetValue(init, ParameterId::Feedback, 0.08f);

    auto& bass = bank[static_cast<std::size_t>(PresetId::SolidBass)];
    setPresetValue(bass, ParameterId::Algorithm, 1.0f / 7.0f);
    setPresetValue(bass, ParameterId::OpA_Ratio, 0.25f);
    setPresetValue(bass, ParameterId::OpB_Ratio, 1.0f / 3.0f);
    setPresetValue(bass, ParameterId::OpC_Ratio, 0.0f);
    setPresetValue(bass, ParameterId::OpD_Ratio, 0.5f);
    setPresetValue(bass, ParameterId::OpA_Level, 0.92f);
    setPresetValue(bass, ParameterId::OpB_Level, 0.80f);
    setPresetValue(bass, ParameterId::OpC_Level, 0.35f);
    setPresetValue(bass, ParameterId::OpD_Level, 0.25f);
    setPresetValue(bass, ParameterId::FmAmount, 0.68f);
    setPresetValue(bass, ParameterId::Feedback, 0.18f);
    setPresetValue(bass, ParameterId::AmpAttack, 0.02f);
    setPresetValue(bass, ParameterId::AmpDecay, 0.28f);
    setPresetValue(bass, ParameterId::AmpSustain, 0.58f);
    setPresetValue(bass, ParameterId::AmpRelease, 0.18f);
    setPresetValue(bass, ParameterId::FilterCutoff, 0.36f);
    setPresetValue(bass, ParameterId::FilterResonance, 0.18f);

    auto& bell = bank[static_cast<std::size_t>(PresetId::GlassBell)];
    setPresetValue(bell, ParameterId::Algorithm, 6.0f / 7.0f);
    setPresetValue(bell, ParameterId::OpA_Ratio, 0.43f);
    setPresetValue(bell, ParameterId::OpB_Ratio, 0.63f);
    setPresetValue(bell, ParameterId::OpC_Ratio, 0.22f);
    setPresetValue(bell, ParameterId::OpD_Ratio, 0.78f);
    setPresetValue(bell, ParameterId::OpA_Level, 0.70f);
    setPresetValue(bell, ParameterId::OpB_Level, 0.58f);
    setPresetValue(bell, ParameterId::OpC_Level, 0.45f);
    setPresetValue(bell, ParameterId::OpD_Level, 0.66f);
    setPresetValue(bell, ParameterId::FmAmount, 0.86f);
    setPresetValue(bell, ParameterId::Feedback, 0.34f);
    setPresetValue(bell, ParameterId::AmpAttack, 0.08f);
    setPresetValue(bell, ParameterId::AmpDecay, 0.64f);
    setPresetValue(bell, ParameterId::AmpSustain, 0.12f);
    setPresetValue(bell, ParameterId::AmpRelease, 0.56f);
    setPresetValue(bell, ParameterId::FilterCutoff, 0.76f);
    setPresetValue(bell, ParameterId::FilterResonance, 0.24f);

    auto& pad = bank[static_cast<std::size_t>(PresetId::SoftPad)];
    setPresetValue(pad, ParameterId::Algorithm, 3.0f / 7.0f);
    setPresetValue(pad, ParameterId::OpA_Ratio, 1.0f / 3.0f);
    setPresetValue(pad, ParameterId::OpB_Ratio, 0.5f);
    setPresetValue(pad, ParameterId::OpC_Ratio, 0.66f);
    setPresetValue(pad, ParameterId::OpD_Ratio, 0.25f);
    setPresetValue(pad, ParameterId::OpA_Level, 0.72f);
    setPresetValue(pad, ParameterId::OpB_Level, 0.55f);
    setPresetValue(pad, ParameterId::OpC_Level, 0.40f);
    setPresetValue(pad, ParameterId::OpD_Level, 0.48f);
    setPresetValue(pad, ParameterId::FmAmount, 0.52f);
    setPresetValue(pad, ParameterId::Feedback, 0.04f);
    setPresetValue(pad, ParameterId::AmpAttack, 0.62f);
    setPresetValue(pad, ParameterId::AmpDecay, 0.70f);
    setPresetValue(pad, ParameterId::AmpSustain, 0.76f);
    setPresetValue(pad, ParameterId::AmpRelease, 0.74f);
    setPresetValue(pad, ParameterId::FilterCutoff, 0.66f);
    setPresetValue(pad, ParameterId::FilterResonance, 0.08f);

    auto& metal = bank[static_cast<std::size_t>(PresetId::MetalPluck)];
    setPresetValue(metal, ParameterId::Algorithm, 0.0f);
    setPresetValue(metal, ParameterId::OpA_Ratio, 0.15f);
    setPresetValue(metal, ParameterId::OpB_Ratio, 0.58f);
    setPresetValue(metal, ParameterId::OpC_Ratio, 0.34f);
    setPresetValue(metal, ParameterId::OpD_Ratio, 0.74f);
    setPresetValue(metal, ParameterId::OpA_Level, 0.94f);
    setPresetValue(metal, ParameterId::OpB_Level, 0.68f);
    setPresetValue(metal, ParameterId::OpC_Level, 0.42f);
    setPresetValue(metal, ParameterId::OpD_Level, 0.36f);
    setPresetValue(metal, ParameterId::FmAmount, 0.82f);
    setPresetValue(metal, ParameterId::Feedback, 0.72f);
    setPresetValue(metal, ParameterId::AmpAttack, 0.0f);
    setPresetValue(metal, ParameterId::AmpDecay, 0.22f);
    setPresetValue(metal, ParameterId::AmpSustain, 0.10f);
    setPresetValue(metal, ParameterId::AmpRelease, 0.26f);
    setPresetValue(metal, ParameterId::FilterCutoff, 0.72f);
    setPresetValue(metal, ParameterId::FilterResonance, 0.32f);

    auto& organ = bank[static_cast<std::size_t>(PresetId::OrganStack)];
    setPresetValue(organ, ParameterId::Algorithm, 7.0f / 7.0f);
    setPresetValue(organ, ParameterId::OpA_Ratio, 1.0f / 3.0f);
    setPresetValue(organ, ParameterId::OpB_Ratio, 1.0f / 3.0f);
    setPresetValue(organ, ParameterId::OpC_Ratio, 0.5f);
    setPresetValue(organ, ParameterId::OpD_Ratio, 0.66f);
    setPresetValue(organ, ParameterId::OpA_Level, 0.72f);
    setPresetValue(organ, ParameterId::OpB_Level, 0.58f);
    setPresetValue(organ, ParameterId::OpC_Level, 0.52f);
    setPresetValue(organ, ParameterId::OpD_Level, 0.46f);
    setPresetValue(organ, ParameterId::FmAmount, 0.28f);
    setPresetValue(organ, ParameterId::Feedback, 0.02f);
    setPresetValue(organ, ParameterId::AmpAttack, 0.14f);
    setPresetValue(organ, ParameterId::AmpDecay, 0.36f);
    setPresetValue(organ, ParameterId::AmpSustain, 0.90f);
    setPresetValue(organ, ParameterId::AmpRelease, 0.24f);
    setPresetValue(organ, ParameterId::FilterCutoff, 0.70f);
    setPresetValue(organ, ParameterId::FilterResonance, 0.05f);

    return bank;
}

inline const std::array<Preset, presetCount()>& presetBank() noexcept
{
    static const auto bank = makePresetBank();
    return bank;
}

inline const Preset& preset(PresetId id) noexcept
{
    return presetBank()[static_cast<std::size_t>(id)];
}

inline void applyPreset(DigitoneEngine& engine, const Preset& preset) noexcept
{
    for (std::size_t index = 0; index < preset.values.size(); ++index)
        engine.setParameter(static_cast<ParameterId>(index), preset.values[index]);
}

} // namespace dn1
