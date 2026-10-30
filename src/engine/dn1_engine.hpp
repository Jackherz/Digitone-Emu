#pragma once

#include "dn1_parameters.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace dn1 {

class DigitoneEngine {
public:
    static constexpr std::size_t kVoiceCount = 8;
    static constexpr std::size_t kOperatorCount = 4;

    DigitoneEngine();

    void prepare(double sampleRate, std::uint32_t maxBlockSize) noexcept;
    void reset() noexcept;

    void setParameter(ParameterId id, float normalized) noexcept;
    [[nodiscard]] float getParameter(ParameterId id) const noexcept;

    // The event methods are intentionally independent of VST3. This is the
    // clean-room signal engine boundary and is also useful for an eventual
    // standalone app or AU adapter.
    void noteOn(std::uint8_t channel, std::int32_t note, float velocity,
                std::int32_t noteId = -1) noexcept;
    void noteOff(std::uint8_t channel, std::int32_t note,
                 std::int32_t noteId = -1) noexcept;
    void pitchBend(std::uint8_t channel, float normalized) noexcept;
    void polyPressure(std::uint8_t channel, std::int32_t note,
                      float pressure) noexcept;
    void controlChange(std::uint8_t channel, std::int32_t controller,
                       float normalized) noexcept;
    void allNotesOff() noexcept;

    // Renders into non-interleaved float buffers. right may be null for a
    // mono host; the engine never allocates on this real-time path.
    void render(float* left, float* right, std::uint32_t numSamples) noexcept;

private:
    enum class EnvelopeStage : std::uint8_t {
        Idle,
        Attack,
        Decay,
        Sustain,
        Release,
    };

    struct Envelope {
        double sampleRate {48000.0};
        float attackSeconds {0.01f};
        float decaySeconds {0.2f};
        float sustainLevel {0.8f};
        float releaseSeconds {0.2f};
        EnvelopeStage stage {EnvelopeStage::Idle};
        float level {0.0f};
        float stageStartLevel {0.0f};
        std::uint32_t samplesInStage {0};
        std::uint32_t stageLength {1};

        void configure(double newSampleRate, float attack, float decay,
                       float sustain, float release) noexcept;
        void start() noexcept;
        void release() noexcept;
        [[nodiscard]] float next() noexcept;
        [[nodiscard]] bool idle() const noexcept;
    };

    struct Operator {
        double phase {0.0};
        float lastOutput {0.0f};
        Envelope envelope;
    };

    struct Voice {
        bool active {false};
        bool keyDown {false};
        bool pendingRelease {false};
        std::uint8_t channel {0};
        std::int32_t note {-1};
        std::int32_t noteId {-1};
        float velocity {0.0f};
        float pressure {0.0f};
        std::uint64_t age {0};
        std::array<Operator, kOperatorCount> operators {};
        Envelope amplifier;
    };

    [[nodiscard]] std::size_t allocateVoice(std::uint8_t channel,
                                            std::int32_t note,
                                            std::int32_t noteId) noexcept;
    void configureVoice(Voice& voice) noexcept;
    void startVoice(Voice& voice, std::uint8_t channel, std::int32_t note,
                    float velocity, std::int32_t noteId) noexcept;
    void releaseVoice(Voice& voice) noexcept;
    void releaseMatching(std::uint8_t channel, std::int32_t note,
                         std::int32_t noteId) noexcept;
    [[nodiscard]] float renderVoice(Voice& voice) noexcept;

    [[nodiscard]] float normalized(ParameterId id) const noexcept;
    [[nodiscard]] float ratio(ParameterId id) const noexcept;
    [[nodiscard]] float fineSemitones(ParameterId id) const noexcept;
    [[nodiscard]] float seconds(ParameterId id) const noexcept;
    [[nodiscard]] float cutoffHz() const noexcept;
    [[nodiscard]] float operatorLevel(ParameterId id) const noexcept;

    std::array<float, static_cast<std::size_t>(ParameterId::Count)> parameters_ {};
    std::array<Voice, kVoiceCount> voices_ {};
    std::array<float, 16> pitchBend_ {};
    std::array<float, 16> modWheel_ {};
    std::array<bool, 16> sustainPedal_ {};

    double sampleRate_ {48000.0};
    std::uint32_t maxBlockSize_ {0};
    std::uint64_t voiceAge_ {0};
    float filterLeft_ {0.0f};
    float filterRight_ {0.0f};
};

} // namespace dn1
