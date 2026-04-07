#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include "../synth/ModMatrix.h"

//==============================================================================
/**
 * NetworkIf — implicit nonlinear lattice solver.
 *
 * Each voice owns one NetworkIf instance. Per-sample pipeline:
 *   1. refreshCoefficients()     (on param change or note-on)
 *   2. calculateFullSystem*()    (assemble A, b)
 *   3. bow()                     (compute friction force, update bowState)
 *   4. NR()                      (3-iteration Newton–Raphson solve)
 *   5. getROutput(pickupPos)      (cubic interpolation readout)
 *
 * Equation of motion (implicit Newmark β=1/4 → backward Euler velocity):
 *   M·ẍ + C·ẋ + K·x = G^T · φ(v_bow − B·(x^{n+1}−x^n)/dt)
 *
 * Discrete system:
 *   A  = M/dt² + C/dt + K
 *   b  = M·(2x^n − x^{n-1})/dt²  +  C·x^n/dt
 *
 * Residual:
 *   R(x) = A·x − b − G^T · φ(v_bow − B·(x−x^n)/dt)
 *
 * Jacobian (rank-1 update):
 *   J = A + (φ'(u)/dt) · w·wᵀ      where w = cubic weight vector at bowPos
 */
class NetworkIf
{
public:
    // -----------------------------------------------------------------------
    // Topology selection
    // -----------------------------------------------------------------------
    enum class Profile { OneDimensional, TwoDimensional, Eco };

    static constexpr int N_ECO         = 4;    ///< Fixed node count for Eco mode
    static constexpr int N_DEFAULT_1D  = 8;    ///< Default 1-D chain size
    static constexpr int N_DEFAULT_2D  = 4;    ///< Default 2-D grid side  (4×4 = 16 nodes)
    static constexpr int N_MAX_TOTAL   = 64;   ///< Hard cap (8×8 2-D or 64 1-D)

    // -----------------------------------------------------------------------
    // Construction / lifecycle
    // -----------------------------------------------------------------------
    NetworkIf();

    /** Call on note-on.  pitch = MIDI note number, velocity ∈ [0,1]. */
    void setupNote (float midiNote, float velocity, float tuning, bool isMono);

    /** Call on note-off (can keep decaying). */
    void stopNote();

    /** Zero displacement / velocity state.
     *  @param hard  if true, also clears the bow velocity. */
    void resetStates (bool hard = false, bool clearBow = false);

    // -----------------------------------------------------------------------
    // Configuration  (call before or instead of per-sample refresh)
    // -----------------------------------------------------------------------
    /** Recalculate all physical coefficients from UI parameter values.
     *  Must be called whenever any p_* field changes or at note-on. */
    void refreshCoefficients();

    /** Resize state vectors to n nodes and zero them. */
    void changeDimensions (int n);

    /** Switch topology and resize as needed. */
    void changeProfile (Profile profile);

    /** Generic parameter setter (matches setParameterWithID in original binary).
     *  id maps to the p_* fields — see NetworkParamID enum below. */
    void setParameterWithID (int id, float value, bool notify = true);
    void setModulationBaseValues (float springDamping,
                                  float massDamping,
                                  float bowForce,
                                  float nonlinearity,
                                  float excitationRatio,
                                  float pickupBase,
                                  float chaos);

    void setSampleRate (float sr);

    // -----------------------------------------------------------------------
    // Per-sample pipeline
    // -----------------------------------------------------------------------

    /** Assemble A and b for a 1-D spring chain (N nodes, fixed-end boundary). */
    void calculateFullSystem1D();

    /** Assemble A and b for a 2-D rectangular lattice (Nx × Ny nodes). */
    void calculateFullSystem2D();

    /** Assemble A and b for the reduced 4-node Eco system. */
    void calculateFullSystemEco();

