#include "engine/dn1_engine.hpp"

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

    std::cout << "dn1_engine tests passed\n";
    return 0;
}
