#pragma once
#include <JuceHeader.h>
#include <array>
#include <random>
#include <cmath>

struct Grain
{
    bool active = false;
    double sourcePos = 0.0;
    double pitchRatio = 1.0;
    int lengthInSamples = 0;
    int age = 0;
    float pan = 0.5f;
    float gain = 0.6f;
};

// Simple granular playback engine: holds a loaded (mono) sample and
// continuously spawns overlapping grains from it based on the params below.
class GranularEngine
{
public:
    static constexpr int maxGrains = 48;

    void prepare(double sr)
    {
        sampleRate = sr;
        rng.seed(std::random_device{}());
    }

    void setSample(juce::AudioBuffer<float>&& newBuffer)
    {
        const juce::SpinLock::ScopedLockType lock(sampleLock);
        sampleBuffer = std::move(newBuffer);
        for (auto& g : grains)
            g.active = false;
        spawnAccumulator = 0.0;
    }

    void setParams(float grainSizeMs_, float densityHz_, float position_,
                   float posSpread_, float pitchSpreadSemis_, float width_)
    {
        grainSizeMs = grainSizeMs_;
        densityHz = densityHz_;
        position = position_;
        posSpread = posSpread_;
        pitchSpreadSemis = pitchSpreadSemis_;
        width = width_;
    }

    // Adds granular output into 'out' (does not clear it first).
    void process(juce::AudioBuffer<float>& out)
    {
        const juce::SpinLock::ScopedTryLockType lock(sampleLock);
        if (!lock.isLocked() || sampleBuffer.getNumSamples() == 0)
            return;

        const int numOut = out.getNumSamples();
        const int srcLength = sampleBuffer.getNumSamples();
        const double spawnInterval = sampleRate / (double) juce::jmax(0.5f, densityHz);
        const auto* src = sampleBuffer.getReadPointer(0);

        for (int i = 0; i < numOut; ++i)
        {
            spawnAccumulator += 1.0;
            if (spawnAccumulator >= spawnInterval)
            {
                spawnAccumulator -= spawnInterval;
                spawnGrain(srcLength);
            }

            float left = 0.0f, right = 0.0f;

            for (auto& g : grains)
            {
                if (!g.active)
                    continue;

                const int i0 = (int) g.sourcePos;
                if (i0 < 0 || i0 >= srcLength - 1)
                {
                    g.active = false;
                    continue;
                }

                const float frac = (float) (g.sourcePos - i0);
                const float s0 = src[i0];
                const float s1 = src[i0 + 1];
                const float sample = s0 + frac * (s1 - s0);

                const float phase = (float) g.age / (float) g.lengthInSamples;
                const float env = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

                const float wet = sample * env * g.gain;
                const float panRad = g.pan * juce::MathConstants<float>::halfPi;
                left += wet * std::cos(panRad);
                right += wet * std::sin(panRad);

                g.sourcePos += g.pitchRatio;
                ++g.age;
                if (g.age >= g.lengthInSamples)
                    g.active = false;
            }

            out.addSample(0, i, left);
            if (out.getNumChannels() > 1)
                out.addSample(1, i, right);
        }
    }

private:
    void spawnGrain(int srcLength)
    {
        const int lengthSamples = juce::jmax(16, (int) (grainSizeMs * 0.001 * sampleRate));
        if (srcLength - lengthSamples - 2 <= 0)
            return;

        for (auto& g : grains)
        {
            if (g.active)
                continue;

            std::uniform_real_distribution<float> unit(-1.0f, 1.0f);

            const float jitter = unit(rng) * posSpread * (float) srcLength;
            const float basePos = position * (float) (srcLength - lengthSamples);
            const float startPos = juce::jlimit(0.0f, (float) (srcLength - lengthSamples - 2), basePos + jitter);

            g.sourcePos = startPos;
            g.lengthInSamples = lengthSamples;
            g.age = 0;
            g.pitchRatio = std::pow(2.0, (double) (unit(rng) * pitchSpreadSemis) / 12.0);
            g.pan = 0.5f + unit(rng) * 0.5f * width;
            g.gain = 0.6f;
            g.active = true;
            return;
        }
    }

    juce::AudioBuffer<float> sampleBuffer;
    juce::SpinLock sampleLock;
    double sampleRate = 44100.0;

    std::array<Grain, maxGrains> grains;
    double spawnAccumulator = 0.0;

    float grainSizeMs = 80.0f, densityHz = 20.0f, position = 0.0f;
    float posSpread = 0.1f, pitchSpreadSemis = 2.0f, width = 0.7f;

    std::mt19937 rng;
};