    /** Compute bow friction force and scatter into bowForce_vec[].
     *  Uses current x, x_prev and bowPos/bowVelocity.
     *  Stores φ(v_rel) and dphi in bow state for NR(). */
    void bow();

    /** 3-iteration Newton–Raphson solve.
     *  Updates x[] (advances one time step).
     *  Moves old x → x_prev. */
    void NR();

    /** Cubic interpolation readout at continuous pickup position.
     *  @param pos  normalised position ∈ [0, 1]
     *  @return     interpolated displacement  */
    float getROutput (float pos);

    // -----------------------------------------------------------------------
    // Interpolation operators  (public for unit-testing / external use)
    // -----------------------------------------------------------------------

    /** Cubic (Catmull–Rom) read operator.
     *  @param arr  source array of length n
     *  @param n    array length
     *  @param pos  continuous position ∈ [0, n−1]
     *  @return     interpolated value */
    static float interpolation3 (const float* arr, int n, float pos);

    /** Cubic (Catmull–Rom) write/scatter operator — distributes value onto
     *  4 adjacent grid points with the same kernel weights.
     *  @param arr    target array of length n  (modified in-place)
     *  @param n      array length
     *  @param pos    continuous position ∈ [0, n−1]
     *  @param value  scalar to scatter */
    static void interpolation3Mult (float* arr, int n, float pos, float value);

    // -----------------------------------------------------------------------
    // Block-level interface  (called by NetworkVoice::renderNextBlock)
    // -----------------------------------------------------------------------

    /** Run the full per-sample pipeline (calculateFullSystem / bow / NR /
     *  getROutput) for @p numSamples and write mono output to @p output. */
    void renderNextBlock (float* output, int numSamples);
    float processSample (const ModMatrix::DeltaFrame& modDelta);

    /** Update pitch mid-note without resetting state (for MPE pitchbend). */
    void updatePitch (float midiNote, float tuning);

    // -----------------------------------------------------------------------
    // UI parameters  (write from MPESynthVoice / setParameterWithID)
    // -----------------------------------------------------------------------
    float p_springDamping   = 0.20f;  ///< β — stiffness-proportional damping [0..1]
    float p_massDamping     = 0.02f;  ///< α — mass-proportional  damping    [0..1]
    float p_bowForce        = 0.50f;  ///< bow pressure → F_b, α_bow          [0..1]
    float p_nonlinearity    = 0.25f;  ///< friction-curve curvature β         [0..1]
    float p_excitationRatio = 0.30f;  ///< normalised bow position            [0..1]
    float p_bowWidth        = 0.05f;  ///< contact width (unused in base bow) [0..1]
    float p_pickupBase      = 0.70f;  ///< normalised pickup position         [0..1]
    float p_octave          = 0.00f;  ///< octave transpose (semitones × 12)
    float p_detune          = 0.00f;  ///< fine detune in semitones
    float p_chaos           = 0.00f;  ///< stochastic perturbation amount     [0..1]

    // -----------------------------------------------------------------------
    // Physical state  (accessible for validation / visualisation)
    // -----------------------------------------------------------------------
    std::vector<float> x;       ///< displacement vector, current step   [n_total]
    std::vector<float> x_prev;  ///< displacement vector, previous step  [n_total]

    int n_total = N_DEFAULT_1D; ///< total number of nodes currently active

    // Current bow state (written by bow(), read by NR())
    float bowPos_curr    = 0.30f; ///< normalised bow position used this sample
    float bowVelocity    = 1.00f; ///< bow tip velocity  (set by voice logic / MPE)
    float bow_phi        = 0.00f; ///< φ(v_rel) — friction force this sample
    float bow_dphi       = 0.00f; ///< φ'(v_rel) — friction force derivative

