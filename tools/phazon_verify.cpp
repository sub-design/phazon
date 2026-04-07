#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <new>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "engine/NetworkIf.h"
#include "synth/PhazonSynthesiser.h"
#include "PostProcessChain.h"

namespace allocmon
{
std::atomic<long long> allocationCount { 0 };
std::atomic<long long> allocationBytes { 0 };
thread_local bool inAllocator = false;

void* allocate (std::size_t size)
{
    void* ptr = std::malloc (size == 0 ? 1 : size);
    if (ptr == nullptr)
        throw std::bad_alloc();

    if (!inAllocator)
    {
        inAllocator = true;
        allocationCount.fetch_add (1, std::memory_order_relaxed);
        allocationBytes.fetch_add ((long long) size, std::memory_order_relaxed);
        inAllocator = false;
    }

    return ptr;
}

struct Snapshot
{
    long long count = 0;
    long long bytes = 0;
};

Snapshot snapshot() noexcept
{
    return { allocationCount.load (std::memory_order_relaxed),
             allocationBytes.load (std::memory_order_relaxed) };
}

Snapshot deltaFrom (Snapshot before) noexcept
{
    auto after = snapshot();
    return { after.count - before.count, after.bytes - before.bytes };
}
}

void* operator new (std::size_t size)                    { return allocmon::allocate (size); }
void* operator new[] (std::size_t size)                  { return allocmon::allocate (size); }
void* operator new (std::size_t size, std::align_val_t)  { return allocmon::allocate (size); }
void* operator new[] (std::size_t size, std::align_val_t){ return allocmon::allocate (size); }
void operator delete (void* ptr) noexcept                { std::free (ptr); }
void operator delete[] (void* ptr) noexcept              { std::free (ptr); }
void operator delete (void* ptr, std::size_t) noexcept   { std::free (ptr); }
void operator delete[] (void* ptr, std::size_t) noexcept { std::free (ptr); }
void operator delete (void* ptr, std::align_val_t) noexcept { std::free (ptr); }
void operator delete[] (void* ptr, std::align_val_t) noexcept { std::free (ptr); }
void operator delete (void* ptr, std::size_t, std::align_val_t) noexcept { std::free (ptr); }
void operator delete[] (void* ptr, std::size_t, std::align_val_t) noexcept { std::free (ptr); }

namespace
{
constexpr int kBlockSize = 256;

struct TestResult
{
    bool ok = true;
    std::string name;
    std::string details;
};

float midiToHz (int midiNote) noexcept
{
    return 440.0f * std::pow (2.0f, ((float) midiNote - 69.0f) / 12.0f);
}

bool isFiniteNetwork (const NetworkIf& network) noexcept
{
    auto finiteValue = [] (float value) { return std::isfinite (value); };

    if (!finiteValue (network.bowVelocity)
        || !finiteValue (network.bow_phi)
        || !finiteValue (network.bow_dphi))
        return false;

    return std::all_of (network.x.begin(), network.x.end(), finiteValue)
        && std::all_of (network.x_prev.begin(), network.x_prev.end(), finiteValue);
}

void configureBaseParams (NetworkIf& network,
                          float sampleRate,
                          int midiNote,
                          float springDamping,
                          float massDamping,
                          float bowForce,
                          float nonlinearity,
                          float excitationRatio,
                          float pickupBase,
                          float octave,
                          float detune,
                          float chaos,
                          float velocity)
{
    network.setSampleRate (sampleRate);
    network.p_springDamping = springDamping;
    network.p_massDamping = massDamping;
    network.p_bowForce = bowForce;
    network.p_nonlinearity = nonlinearity;
    network.p_excitationRatio = excitationRatio;
    network.p_pickupBase = pickupBase;
    network.p_octave = octave;
    network.p_detune = detune;
    network.p_chaos = chaos;
    network.setModulationBaseValues (springDamping, massDamping, bowForce, nonlinearity,
                                     excitationRatio, pickupBase, chaos);
    network.setupNote ((float) midiNote, velocity, 0.0f, false);
}

std::vector<float> renderPluckedNote (float sampleRate, int midiNote)
{
    NetworkIf network;
    network.changeProfile (NetworkIf::Profile::OneDimensional);
    network.changeDimensions (8);
    configureBaseParams (network, sampleRate, midiNote,
                         0.03f, 0.002f, 0.0f, 0.0f, 0.24f, 0.70f, 0.0f, 0.0f, 0.0f, 0.0f);
    network.bowVelocity = 0.0f;
    NetworkIf::interpolation3Mult (network.x.data(), network.n_total,
                                   0.23f * (float) (network.n_total - 1), 0.008f);

    const int totalSamples = (int) std::round (sampleRate * 0.35f);
    std::vector<float> output ((size_t) totalSamples, 0.0f);
    for (int i = 0; i < totalSamples; ++i)
        output[(size_t) i] = network.processSample ({});

    return output;
}

double estimateFrequency (const std::vector<float>& signal,
                          float sampleRate,
                          double targetHz)
{
    const int start = std::min ((int) signal.size() / 10, (int) std::round (sampleRate * 0.03f));
    const int analysisLength = std::min ((int) signal.size() - start, 8192);
    if (analysisLength < 1024)
        return 0.0;

    std::vector<double> windowed ((size_t) analysisLength, 0.0);
    for (int i = 0; i < analysisLength; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::twoPi * (double) i / (double) (analysisLength - 1));
        windowed[(size_t) i] = (double) signal[(size_t) (start + i)] * w;
    }

