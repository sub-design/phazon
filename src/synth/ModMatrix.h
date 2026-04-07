#pragma once

#include <array>

namespace ModMatrix
{
constexpr int kSlotCount = 8;

enum class Source : int
{
    Off = 0,
    Lfo,
    Pressure,
    Timbre,
    Velocity
};

enum class Destination : int
{
    Off = 0,
    Chaos,
    Force,
    Overtones,
    Cutoff,
    Drive,
    Body,
    Spring,
    Count
};

constexpr int kDestinationCount = static_cast<int> (Destination::Count);

struct Slot
{
    Source source = Source::Off;
    Destination destination = Destination::Off;
    float amount = 0.0f;
};

struct Config
{
    float lfoRate = 1.0f;
    float lfoDepth = 0.0f;
    int lfoShape = 0;
    std::array<Slot, kSlotCount> slots {};
};

using DeltaFrame = std::array<float, kDestinationCount>;

inline constexpr std::array<const char*, 5> sourceLabels {
    "Off", "LFO", "Pressure", "Timbre", "Velocity"
};

inline constexpr std::array<const char*, 8> destinationLabels {
    "Off", "Chaos", "Force", "Overtones", "Cutoff", "Drive", "Body", "Spring"
};

inline constexpr int toIndex (Source source) noexcept
{
    return static_cast<int> (source);
}

inline constexpr int toIndex (Destination destination) noexcept
{
    return static_cast<int> (destination);
}
}