    // -----------------------------------------------------------------------
    // Parameter ID enum (mirrors setParameterWithID index)
    // -----------------------------------------------------------------------
    enum NetworkParamID : int
    {
        kSpringDamping   = 0,
        kMassDamping     = 1,
        kBowForce        = 2,
        kNonlinearity    = 3,
        kExcitationRatio = 4,
        kBowWidth        = 5,
        kPickupBase      = 6,
        kOctave          = 7,
        kDetune          = 8,
    };

private:
    // -----------------------------------------------------------------------
    // Physical coefficients  (computed by refreshCoefficients)
    // -----------------------------------------------------------------------
    float sampleRate_     = 44100.0f;
    float dt_             = 1.0f / 44100.0f;

    float baseSpring_     = 1.0f;   ///< spring constant k  (pitch-scaled)
    float baseMass_       = 1.0f;   ///< uniform node mass m
    float alphaDamp_      = 0.0f;   ///< mass-proportional Rayleigh coefficient
    float betaDamp_       = 0.0f;   ///< stiffness-proportional Rayleigh coefficient

    float bowForceScale_  = 1.0f;   ///< F_b
    float alphaBow_       = 100.0f; ///< friction-curve slope α
    float betaBow_        = 0.10f;  ///< friction-curve nonlinearity β

    float noteFrequency_  = 440.0f; ///< fundamental frequency in Hz
    float baseSpringDamping_ = 0.20f;
    float baseMassDamping_ = 0.02f;
    float baseBowForce_ = 0.50f;
    float baseNonlinearity_ = 0.25f;
    float baseExcitationRatio_ = 0.30f;
    float basePickupBase_ = 0.70f;
    float baseChaos_ = 0.0f;
    float springModAmount_ = 0.0f;
    float chaosSmoothed_ = 0.0f;
    float chaosTarget_ = 0.0f;
    int chaosCountdown_ = 0;
    unsigned int chaosState_ = 0x12345678u;

    // -----------------------------------------------------------------------
    // System matrices  (assembled by calculateFullSystem*)
    // -----------------------------------------------------------------------
    std::vector<float> A_mat_;   ///< n_total × n_total, row-major
    std::vector<float> b_vec_;   ///< n_total RHS vector
    std::vector<float> velocityScratch_;
    std::vector<float> xStepStart_;
    std::vector<float> xNewScratch_;
    std::vector<float> weightScratch_;
    std::vector<float> jacobianScratch_;
    std::vector<float> residualScratch_;

    // -----------------------------------------------------------------------
    // Topology
    // -----------------------------------------------------------------------
    Profile profile_ = Profile::OneDimensional;
    int n2d_x_       = N_DEFAULT_2D; ///< 2-D grid columns
    int n2d_y_       = N_DEFAULT_2D; ///< 2-D grid rows

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    /** Resize all internal vectors to n nodes. */
    void resize_ (int n);

    /** Build the 1-D tridiagonal stiffness/damping contribution into A_mat_/b_vec_.
     *  Shared by calculateFullSystem1D() and calculateFullSystemEco(). */
    void buildSystem1DImpl_ (int n);

    /** In-place Gaussian elimination with partial pivoting.
     *  Solves A·x = b; on return b contains the solution.
     *  @param mat   n×n matrix (row-major, modified in-place)
     *  @param rhs   n-vector  (modified in-place → solution)
     *  @param n     system size
     *  @return      false if the matrix is (near-)singular */
    static bool gaussianSolve (std::vector<float>& mat,
                                std::vector<float>& rhs,
                                int n);

    /** Friction law:  φ(v) = F_b·(tanh(α·v) + β·tanh³(α·v)) */
    float bowPhi_  (float v_rel) const;

    /** Friction law derivative:  φ'(v) = F_b·α·(1 − tanh²(α·v))·(1 + 3β·tanh²(α·v)) */
    float bowDPhi_ (float v_rel) const;

    /** Fill a dense N-vector with Catmull–Rom weights for position pos.
     *  Only 4 consecutive entries are non-zero; all others are 0. */
    static void getCubicWeightVector (float pos, int n, std::vector<float>& w);
};
