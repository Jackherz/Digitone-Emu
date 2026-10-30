#include "dn1_engine.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace dn1 {
namespace {

constexpr double kPi = 3.1415926535897932384626433832795;
constexpr double kTwoPi = 2.0 * kPi;
constexpr float kVoiceGain = 0.22f;

constexpr float clamp01(float value) noexcept
{
    return std::clamp(value, 0.0f, 1.0f);
}

constexpr float clampSigned(float value) noexcept
{
    return std::clamp(value, -1.0f, 1.0f);
}

constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kRatioIds{
    ParameterId::OpA_Ratio, ParameterId::OpB_Ratio, ParameterId::OpC_Ratio,
    ParameterId::OpD_Ratio};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kFineIds{
    ParameterId::OpA_Fine, ParameterId::OpB_Fine, ParameterId::OpC_Fine,
    ParameterId::OpD_Fine};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kLevelIds{
    ParameterId::OpA_Level, ParameterId::OpB_Level, ParameterId::OpC_Level,
    ParameterId::OpD_Level};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kAttackIds{
    ParameterId::OpA_Attack, ParameterId::OpB_Attack, ParameterId::OpC_Attack,
    ParameterId::OpD_Attack};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kDecayIds{
    ParameterId::OpA_Decay, ParameterId::OpB_Decay, ParameterId::OpC_Decay,
    ParameterId::OpD_Decay};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kSustainIds{
    ParameterId::OpA_Sustain, ParameterId::OpB_Sustain, ParameterId::OpC_Sustain,
    ParameterId::OpD_Sustain};
constexpr std::array<ParameterId, DigitoneEngine::kOperatorCount> kReleaseIds{
    ParameterId::OpA_Release, ParameterId::OpB_Release, ParameterId::OpC_Release,
    ParameterId::OpD_Release};

float midiToHz(std::int32_t note) noexcept
{
    const auto semitones = static_cast<double>(note - 69);
    return static_cast<float>(440.0 * std::pow(2.0, semitones / 12.0));
}

} // namespace

void DigitoneEngine::Envelope::configure(double newSampleRate, float attack,
                                          float decay, float sustain,
                                          float releaseTime) noexcept
{
    sampleRate = std::max(1.0, newSampleRate);
    attackSeconds = std::max(0.0005f, attack);
    decaySeconds = std::max(0.0005f, decay);
    sustainLevel = clamp01(sustain);
    releaseSeconds = std::max(0.0005f, releaseTime);
}

void DigitoneEngine::Envelope::start() noexcept
{
    stage = EnvelopeStage::Attack;
    level = 0.0f;
    stageStartLevel = 0.0f;
    samplesInStage = 0;
    stageLength = std::max<std::uint32_t>(
        1, static_cast<std::uint32_t>(attackSeconds * sampleRate));
}

void DigitoneEngine::Envelope::release() noexcept
{
    if (stage == EnvelopeStage::Idle || stage == EnvelopeStage::Release)
        return;

    stage = EnvelopeStage::Release;
    stageStartLevel = level;
    samplesInStage = 0;
    stageLength = std::max<std::uint32_t>(
        1, static_cast<std::uint32_t>(releaseSeconds * sampleRate));
}

float DigitoneEngine::Envelope::next() noexcept
{
    switch (stage) {
    case EnvelopeStage::Idle:
        return 0.0f;
    case EnvelopeStage::Attack:
        ++samplesInStage;
        level = static_cast<float>(samplesInStage) /
                static_cast<float>(stageLength);
        if (samplesInStage >= stageLength) {
            level = 1.0f;
            stage = EnvelopeStage::Decay;
            stageStartLevel = 1.0f;
            samplesInStage = 0;
            stageLength = std::max<std::uint32_t>(
                1, static_cast<std::uint32_t>(decaySeconds * sampleRate));
        }
        break;
    case EnvelopeStage::Decay:
        ++samplesInStage;
        level = 1.0f + (sustainLevel - 1.0f) *
                           (static_cast<float>(samplesInStage) /
                            static_cast<float>(stageLength));
        if (samplesInStage >= stageLength) {
            level = sustainLevel;
            stage = EnvelopeStage::Sustain;
        }
        break;
    case EnvelopeStage::Sustain:
        level = sustainLevel;
        break;
    case EnvelopeStage::Release:
        ++samplesInStage;
        level = stageStartLevel *
                (1.0f - static_cast<float>(samplesInStage) /
                           static_cast<float>(stageLength));
        if (samplesInStage >= stageLength) {
            level = 0.0f;
            stage = EnvelopeStage::Idle;
        }
        break;
    }

    return std::max(0.0f, level);
}