    const int fftSize = juce::nextPowerOfTwo (analysisLength);
    juce::dsp::FFT fft ((int) std::log2 ((double) fftSize));
    std::vector<float> fftBuffer ((size_t) fftSize * 2u, 0.0f);

    for (int i = 0; i < analysisLength; ++i)
        fftBuffer[(size_t) i] = (float) windowed[(size_t) i];

    fft.performRealOnlyForwardTransform (fftBuffer.data());

    const double minHz = std::max (20.0, targetHz * 0.80);
    const double maxHz = std::min (sampleRate * 0.48, targetHz * 1.20);
    const int minBin = std::max (1, (int) std::floor (minHz * (double) fftSize / sampleRate));
    const int maxBin = std::min (fftSize / 2 - 1, (int) std::ceil (maxHz * (double) fftSize / sampleRate));

    int peakBin = minBin;
    double peakMag = 0.0;
    for (int bin = minBin; bin <= maxBin; ++bin)
    {
        const float re = fftBuffer[(size_t) bin * 2u];
        const float im = fftBuffer[(size_t) bin * 2u + 1u];
        const double mag = (double) re * re + (double) im * im;
        if (mag > peakMag)
        {
            peakMag = mag;
            peakBin = bin;
        }
    }

    if (peakBin <= minBin || peakBin >= maxBin)
        return (double) peakBin * sampleRate / (double) fftSize;

    const auto binMagnitude = [&] (int bin) {
        const float re = fftBuffer[(size_t) bin * 2u];
        const float im = fftBuffer[(size_t) bin * 2u + 1u];
        return (double) re * re + (double) im * im;
    };

    const double left = std::log (std::max (1.0e-20, binMagnitude (peakBin - 1)));
    const double center = std::log (std::max (1.0e-20, binMagnitude (peakBin)));
    const double right = std::log (std::max (1.0e-20, binMagnitude (peakBin + 1)));
    const double denom = left - 2.0 * center + right;
    const double delta = std::abs (denom) > 1.0e-12 ? 0.5 * (left - right) / denom : 0.0;

    return ((double) peakBin + delta) * sampleRate / (double) fftSize;
}

TestResult runTrace()
{
    NetworkIf network;
    network.changeProfile (NetworkIf::Profile::OneDimensional);
    network.changeDimensions (8);
    configureBaseParams (network, 48000.0f, 60,
                         0.18f, 0.015f, 0.42f, 0.33f, 0.31f, 0.70f, 0.0f, 0.0f, 0.0f, 0.5f);

    std::cout << std::fixed << std::setprecision (9);
    for (int i = 0; i < 128; ++i)
    {
        const float sample = network.processSample ({});
        std::cout << "TRACE " << i << ' ' << sample;
        for (float value : network.x)
            std::cout << ' ' << value;
        std::cout << '\n';
    }

    return { true, "trace", "ok" };
}

