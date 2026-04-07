#pragma once

#include <array>

/** Centralised parameter ID strings — single source of truth for APVTS keys. */
namespace ParamIDs
{
    // -------------------------------------------------------------------------
    // Physical model core
    // -------------------------------------------------------------------------
    inline constexpr const char* springDamping   = "springDamping";
    inline constexpr const char* massDamping     = "massDamping";
    inline constexpr const char* excitationRatio    = "excitationRatio";
    inline constexpr const char* excitationLocation = "excitationLocation";
    inline constexpr const char* bowForce           = "bowForce";
    inline constexpr const char* bowWidth        = "bowWidth";
    inline constexpr const char* nonlinearity    = "nonlinearity";
    inline constexpr const char* drive           = "drive";
    inline constexpr const char* dimensions      = "dimensions";
    inline constexpr const char* massProfile     = "massProfile";
    inline constexpr const char* springProfile   = "springProfile";

    // -------------------------------------------------------------------------
    // Automation wrappers — physical core (per-param MIN/MAX/RATE/TYPE/SYNC/INV/LFOmode)
    // -------------------------------------------------------------------------
    inline constexpr const char* springDampingMIN     = "springDampingMIN";
    inline constexpr const char* springDampingMAX     = "springDampingMAX";
    inline constexpr const char* springDampingRATE    = "springDampingRATE";
    inline constexpr const char* springDampingTYPE    = "springDampingTYPE";
    inline constexpr const char* springDampingSYNC    = "springDampingSYNC";
    inline constexpr const char* springDampingINV     = "springDampingINV";
    inline constexpr const char* springDampingLFOmode = "springDampingLFOmode";

    inline constexpr const char* massDampingMIN     = "massDampingMIN";
    inline constexpr const char* massDampingMAX     = "massDampingMAX";
    inline constexpr const char* massDampingRATE    = "massDampingRATE";
    inline constexpr const char* massDampingTYPE    = "massDampingTYPE";
    inline constexpr const char* massDampingSYNC    = "massDampingSYNC";
    inline constexpr const char* massDampingINV     = "massDampingINV";
    inline constexpr const char* massDampingLFOmode = "massDampingLFOmode";

    inline constexpr const char* bowForceMIN     = "bowForceMIN";
    inline constexpr const char* bowForceMAX     = "bowForceMAX";
    inline constexpr const char* bowForceRATE    = "bowForceRATE";
    inline constexpr const char* bowForceTYPE    = "bowForceTYPE";
    inline constexpr const char* bowForceSYNC    = "bowForceSYNC";
    inline constexpr const char* bowForceINV     = "bowForceINV";
    inline constexpr const char* bowForceLFOmode = "bowForceLFOmode";

    inline constexpr const char* nonlinearityMIN     = "nonlinearityMIN";
    inline constexpr const char* nonlinearityMAX     = "nonlinearityMAX";
    inline constexpr const char* nonlinearityRATE    = "nonlinearityRATE";
    inline constexpr const char* nonlinearityTYPE    = "nonlinearityTYPE";
    inline constexpr const char* nonlinearitySYNC    = "nonlinearitySYNC";
    inline constexpr const char* nonlinearityINV     = "nonlinearityINV";
    inline constexpr const char* nonlinearityLFOmode = "nonlinearityLFOmode";

    inline constexpr const char* excitationRatioMIN     = "excitationRatioMIN";
    inline constexpr const char* excitationRatioMAX     = "excitationRatioMAX";
    inline constexpr const char* excitationRatioRATE    = "excitationRatioRATE";
    inline constexpr const char* excitationRatioTYPE    = "excitationRatioTYPE";
    inline constexpr const char* excitationRatioSYNC    = "excitationRatioSYNC";
    inline constexpr const char* excitationRatioINV     = "excitationRatioINV";
    inline constexpr const char* excitationRatioLFOmode = "excitationRatioLFOmode";

    inline constexpr const char* driveMIN     = "driveMIN";
    inline constexpr const char* driveMAX     = "driveMAX";
    inline constexpr const char* driveRATE    = "driveRATE";
    inline constexpr const char* driveTYPE    = "driveTYPE";
    inline constexpr const char* driveSYNC    = "driveSYNC";
    inline constexpr const char* driveINV     = "driveINV";
    inline constexpr const char* driveLFOmode = "driveLFOmode";

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
    // Output
    // -------------------------------------------------------------------------
    inline constexpr const char* outputGain = "outputGain";

    // -------------------------------------------------------------------------
    // System / global
    // -------------------------------------------------------------------------
    inline constexpr const char* chaos          = "chaos";
    inline constexpr const char* ecoMode        = "ecoMode";
    inline constexpr const char* excitationMode = "excitationMode";
    inline constexpr const char* mpeMode        = "mpeMode";
    inline constexpr const char* automationMode = "automationMode";
    inline constexpr const char* driftAmount    = "driftAmount";

    // -------------------------------------------------------------------------
    // Mod matrix
    // -------------------------------------------------------------------------
    inline constexpr const char* modLfoRate  = "modLfoRate";
    inline constexpr const char* modLfoDepth = "modLfoDepth";
    inline constexpr const char* modLfoShape = "modLfoShape";

    inline constexpr std::array<const char*, 8> modSlotSource {
        "modSlot1Source", "modSlot2Source", "modSlot3Source", "modSlot4Source",
        "modSlot5Source", "modSlot6Source", "modSlot7Source", "modSlot8Source"
    };

    inline constexpr std::array<const char*, 8> modSlotDestination {
        "modSlot1Destination", "modSlot2Destination", "modSlot3Destination", "modSlot4Destination",
        "modSlot5Destination", "modSlot6Destination", "modSlot7Destination", "modSlot8Destination"
    };

    inline constexpr std::array<const char*, 8> modSlotAmount {
        "modSlot1Amount", "modSlot2Amount", "modSlot3Amount", "modSlot4Amount",
        "modSlot5Amount", "modSlot6Amount", "modSlot7Amount", "modSlot8Amount"
    };

} // namespace ParamIDs
