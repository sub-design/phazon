#pragma once

/** Centralised parameter ID strings — single source of truth for APVTS keys. */
namespace ParamIDs
{
    // -------------------------------------------------------------------------
    // Physical model core
    // -------------------------------------------------------------------------
    inline constexpr const char* springDamping   = "springDamping";
    inline constexpr const char* massDamping     = "massDamping";
    inline constexpr const char* excitationRatio = "excitationRatio";
    inline constexpr const char* bowForce        = "bowForce";
    inline constexpr const char* bowWidth        = "bowWidth";
    inline constexpr const char* nonlinearity    = "nonlinearity";
    inline constexpr const char* drive           = "drive";
    inline constexpr const char* dimensions      = "dimensions";
    inline constexpr const char* massProfile     = "massProfile";
    inline constexpr const char* springProfile   = "springProfile";

    // -------------------------------------------------------------------------
    // Pitch / note setup
    // -------------------------------------------------------------------------
    inline constexpr const char* octave      = "octave";
    inline constexpr const char* detune      = "detune";
    inline constexpr const char* attack      = "attack";
    inline constexpr const char* release     = "release";
    inline constexpr const char* monoMode    = "monoMode";
    inline constexpr const char* releaseLock = "releaseLock";

    // -------------------------------------------------------------------------
    // Movement LFO (bow position modulation)
    // -------------------------------------------------------------------------
    inline constexpr const char* MovementFREQ    = "MovementFREQ";
    inline constexpr const char* MovementDEPTH   = "MovementDEPTH";
    inline constexpr const char* MovementSYNC    = "MovementSYNC";
    inline constexpr const char* MovementTYPE    = "MovementTYPE";
    inline constexpr const char* MovementINV     = "MovementINV";
    inline constexpr const char* MovementMIN     = "MovementMIN";
    inline constexpr const char* MovementMAX     = "MovementMAX";
    inline constexpr const char* MovementRATE    = "MovementRATE";
    inline constexpr const char* MovementLFOmode = "MovementLFOmode";

    // -------------------------------------------------------------------------
    // Modulation LFO (pickup position modulation)
    // -------------------------------------------------------------------------
    inline constexpr const char* ModulationFREQ    = "ModulationFREQ";
    inline constexpr const char* ModulationDEPTH   = "ModulationDEPTH";
    inline constexpr const char* ModulationSYNC    = "ModulationSYNC";
    inline constexpr const char* ModulationTYPE    = "ModulationTYPE";
    inline constexpr const char* ModulationINV     = "ModulationINV";
    inline constexpr const char* ModulationMIN     = "ModulationMIN";
    inline constexpr const char* ModulationMAX     = "ModulationMAX";
    inline constexpr const char* ModulationRATE    = "ModulationRATE";
    inline constexpr const char* ModulationLFOmode = "ModulationLFOmode";

    // -------------------------------------------------------------------------
    // Vibrato LFO (spring stiffness modulation)
    // -------------------------------------------------------------------------
    inline constexpr const char* vibratoFreq    = "vibratoFreq";
    inline constexpr const char* vibratoDepth   = "vibratoDepth";
    inline constexpr const char* vibratoSync    = "vibratoSync";
    inline constexpr const char* VibratoTYPE    = "VibratoTYPE";
    inline constexpr const char* VibratoINV     = "VibratoINV";
    inline constexpr const char* VibratoMIN     = "VibratoMIN";
    inline constexpr const char* VibratoMAX     = "VibratoMAX";
    inline constexpr const char* VibratoRATE    = "VibratoRATE";
    inline constexpr const char* VibratoLFOmode = "VibratoLFOmode";

    // -------------------------------------------------------------------------
    // Filter
    // -------------------------------------------------------------------------
    inline constexpr const char* filterCutoff    = "filterCutoff";
    inline constexpr const char* filterResonance = "filterResonance";
    inline constexpr const char* filterType      = "filterType";
    inline constexpr const char* filterMin       = "filterMin";
    inline constexpr const char* filterMax       = "filterMax";
    inline constexpr const char* filterInv       = "filterInv";
    inline constexpr const char* filterLock      = "filterLock";

    // -------------------------------------------------------------------------
    // Space / reverb
    // -------------------------------------------------------------------------
    inline constexpr const char* SpaceMix     = "SpaceMix";
    inline constexpr const char* SpaceSize    = "SpaceSize";
    inline constexpr const char* SpaceMIN     = "SpaceMIN";
    inline constexpr const char* SpaceMAX     = "SpaceMAX";
    inline constexpr const char* SpaceRATE    = "SpaceRATE";
    inline constexpr const char* SpaceTYPE    = "SpaceTYPE";
    inline constexpr const char* SpaceSYNC    = "SpaceSYNC";
    inline constexpr const char* SpaceINV     = "SpaceINV";
    inline constexpr const char* SpaceLFOmode = "SpaceLFOmode";

    // -------------------------------------------------------------------------
    // System / global
    // -------------------------------------------------------------------------
    inline constexpr const char* chaos          = "chaos";
    inline constexpr const char* ecoMode        = "ecoMode";
    inline constexpr const char* mpeMode        = "mpeMode";
    inline constexpr const char* automationMode = "automationMode";
    inline constexpr const char* driftAmount    = "driftAmount";

} // namespace ParamIDs
