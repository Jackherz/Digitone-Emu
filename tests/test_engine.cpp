#include "engine/dn1_engine.hpp"
#include "engine/dn1_presets.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {

bool finiteBuffer(const std::vector<float>& buffer)
{
    return std::all_of(buffer.begin(), buffer.end(),
                       [](float sample) { return std::isfinite(sample); });
}

float peak(const std::vector<float>& buffer)
{
    float result = 0.0f;
    for (const auto sample : buffer)
        result = std::max(result, std::abs(sample));
    return result;
}

} // namespace

int main()
{
    using dn1::DigitoneEngine;
    using dn1::ParameterId;

    constexpr std::uint32_t sampleRate = 48000;
    DigitoneEngine engine;
    engine.prepare(sampleRate, 512);

    std::vector<float> left(sampleRate / 2);
    std::vector<float> right(sampleRate / 2);
    engine.noteOn(0, 60, 0.9f, 42);
    engine.render(left.data(), right.data(), static_cast<std::uint32_t>(left.size()));

    assert(finiteBuffer(left));
    assert(finiteBuffer(right));
    assert(peak(left) > 0.0001f);
    assert(peak(right) > 0.0001f);

    engine.noteOff(0, 60, 42);
    std::vector<float> tail(sampleRate);
    std::vector<float> tailRight(sampleRate);
    engine.render(tail.data(), tailRight.data(), static_cast<std::uint32_t>(tail.size()));
    assert(finiteBuffer(tail));
    assert(finiteBuffer(tailRight));
    assert(peak(tail) > 0.0f);
    assert(std::abs(tail.back()) < 0.001f);

    // Exercise every routing without relying on a particular waveform level.
    for (int algorithm = 0; algorithm < 8; ++algorithm) {
        engine.reset();
        engine.setParameter(ParameterId::Algorithm, algorithm / 7.0f);
        engine.noteOn(0, 48 + algorithm, 0.75f);
        std::vector<float> block(512);
        std::vector<float> blockRight(512);
        engine.render(block.data(), blockRight.data(), 512);
        assert(finiteBuffer(block));
        assert(finiteBuffer(blockRight));
        assert(peak(block) > 0.00001f);
    }

    // The starter bank is deliberately independent of the VST3 layer so the
    // same original sounds can be used by a future browser or standalone app.
    const auto& bank = dn1::presetBank();
    assert(bank.size() == dn1::presetCount());
    for (std::size_t index = 0; index < bank.size(); ++index) {
        const auto& patch = bank[index];
        assert(patch.name != nullptr);
        assert(patch.name[0] != '\0');
        for (const auto value : patch.values)
            assert(value >= 0.0f && value <= 1.0f);

        dn1::applyPreset(engine, patch);
        engine.reset();
        engine.noteOn(0, 60, 0.8f);
        std::vector<float> patchLeft(1024);
        std::vector<float> patchRight(1024);
        engine.render(patchLeft.data(), patchRight.data(),
                      static_cast<std::uint32_t>(patchLeft.size()));
        assert(finiteBuffer(patchLeft));
        assert(finiteBuffer(patchRight));
        assert(peak(patchLeft) > 0.00001f);
    }

    std::cout << "dn1_engine tests passed\n";
    return 0;
}