TestResult runPitchSweep()
{
    TestResult result { true, "pitch", {} };
    double worstCents = 0.0;
    int worstMidi = -1;
    int worstRate = 0;

    for (float sampleRate : { 44100.0f, 48000.0f })
    {
        for (int midi = 24; midi <= 96; ++midi)
        {
            const auto signal = renderPluckedNote (sampleRate, midi);
            const double targetHz = midiToHz (midi);
            const double measuredHz = estimateFrequency (signal, sampleRate, targetHz);
            if (!(measuredHz > 0.0))
            {
                result.ok = false;
                result.details = "frequency estimator failed";
                return result;
            }

            const double cents = 1200.0 * std::log2 (measuredHz / targetHz);
            if (std::abs (cents) > std::abs (worstCents))
            {
                worstCents = cents;
                worstMidi = midi;
                worstRate = (int) sampleRate;
            }
        }
    }

    std::ostringstream details;
    details << std::fixed << std::setprecision (3)
            << "worst_cents=" << worstCents
            << " midi=" << worstMidi
            << " sample_rate=" << worstRate;
    result.details = details.str();
    result.ok = std::abs (worstCents) <= 2.0;
    return result;
}

TestResult runFuzz()
{
    std::mt19937 rng (0x50485a4eu);
    std::uniform_real_distribution<float> zeroToOne (0.0f, 1.0f);
    std::uniform_real_distribution<float> minusOneToOne (-1.0f, 1.0f);
    std::uniform_real_distribution<float> detuneDist (-1.0f, 1.0f);
    std::uniform_real_distribution<float> octaveDist (-2.0f, 2.0f);
    std::uniform_int_distribution<int> midiDist (24, 96);
    std::uniform_int_distribution<int> profileDist (0, 2);
    std::uniform_int_distribution<int> dimensionDist (4, 25);

    TestResult result { true, "fuzz", {} };
    NetworkIf network;
    allocmon::Snapshot before = allocmon::snapshot();

    constexpr float sampleRate = 44100.0f;
    network.setSampleRate (sampleRate);
    const int totalSamples = (int) std::round (sampleRate * 10.0f);

    for (int sampleIndex = 0; sampleIndex < totalSamples; ++sampleIndex)
    {
        if (sampleIndex % 64 == 0)
        {
            const int profileIndex = profileDist (rng);
            const auto profile = profileIndex == 0 ? NetworkIf::Profile::OneDimensional
                               : profileIndex == 1 ? NetworkIf::Profile::TwoDimensional
                                                   : NetworkIf::Profile::Eco;
            network.changeProfile (profile);
            if (profile != NetworkIf::Profile::Eco)
                network.changeDimensions (dimensionDist (rng));

            const int midi = midiDist (rng);
            const float springDamping = zeroToOne (rng);
            const float massDamping = zeroToOne (rng);
            const float bowForce = zeroToOne (rng);
            const float nonlinearity = zeroToOne (rng);
            const float excitationRatio = zeroToOne (rng);
            const float pickupBase = zeroToOne (rng);
            const float octave = octaveDist (rng);
            const float detune = detuneDist (rng);
            const float chaos = zeroToOne (rng);
            const float velocity = zeroToOne (rng);

            configureBaseParams (network, sampleRate, midi, springDamping, massDamping,
                                 bowForce, nonlinearity, excitationRatio, pickupBase,
                                 octave, detune, chaos, velocity);

            if (zeroToOne (rng) > 0.5f)
            {
                network.bowVelocity = 0.0f;
                NetworkIf::interpolation3Mult (network.x.data(), network.n_total,
                                               zeroToOne (rng) * (float) (network.n_total - 1),
                                               minusOneToOne (rng) * 0.02f);
            }
        }

        ModMatrix::DeltaFrame delta {};
        for (float& value : delta)
            value = minusOneToOne (rng);

        const float output = network.processSample (delta);
        if (!std::isfinite (output) || !isFiniteNetwork (network))
        {
            result.ok = false;
            result.details = "NaN/Inf detected during fuzz";
            return result;
        }
    }

    const auto allocDelta = allocmon::deltaFrom (before);
    std::ostringstream details;
    details << "allocations=" << allocDelta.count << " bytes=" << allocDelta.bytes;
    result.details = details.str();
    result.ok = allocDelta.count == 0;
    return result;
}

PhysicsParams makeCpuParams()
{
    PhysicsParams params;
    params.springDamping = 0.18f;
    params.massDamping = 0.02f;
    params.bowForce = 0.46f;
    params.nonlinearity = 0.28f;
    params.excitationRatio = 0.28f;
    params.bowWidth = 0.05f;
    params.drive = 0.22f;
    params.dimensions = 8;
    params.massProfile = 0;
    params.attack = 0.005f;
    params.release = 0.4f;
    params.octave = 0.0f;
    params.detune = 0.0f;
    params.chaos = 0.0f;
    return params;
}