bool DigitoneEngine::Envelope::idle() const noexcept
{
    return stage == EnvelopeStage::Idle;
}

DigitoneEngine::DigitoneEngine()
{
    for (std::size_t index = 0; index < parameters_.size(); ++index)
        parameters_[index] = kParameterSpecs[index].defaultNormalized;

    pitchBend_.fill(0.5f);
    modWheel_.fill(0.0f);
    sustainPedal_.fill(false);
}

void DigitoneEngine::prepare(double sampleRate, std::uint32_t maxBlockSize) noexcept
{
    sampleRate_ = std::max(8000.0, sampleRate);
    maxBlockSize_ = maxBlockSize;
    reset();
}

void DigitoneEngine::reset() noexcept
{
    voices_ = {};
    voiceAge_ = 0;
    filterLeft_ = 0.0f;
    filterRight_ = 0.0f;
    pitchBend_.fill(0.5f);
    modWheel_.fill(0.0f);
    sustainPedal_.fill(false);
}

void DigitoneEngine::setParameter(ParameterId id, float normalizedValue) noexcept
{
    const auto index = parameterIndex(id);
    if (index < parameters_.size())
        parameters_[index] = clamp01(normalizedValue);
}

float DigitoneEngine::getParameter(ParameterId id) const noexcept
{
    const auto index = parameterIndex(id);
    return index < parameters_.size() ? parameters_[index] : 0.0f;
}

float DigitoneEngine::normalized(ParameterId id) const noexcept
{
    return getParameter(id);
}

float DigitoneEngine::ratio(ParameterId id) const noexcept
{
    // 0.25x to 16x, with more useful resolution around the harmonic ratios.
    return static_cast<float>(0.25 * std::pow(64.0, normalized(id)));
}

float DigitoneEngine::fineSemitones(ParameterId id) const noexcept
{
    return (normalized(id) - 0.5f) * 2.0f;
}

float DigitoneEngine::seconds(ParameterId id) const noexcept
{
    // A short but non-zero floor avoids denormal values and makes envelopes
    // behave predictably when a host sends the minimum automation value.
    return static_cast<float>(0.002 * std::pow(4000.0, normalized(id)));
}

float DigitoneEngine::cutoffHz() const noexcept
{
    return static_cast<float>(30.0 * std::pow(600.0, normalized(ParameterId::FilterCutoff)));
}

float DigitoneEngine::operatorLevel(ParameterId id) const noexcept
{
    return normalized(id);
}

void DigitoneEngine::configureVoice(Voice& voice) noexcept
{
    for (std::size_t index = 0; index < kOperatorCount; ++index) {
        voice.operators[index].envelope.configure(
            sampleRate_, seconds(kAttackIds[index]), seconds(kDecayIds[index]),
            normalized(kSustainIds[index]), seconds(kReleaseIds[index]));
    }
    voice.amplifier.configure(sampleRate_, seconds(ParameterId::AmpAttack),
                              seconds(ParameterId::AmpDecay),
                              normalized(ParameterId::AmpSustain),
                              seconds(ParameterId::AmpRelease));
}

std::size_t DigitoneEngine::allocateVoice(std::uint8_t channel,
                                          std::int32_t note,
                                          std::int32_t noteId) noexcept
{
    for (std::size_t index = 0; index < voices_.size(); ++index) {
        const auto& voice = voices_[index];
        if (voice.active && voice.channel == channel &&
            ((noteId >= 0 && voice.noteId == noteId) ||
             (noteId < 0 && voice.note == note && voice.keyDown))) {
            return index;
        }
    }

    for (std::size_t index = 0; index < voices_.size(); ++index) {
        if (!voices_[index].active)
            return index;
    }

    std::size_t oldest = 0;
    for (std::size_t index = 1; index < voices_.size(); ++index) {
        if (voices_[index].age < voices_[oldest].age)
            oldest = index;
    }
    return oldest;
}

void DigitoneEngine::startVoice(Voice& voice, std::uint8_t channel,
                                std::int32_t note, float velocity,
                                std::int32_t noteId) noexcept
{
    voice = {};
    voice.active = true;
    voice.keyDown = true;
    voice.channel = static_cast<std::uint8_t>(channel & 0x0f);
    voice.note = std::clamp(note, 0, 127);
    voice.noteId = noteId;
    voice.velocity = clamp01(velocity);
    voice.age = ++voiceAge_;
    configureVoice(voice);
    for (auto& op : voice.operators) {
        op.phase = 0.0;
        op.lastOutput = 0.0f;
        op.envelope.start();
    }
    voice.amplifier.start();
}

