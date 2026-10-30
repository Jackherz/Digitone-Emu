#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace dn1 {

enum class ParameterId : std::uint32_t {
    Algorithm = 0,

    OpA_Ratio,
    OpB_Ratio,
    OpC_Ratio,
    OpD_Ratio,

    OpA_Fine,
    OpB_Fine,
    OpC_Fine,
    OpD_Fine,

    OpA_Level,
    OpB_Level,
    OpC_Level,
    OpD_Level,

    OpA_Attack,
    OpB_Attack,
    OpC_Attack,
    OpD_Attack,

    OpA_Decay,
    OpB_Decay,
    OpC_Decay,
    OpD_Decay,

    OpA_Sustain,
    OpB_Sustain,
    OpC_Sustain,
    OpD_Sustain,

    OpA_Release,
    OpB_Release,
    OpC_Release,
    OpD_Release,

    FmAmount,
    Feedback,
    AmpAttack,
    AmpDecay,
    AmpSustain,
    AmpRelease,
    FilterCutoff,
    FilterResonance,
    Pan,
    Master,

    Count,
};

struct ParameterSpec {
    ParameterId id;
    const char* name;
    const char* unit;
    float defaultNormalized;
    int stepCount;
};

// Parameters are deliberately normalized at the plug-in boundary. The
// engine maps them to physical values in one place, which keeps the VST3
// controller and the portable engine in agreement.
inline constexpr std::array<ParameterSpec, static_cast<std::size_t>(ParameterId::Count)>
    kParameterSpecs{{
        {ParameterId::Algorithm, "Algorithm", "", 0.0f, 7},

        {ParameterId::OpA_Ratio, "Op A Ratio", "x", 1.0f / 3.0f, 0},
        {ParameterId::OpB_Ratio, "Op B Ratio", "x", 1.0f / 3.0f, 0},
        {ParameterId::OpC_Ratio, "Op C Ratio", "x", 1.0f / 3.0f, 0},
        {ParameterId::OpD_Ratio, "Op D Ratio", "x", 1.0f / 3.0f, 0},

        {ParameterId::OpA_Fine, "Op A Fine", "st", 0.5f, 0},
        {ParameterId::OpB_Fine, "Op B Fine", "st", 0.5f, 0},
        {ParameterId::OpC_Fine, "Op C Fine", "st", 0.5f, 0},
        {ParameterId::OpD_Fine, "Op D Fine", "st", 0.5f, 0},

        {ParameterId::OpA_Level, "Op A Level", "%", 0.85f, 0},
        {ParameterId::OpB_Level, "Op B Level", "%", 0.85f, 0},
        {ParameterId::OpC_Level, "Op C Level", "%", 0.85f, 0},
        {ParameterId::OpD_Level, "Op D Level", "%", 0.85f, 0},

        {ParameterId::OpA_Attack, "Op A Attack", "s", 0.25f, 0},
        {ParameterId::OpB_Attack, "Op B Attack", "s", 0.25f, 0},
        {ParameterId::OpC_Attack, "Op C Attack", "s", 0.25f, 0},
        {ParameterId::OpD_Attack, "Op D Attack", "s", 0.25f, 0},

        {ParameterId::OpA_Decay, "Op A Decay", "s", 0.45f, 0},
        {ParameterId::OpB_Decay, "Op B Decay", "s", 0.45f, 0},
        {ParameterId::OpC_Decay, "Op C Decay", "s", 0.45f, 0},
        {ParameterId::OpD_Decay, "Op D Decay", "s", 0.45f, 0},

        {ParameterId::OpA_Sustain, "Op A Sustain", "%", 0.8f, 0},
        {ParameterId::OpB_Sustain, "Op B Sustain", "%", 0.8f, 0},
        {ParameterId::OpC_Sustain, "Op C Sustain", "%", 0.8f, 0},
        {ParameterId::OpD_Sustain, "Op D Sustain", "%", 0.8f, 0},

        {ParameterId::OpA_Release, "Op A Release", "s", 0.35f, 0},
        {ParameterId::OpB_Release, "Op B Release", "s", 0.35f, 0},
        {ParameterId::OpC_Release, "Op C Release", "s", 0.35f, 0},
        {ParameterId::OpD_Release, "Op D Release", "s", 0.35f, 0},

        {ParameterId::FmAmount, "FM Amount", "%", 0.65f, 0},
        {ParameterId::Feedback, "Feedback", "%", 0.0f, 0},
        {ParameterId::AmpAttack, "Amp Attack", "s", 0.12f, 0},
        {ParameterId::AmpDecay, "Amp Decay", "s", 0.35f, 0},
        {ParameterId::AmpSustain, "Amp Sustain", "%", 0.85f, 0},
        {ParameterId::AmpRelease, "Amp Release", "s", 0.35f, 0},
        {ParameterId::FilterCutoff, "Filter Cutoff", "Hz", 0.82f, 0},
        {ParameterId::FilterResonance, "Filter Resonance", "%", 0.08f, 0},
        {ParameterId::Pan, "Pan", "%", 0.5f, 0},
        {ParameterId::Master, "Master", "%", 0.72f, 0},
    }};

constexpr std::size_t parameterIndex(ParameterId id) noexcept
{
    return static_cast<std::size_t>(id);
}

constexpr const ParameterSpec& parameterSpec(ParameterId id) noexcept
{
    return kParameterSpecs[parameterIndex(id)];
}

} // namespace dn1
