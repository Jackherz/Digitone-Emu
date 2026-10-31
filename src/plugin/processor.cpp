#include "processor.hpp"

#include "engine/dn1_presets.hpp"
#include "param_ids.hpp"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace dn1::vst3 {
namespace {

constexpr std::int32_t kStateVersion = 2;
constexpr std::int32_t kLegacyStateVersion = 1;
constexpr std::size_t kMaxTimedActions = 512;

enum class TimedActionKind : std::uint8_t {
    Preset,
    Parameter,
    Event,
};

struct TimedAction {
    Steinberg::int32 sampleOffset {0};
    TimedActionKind kind {TimedActionKind::Parameter};
    Steinberg::Vst::ParamID parameterId {0};
    float parameterValue {0.0f};
    Steinberg::Vst::Event event {};
};

} // namespace

Processor::Processor()
{
    setControllerClass(kControllerUID);
    applyPresetIndex(0);
}

void Processor::applyPresetIndex(std::uint32_t index) noexcept
{
    if (index >= dn1::presetCount())
        return;
    currentPreset_ = index;
    dn1::applyPreset(engine_, dn1::preset(static_cast<dn1::PresetId>(index)));
}

Steinberg::tresult PLUGIN_API Processor::initialize(Steinberg::FUnknown* context)
{
    const auto result = AudioEffect::initialize(context);
    if (result == Steinberg::kResultTrue) {
        addAudioOutput(STR16("Stereo Output"),
                       Steinberg::Vst::SpeakerArr::kStereo);
        addEventInput(STR16("MIDI In"), 1);
    }
    return result;
}

Steinberg::tresult PLUGIN_API Processor::setActive(Steinberg::TBool state)
{
    if (state) {
        engine_.prepare(processSetup.sampleRate,
                        static_cast<std::uint32_t>(
                            std::max<Steinberg::int32>(1, processSetup.maxSamplesPerBlock)));
    } else {
        engine_.reset();
    }
    return AudioEffect::setActive(state);
}