void DigitoneEngine::noteOn(std::uint8_t channel, std::int32_t note,
                            float velocity, std::int32_t noteId) noexcept
{
    if (velocity <= 0.0f) {
        noteOff(channel, note, noteId);
        return;
    }
    const auto index = allocateVoice(channel, note, noteId);
    startVoice(voices_[index], channel, note, velocity, noteId);
}

void DigitoneEngine::releaseVoice(Voice& voice) noexcept
{
    if (!voice.active)
        return;
    voice.pendingRelease = false;
    voice.amplifier.release();
    for (auto& op : voice.operators)
        op.envelope.release();
}

void DigitoneEngine::releaseMatching(std::uint8_t channel, std::int32_t note,
                                     std::int32_t noteId) noexcept
{
    const auto maskedChannel = static_cast<std::uint8_t>(channel & 0x0f);
    for (auto& voice : voices_) {
        if (!voice.active || voice.channel != maskedChannel)
            continue;
        const bool matches = noteId >= 0 ? voice.noteId == noteId :
                                          voice.note == note && voice.keyDown;
        if (matches) {
            voice.keyDown = false;
            if (sustainPedal_[maskedChannel])
                voice.pendingRelease = true;
            else
                releaseVoice(voice);
            if (noteId >= 0)
                break;
        }
    }
}

void DigitoneEngine::noteOff(std::uint8_t channel, std::int32_t note,
                             std::int32_t noteId) noexcept
{
    releaseMatching(channel, note, noteId);
}

void DigitoneEngine::pitchBend(std::uint8_t channel, float normalizedValue) noexcept
{
    pitchBend_[channel & 0x0f] = clamp01(normalizedValue);
}

void DigitoneEngine::polyPressure(std::uint8_t channel, std::int32_t note,
                                  float pressure) noexcept
{
    const auto maskedChannel = static_cast<std::uint8_t>(channel & 0x0f);
    for (auto& voice : voices_) {
        if (voice.active && voice.channel == maskedChannel && voice.note == note)
            voice.pressure = clamp01(pressure);
    }
}

void DigitoneEngine::controlChange(std::uint8_t channel, std::int32_t controller,
                                   float normalizedValue) noexcept
{
    const auto maskedChannel = static_cast<std::uint8_t>(channel & 0x0f);
    const auto value = clamp01(normalizedValue);
    if (controller == 1) {
        modWheel_[maskedChannel] = value;
    } else if (controller == 64) {
        const bool wasDown = sustainPedal_[maskedChannel];
        const bool isDown = value >= 0.5f;
        sustainPedal_[maskedChannel] = isDown;
        if (wasDown && !isDown) {
            for (auto& voice : voices_) {
                if (voice.active && voice.channel == maskedChannel &&
                    voice.pendingRelease && !voice.keyDown)
                    releaseVoice(voice);
            }
        }
    } else if (controller == 120 || controller == 123) {
        allNotesOff();
    }
}

void DigitoneEngine::allNotesOff() noexcept
{
    for (auto& voice : voices_) {
        voice.keyDown = false;
        if (voice.active)
            releaseVoice(voice);
    }
    sustainPedal_.fill(false);
}