TestResult runCpuBench()
{
    constexpr float sampleRate = 44100.0f;
    constexpr int warmupBlocks = 32;
    constexpr int measuredBlocks = 512;

    TestResult result { true, "cpu", {} };

    PhazonSynthesiser synth;
    synth.setCurrentPlaybackSampleRate (sampleRate);
    synth.prepare (kBlockSize);
    synth.setEcoMode (false);
    synth.setExcitationMode (NetworkVoice::ExcitationMode::Bow);
    synth.setBaseParams (makeCpuParams());
    synth.setModMatrixConfig ({});

    PostProcessChain post;
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) kBlockSize;
    spec.numChannels = 2;
    post.prepare (spec);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer noteOns;
    for (int voice = 0; voice < 8; ++voice)
        noteOns.addEvent (juce::MidiMessage::noteOn (2 + voice, 48 + voice * 3, (juce::uint8) 100), 0);

    for (int block = 0; block < warmupBlocks; ++block)
    {
        buffer.clear();
        synth.renderNextBlock (buffer, block == 0 ? noteOns : juce::MidiBuffer(), 0, kBlockSize);
        post.process (buffer, 0.22f, 12000.0f, 0.05f, 0.15f, 0.35f, 0.0f,
                      synth.getDriveModulationBuffer().data(),
                      synth.getCutoffModulationBuffer().data(),
                      kBlockSize);
    }

    auto runPass = [&] (bool withPost) {
        allocmon::Snapshot before = allocmon::snapshot();
        auto start = std::chrono::steady_clock::now();

        for (int block = 0; block < measuredBlocks; ++block)
        {
            buffer.clear();
            synth.renderNextBlock (buffer, juce::MidiBuffer(), 0, kBlockSize);
            if (withPost)
            {
                post.process (buffer, 0.22f, 12000.0f, 0.05f, 0.15f, 0.35f, 0.0f,
                              synth.getDriveModulationBuffer().data(),
                              synth.getCutoffModulationBuffer().data(),
                              kBlockSize);
            }

            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                const auto* data = buffer.getReadPointer (ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i)
                {
                    if (!std::isfinite (data[i]))
                        throw std::runtime_error ("NaN/Inf during CPU benchmark");
                }
            }
        }

        const auto elapsed = std::chrono::steady_clock::now() - start;
        const auto allocDelta = allocmon::deltaFrom (before);
        const double elapsedSeconds = std::chrono::duration<double> (elapsed).count();
        const double renderedSeconds = (double) measuredBlocks * (double) kBlockSize / sampleRate;
        return std::tuple<double, long long, long long> {
            elapsedSeconds / renderedSeconds * 100.0,
            allocDelta.count,
            allocDelta.bytes
        };
    };

    const auto [synthPercent, synthAllocations, synthBytes] = runPass (false);
    const auto [fullPercent, fullAllocations, fullBytes] = runPass (true);

    std::ostringstream details;
    details << std::fixed << std::setprecision (3)
            << "synth_only_percent=" << synthPercent
            << " full_chain_percent=" << fullPercent
            << " synth_allocations=" << synthAllocations
            << " full_allocations=" << fullAllocations;
    result.details = details.str();
    result.ok = synthPercent < 15.0
             && synthAllocations == 0
             && fullAllocations == 0
             && synthBytes == 0
             && fullBytes == 0;
    return result;
}

void printResult (const TestResult& result)
{
    std::cout << (result.ok ? "PASS " : "FAIL ")
              << result.name << ' '
              << result.details << '\n';
}
}

int main (int argc, char** argv)
{
    try
    {
        const std::string command = argc > 1 ? argv[1] : "full";
        if (command == "trace")
            return runTrace().ok ? 0 : 1;

        std::vector<TestResult> results;

        if (command == "pitch" || command == "full")
            results.push_back (runPitchSweep());
        if (command == "fuzz" || command == "full")
            results.push_back (runFuzz());
        if (command == "cpu" || command == "full")
            results.push_back (runCpuBench());

        if (results.empty())
        {
            std::cerr << "unknown command: " << command << '\n';
            return 2;
        }

        bool ok = true;
        for (const auto& result : results)
        {
            printResult (result);
            ok = ok && result.ok;
        }

        return ok ? 0 : 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR " << e.what() << '\n';
        return 1;
    }
}