Steinberg::tresult PLUGIN_API Processor::setBusArrangements(
    Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns,
    Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
{
    if (numIns == 0 && numOuts == 1 && outputs &&
        outputs[0] == Steinberg::Vst::SpeakerArr::kStereo) {
        return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
    }
    return Steinberg::kResultFalse;
}

Steinberg::tresult PLUGIN_API Processor::canProcessSampleSize(
    Steinberg::int32 symbolicSampleSize)
{
    // The portable engine's first milestone is a float32 real-time path.
    // VST3 hosts are allowed to select this format for an instrument.
    return symbolicSampleSize == Steinberg::Vst::kSample32
               ? Steinberg::kResultTrue
               : Steinberg::kResultFalse;
}

void Processor::handleEvent(const Steinberg::Vst::Event& event) noexcept
{
    switch (event.type) {
    case Steinberg::Vst::Event::kNoteOnEvent:
        engine_.noteOn(event.noteOn.channel, event.noteOn.pitch,
                       event.noteOn.velocity, event.noteOn.noteId);
        break;
    case Steinberg::Vst::Event::kNoteOffEvent:
        engine_.noteOff(event.noteOff.channel, event.noteOff.pitch,
                        event.noteOff.noteId);
        break;
    case Steinberg::Vst::Event::kPolyPressureEvent:
        engine_.polyPressure(event.polyPressure.channel,
                             event.polyPressure.pitch,
                             event.polyPressure.pressure);
        break;
    default:
        break;
    }
}

Steinberg::tresult PLUGIN_API Processor::process(
    Steinberg::Vst::ProcessData& data)
{
    if (data.numSamples <= 0 || !data.outputs || data.numOutputs < 1)
        return Steinberg::kResultTrue;

    auto& outputBus = data.outputs[0];
    if (outputBus.numChannels < 1 || !outputBus.channelBuffers32 ||
        !outputBus.channelBuffers32[0])
        return Steinberg::kResultTrue;

    std::array<TimedAction, kMaxTimedActions> actions {};
    std::size_t actionCount = 0;

    if (data.inputParameterChanges) {
        const auto queueCount = data.inputParameterChanges->getParameterCount();
        for (Steinberg::int32 queueIndex = 0; queueIndex < queueCount;
             ++queueIndex) {
            auto* queue = data.inputParameterChanges->getParameterData(queueIndex);
            if (!queue || (!isEngineParameter(queue->getParameterId()) &&
                           !isPresetParameter(queue->getParameterId())))
                continue;

            const auto pointCount = queue->getPointCount();
            for (Steinberg::int32 pointIndex = 0; pointIndex < pointCount;
                 ++pointIndex) {
                Steinberg::int32 sampleOffset = 0;
                Steinberg::Vst::ParamValue value = 0.0;
                if (queue->getPoint(pointIndex, sampleOffset, value) !=
                    Steinberg::kResultTrue)
                    continue;
                if (actionCount >= actions.size())
                    break;

                auto& action = actions[actionCount++];
                action.sampleOffset = sampleOffset;
                action.kind = isPresetParameter(queue->getParameterId())
                                  ? TimedActionKind::Preset
                                  : TimedActionKind::Parameter;
                action.parameterId = queue->getParameterId();
                action.parameterValue = static_cast<float>(value);
            }
        }
    }

    if (data.inputEvents) {
        const auto eventCount = data.inputEvents->getEventCount();
        for (Steinberg::int32 eventIndex = 0; eventIndex < eventCount;
             ++eventIndex) {
            if (actionCount >= actions.size())
                break;

            Steinberg::Vst::Event event;
            if (data.inputEvents->getEvent(eventIndex, event) !=
                Steinberg::kResultTrue)
                continue;

            auto& action = actions[actionCount++];
            action.sampleOffset = event.sampleOffset;
            action.kind = TimedActionKind::Event;
            action.event = event;
        }
    }

    std::stable_sort(actions.begin(), actions.begin() + actionCount,
                     [](const TimedAction& left, const TimedAction& right) {
                         if (left.sampleOffset != right.sampleOffset)
                             return left.sampleOffset < right.sampleOffset;
                         return left.kind < right.kind;
                     });

    float* left = outputBus.channelBuffers32[0];
    float* right = outputBus.numChannels > 1
                       ? outputBus.channelBuffers32[1]
                       : nullptr;
    const auto totalSamples = data.numSamples;
    Steinberg::int32 cursor = 0;

    for (std::size_t index = 0; index < actionCount; ++index) {
        const auto offset = std::clamp(actions[index].sampleOffset, 0, totalSamples);
        if (offset > cursor) {
            engine_.render(left + cursor, right ? right + cursor : nullptr,
                           static_cast<std::uint32_t>(offset - cursor));
            cursor = offset;
        }

        if (actions[index].kind == TimedActionKind::Preset) {
            const auto maxPreset = static_cast<float>(dn1::presetCount() - 1);
            const auto normalized = std::clamp(actions[index].parameterValue,
                                               0.0f, 1.0f);
            const auto presetIndex = static_cast<std::uint32_t>(
                std::lround(normalized * maxPreset));
            applyPresetIndex(presetIndex);
        } else if (actions[index].kind == TimedActionKind::Parameter) {
            engine_.setParameter(toEngineParameter(actions[index].parameterId),
                                 actions[index].parameterValue);
        } else {
            handleEvent(actions[index].event);
        }
    }

    if (cursor < totalSamples) {
        engine_.render(left + cursor, right ? right + cursor : nullptr,
                       static_cast<std::uint32_t>(totalSamples - cursor));
    }

    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API Processor::setState(Steinberg::IBStream* state)
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
        currentPreset_ = static_cast<std::uint32_t>(presetIndex);
    } else {
        currentPreset_ = 0;
    }

    for (const auto& spec : dn1::kParameterSpecs) {
        float value = 0.0f;
        if (!streamer.readFloat(value))
            return Steinberg::kResultFalse;
        engine_.setParameter(spec.id, value);
    }
    return Steinberg::kResultTrue;
}

Steinberg::tresult PLUGIN_API Processor::getState(Steinberg::IBStream* state)
{
    if (!state)
        return Steinberg::kInvalidArgument;

    Steinberg::IBStreamer streamer(state, kLittleEndian);
    if (!streamer.writeInt32(kStateVersion) ||
        !streamer.writeInt32(static_cast<Steinberg::int32>(currentPreset_)))
        return Steinberg::kResultFalse;
    for (const auto& spec : dn1::kParameterSpecs) {
        if (!streamer.writeFloat(engine_.getParameter(spec.id)))
            return Steinberg::kResultFalse;
    }
    return Steinberg::kResultTrue;
}

} // namespace dn1::vst3