float DigitoneEngine::renderVoice(Voice& voice) noexcept
{
    if (!voice.active)
        return 0.0f;

    const float ampLevel = voice.amplifier.next();
    if (voice.amplifier.idle()) {
        voice.active = false;
        return 0.0f;
    }

    const auto channel = voice.channel & 0x0f;
    const float bendSemitones = (pitchBend_[channel] - 0.5f) * 4.0f;
    const float pressureBoost = 1.0f + voice.pressure * 0.35f;
    const float wheelBoost = modWheel_[channel] * 0.35f;
    const float fmIndex = (1.0f + 7.0f * normalized(ParameterId::FmAmount)) *
                          pressureBoost * (1.0f + wheelBoost);
    const float feedback = normalized(ParameterId::Feedback) * 5.0f;

    std::array<float, kOperatorCount> opLevels {};
    for (std::size_t index = 0; index < kOperatorCount; ++index)
        opLevels[index] = voice.operators[index].envelope.next() *
                          operatorLevel(kLevelIds[index]);

    const float baseFrequency = midiToHz(voice.note) *
                                std::pow(2.0f, bendSemitones / 12.0f);
    auto renderOperator = [&](std::size_t index, float modulation) noexcept {
        auto& op = voice.operators[index];
        const float frequency = baseFrequency * ratio(kRatioIds[index]) *
                                std::pow(2.0f, fineSemitones(kFineIds[index]) / 12.0f);
        const double phaseIncrement = std::clamp(
            static_cast<double>(frequency) / sampleRate_, 0.0, 0.49);
        const float value = static_cast<float>(
            std::sin(kTwoPi * op.phase + static_cast<double>(modulation)));
        op.phase += phaseIncrement;
        op.phase -= std::floor(op.phase);
        op.lastOutput = value * opLevels[index];
        return op.lastOutput;
    };

    // The eight routings are intentionally expressed as signal-flow graphs,
    // rather than as a lookup table copied from a device implementation. They
    // provide the same useful 4-operator FM building blocks while this
    // clean-room engine is being measured against user-supplied references.
    float output = 0.0f;
    const auto algorithm = static_cast<int>(
        std::lround(normalized(ParameterId::Algorithm) * 7.0f));
    switch (algorithm) {
    case 0: { // A -> B -> C -> D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, a * fmIndex);
        const float c = renderOperator(2, b * fmIndex);
        output = renderOperator(3, c * fmIndex);
        break;
    }
    case 1: { // A + B -> C -> D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, 0.0f);
        const float c = renderOperator(2, (a + b) * fmIndex);
        output = renderOperator(3, c * fmIndex);
        break;
    }
    case 2: { // A -> B, C -> D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, a * fmIndex);
        const float c = renderOperator(2, 0.0f);
        const float d = renderOperator(3, c * fmIndex);
        output = 0.5f * (b + d);
        break;
    }
    case 3: { // A -> B plus C -> D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, a * fmIndex);
        const float c = renderOperator(2, 0.0f);
        const float d = renderOperator(3, c * fmIndex);
        output = b + d;
        break;
    }
    case 4: { // A -> B -> C plus D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, a * fmIndex);
        const float c = renderOperator(2, b * fmIndex);
        const float d = renderOperator(3, 0.0f);
        output = c + d;
        break;
    }
    case 5: { // A -> B plus C plus D
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, a * fmIndex);
        const float c = renderOperator(2, 0.0f);
        const float d = renderOperator(3, 0.0f);
        output = b + c + d;
        break;
    }
    case 6: { // C -> A, D -> B
        const float c = renderOperator(2, 0.0f);
        const float d = renderOperator(3, 0.0f);
        const float a = renderOperator(0, c * fmIndex);
        const float b = renderOperator(1, d * fmIndex);
        output = a + b;
        break;
    }
    case 7: { // four parallel carriers
        const float a = renderOperator(0, voice.operators[0].lastOutput * feedback);
        const float b = renderOperator(1, 0.0f);
        const float c = renderOperator(2, 0.0f);
        const float d = renderOperator(3, 0.0f);
        output = a + b + c + d;
        break;
    }
    default:
        break;
    }

    return output * ampLevel * voice.velocity * kVoiceGain;
}

void DigitoneEngine::render(float* left, float* right,
                            std::uint32_t numSamples) noexcept
{
    if (!left || numSamples == 0)
        return;

    const float cutoff = std::min(cutoffHz(),
                                  static_cast<float>(sampleRate_ * 0.45));
    const float coefficient = 1.0f - static_cast<float>(
        std::exp(-2.0 * kPi * static_cast<double>(cutoff) / sampleRate_));
    const float resonance = normalized(ParameterId::FilterResonance);
    const float master = normalized(ParameterId::Master);
    const float pan = normalized(ParameterId::Pan) *
                      static_cast<float>(kPi * 0.5);
    const float panLeft = std::cos(pan);
    const float panRight = std::sin(pan);

    for (std::uint32_t sample = 0; sample < numSamples; ++sample) {
        float mixed = 0.0f;
        for (auto& voice : voices_)
            mixed += renderVoice(voice);

        // A compact one-pole low-pass is sufficient for the first milestone;
        // resonance is kept deliberately conservative to remain stable for
        // arbitrary host automation.
        const float driven = std::tanh(mixed * (1.0f + resonance * 1.5f));
        filterLeft_ += coefficient * (driven - filterLeft_);
        filterRight_ += coefficient * (driven - filterRight_);
        const float outputLeft = filterLeft_ * master * panLeft;
        const float outputRight = filterRight_ * master * panRight;
        left[sample] = outputLeft;
        if (right)
            right[sample] = outputRight;
    }
}

} // namespace dn1
