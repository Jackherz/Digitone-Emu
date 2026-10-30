#pragma once

#include "engine/dn1_parameters.hpp"
#include "pluginterfaces/vst/vsttypes.h"

#include <cstdint>

namespace dn1::vst3 {

constexpr Steinberg::Vst::ParamID kFirstEngineParameter = 1000;
constexpr Steinberg::Vst::ParamID kLastEngineParameter =
    kFirstEngineParameter + static_cast<Steinberg::Vst::ParamID>(ParameterId::Count) - 1;

constexpr Steinberg::Vst::ParamID toVstParameter(ParameterId id) noexcept
{
    return kFirstEngineParameter + static_cast<Steinberg::Vst::ParamID>(id);
}

constexpr bool isEngineParameter(Steinberg::Vst::ParamID id) noexcept
{
    return id >= kFirstEngineParameter && id <= kLastEngineParameter;
}

constexpr ParameterId toEngineParameter(Steinberg::Vst::ParamID id) noexcept
{
    return static_cast<ParameterId>(id - kFirstEngineParameter);
}

} // namespace dn1::vst3
