#include "NetworkIf.h"

#include <cassert>
#include <cstring>
#include <stdexcept>

static constexpr float kPi = 3.14159265358979323846f;

//==============================================================================
// Construction
//==============================================================================

NetworkIf::NetworkIf()
{
    resize_ (N_DEFAULT_1D);
    refreshCoefficients();
}

//==============================================================================
// Lifecycle
//==============================================================================

void NetworkIf::setupNote (float midiNote, float velocity, float tuning, bool /*isMono*/)
{
    // Convert MIDI note + tuning offset to Hz
    noteFrequency_ = 440.0f * std::pow (2.0f, (midiNote - 69.0f + tuning) / 12.0f);

    // Scale bow velocity from MIDI velocity (linear mapping 0→0, 1→3 m/s equivalent)
    bowVelocity = velocity * 3.0f;

    refreshCoefficients();
    resetStates (true, false);
}

void NetworkIf::stopNote()
{
    // Voice keeps decaying; just zero the bow so excitation stops.
    bowVelocity = 0.0f;
}

void NetworkIf::resetStates (bool hard, bool clearBow)
{
    std::fill (x.begin(),      x.end(),      0.0f);
    std::fill (x_prev.begin(), x_prev.end(), 0.0f);

    if (clearBow || hard)
    {
        bow_phi   = 0.0f;
        bow_dphi  = 0.0f;
    }
    if (hard)
        bowVelocity = 0.0f;
}

void NetworkIf::setSampleRate (float sr)
{
    sampleRate_ = sr;
    dt_         = 1.0f / sr;
}

//==============================================================================
// Configuration
//==============================================================================

void NetworkIf::changeDimensions (int n)
{
    n = std::max (1, std::min (n, N_MAX_TOTAL));
    resize_ (n);
}

void NetworkIf::changeProfile (Profile profile)
{
    profile_ = profile;

    switch (profile)
    {
        case Profile::OneDimensional:
            resize_ (N_DEFAULT_1D);
            break;

        case Profile::TwoDimensional:
            n2d_x_ = N_DEFAULT_2D;
            n2d_y_ = N_DEFAULT_2D;
            resize_ (n2d_x_ * n2d_y_);
            break;

        case Profile::Eco:
            resize_ (N_ECO);
            break;
    }
}

void NetworkIf::setParameterWithID (int id, float value, bool /*notify*/)
{
    switch (id)
    {
        case kSpringDamping:    p_springDamping   = value; break;
        case kMassDamping:      p_massDamping     = value; break;
        case kBowForce:         p_bowForce        = value; break;
        case kNonlinearity:     p_nonlinearity    = value; break;
        case kExcitationRatio:  p_excitationRatio = value; break;
        case kBowWidth:         p_bowWidth        = value; break;
        case kPickupBase:       p_pickupBase      = value; break;
        case kOctave:           p_octave          = value; break;
        case kDetune:           p_detune          = value; break;
        default: break;
    }
}

//==============================================================================
// refreshCoefficients  — map UI params to physical coefficients
//==============================================================================

void NetworkIf::refreshCoefficients()
{
    // -----------------------------------------------------------------------
    // Pitch:  ω₀ ∝ √(k/m)
    //
    // For a discrete 1-D chain of N equal masses/springs with fixed ends,
    // the fundamental angular frequency is approximately:
    //   ω₁ ≈ 2·√(k/m)·sin(π/(2N))  ≈  √(k/m)·π/N   (for large N)
    //
    // Setting ω₁ = 2π·f_eff gives:
    //   k = m·(2π·f_eff·N/π)²  =  m·4·f_eff²·N²
    //
    // For 2-D the effective mode spacing is similar (use n_total as proxy).
    // -----------------------------------------------------------------------
    float semitones   = p_octave * 12.0f + p_detune;
    float pitchScale  = std::pow (2.0f, semitones / 12.0f);
    float f_eff       = noteFrequency_ * pitchScale;
    float omega_eff   = 2.0f * kPi * f_eff;
    float N_f         = float (n_total);

    baseMass_   = 1.0f;
    baseSpring_ = baseMass_ * (omega_eff * N_f / kPi) * (omega_eff * N_f / kPi);

    // Guard against degenerate configurations
    baseSpring_ = std::max (baseSpring_, 1.0f);

    // -----------------------------------------------------------------------
    // Rayleigh damping:  C = α·M + β·K
    //   α  (massDampCoeff)    → flat HF damping (mass-proportional)
    //   β  (springDampCoeff)  → frequency-dependent LF damping
    // -----------------------------------------------------------------------
    alphaDamp_ = p_massDamping  * 200.0f;   // α  ∈ [0, 200]
    betaDamp_  = p_springDamping * 0.005f;   // β  ∈ [0, 0.005]

    // -----------------------------------------------------------------------
    // Bow friction law:  φ(v) = F_b·(tanh(α·v) + β·tanh³(α·v))
    //   F_b   ← p_bowForce (scaled)
    //   α_bow ← p_bowForce (friction slope steepens with pressure)
    //   β_bow ← p_nonlinearity
    // -----------------------------------------------------------------------
    bowForceScale_ = p_bowForce * 5.0f;
    alphaBow_      = 50.0f + p_bowForce * 450.0f;   // 50 … 500
    betaBow_       = p_nonlinearity;
}

//==============================================================================
// System assembly helpers
//==============================================================================

/**
 * Common 1-D tridiagonal assembly shared by calculateFullSystem1D and Eco.
 *
 * Stiffness topology: fixed-end chain with ground springs at both ends.
 *   K[i,i]   =  2·k  for all i   (one spring left, one right, or to wall)
 *   K[i,i±1] = −k
 *
 * Rayleigh damping:  C = α·M + β·K   (M diagonal, K tridiagonal)
 *   C[i,i]   = α·m + β·K[i,i]
 *   C[i,i±1] = β·K[i,i±1]
 *
 * System matrix:   A = M/dt² + C/dt + K
 * RHS vector:      b = M·(2x−x_prev)/dt²  +  C·x/dt
 */
void NetworkIf::buildSystem1DImpl_ (int n)
{
    const float k    = baseSpring_;
    const float m    = baseMass_;
    const float idt  = 1.0f / dt_;
    const float idt2 = idt * idt;
    const float a    = alphaDamp_;
    const float b    = betaDamp_;

    // Zero matrices
    std::fill (A_mat_.begin(), A_mat_.end(), 0.0f);
    std::fill (b_vec_.begin(), b_vec_.end(), 0.0f);

    for (int i = 0; i < n; ++i)
    {
        // --- Stiffness row ---
        float K_diag  =  2.0f * k;
        float K_offL  = (i > 0)     ? -k : 0.0f;
        float K_offR  = (i < n - 1) ? -k : 0.0f;

        // --- Damping row (Rayleigh C = α·M + β·K) ---
        float C_diag = a * m + b * K_diag;
        float C_offL = b * K_offL;
        float C_offR = b * K_offR;

        // --- A = M/dt² + C/dt + K ---
        A_mat_[i * n + i] = m * idt2 + C_diag * idt + K_diag;

        if (i > 0)
            A_mat_[i * n + (i - 1)] = C_offL * idt + K_offL;

        if (i < n - 1)
            A_mat_[i * n + (i + 1)] = C_offR * idt + K_offR;

        // --- b = M·(2x−x_prev)/dt²  +  C·x/dt ---
        // Mass term
        float b_i = m * (2.0f * x[i] - x_prev[i]) * idt2;

        // Damping term: (C·x)[i] = C_diag·x[i] + C_offL·x[i-1] + C_offR·x[i+1]
        float Cx_i = C_diag * x[i];
        if (i > 0)     Cx_i += C_offL * x[i - 1];
        if (i < n - 1) Cx_i += C_offR * x[i + 1];

        b_vec_[i] = b_i + Cx_i * idt;
    }
}

//==============================================================================
// calculateFullSystem1D
//==============================================================================

void NetworkIf::calculateFullSystem1D()
{
    buildSystem1DImpl_ (n_total);
}

//==============================================================================
// calculateFullSystem2D
//==============================================================================

/**
 * 2-D rectangular lattice of Nx × Ny nodes (flattened index k = i*Ny + j).
 *
 * Spring connections: each interior node connects to 4 neighbours.
 * Missing neighbours are replaced by ground springs so that
 *   K[k,k] = (number_of_springs_connected_to_k) · k_spring
 * is always equal to 4·k for interior, 3·k for edge, 2·k for corner
 * when using a fixed-wall model — here simplified to always 4·k
 * (i.e. boundary nodes also have a "virtual" ground spring for stability).
 */
void NetworkIf::calculateFullSystem2D()
{
    const int Nx = n2d_x_;
    const int Ny = n2d_y_;
    const int n  = n_total;   // == Nx * Ny

    const float k    = baseSpring_;
    const float m    = baseMass_;
    const float idt  = 1.0f / dt_;
    const float idt2 = idt * idt;
    const float a    = alphaDamp_;
    const float bg   = betaDamp_;

    std::fill (A_mat_.begin(), A_mat_.end(), 0.0f);
    std::fill (b_vec_.begin(), b_vec_.end(), 0.0f);

    // Neighbour offsets in 2-D: left, right, up, down  (flattened)
    const int dx[4] = {-1,  1,  0,  0};
    const int dy[4] = { 0,  0, -1,  1};

    for (int i = 0; i < Nx; ++i)
    {
        for (int j = 0; j < Ny; ++j)
        {
            const int ki = i * Ny + j;

            // Count actual in-grid neighbours
            int nNeighbours = 0;
            for (int d = 0; d < 4; ++d)
            {
                int ni = i + dx[d];
                int nj = j + dy[d];
                if (ni >= 0 && ni < Nx && nj >= 0 && nj < Ny)
                    ++nNeighbours;
            }

            // Include virtual ground springs on boundary so K_diag = 4k always
            int nGround = 4 - nNeighbours;

            float K_diag = float (nNeighbours + nGround) * k;  // = 4k
            float C_diag = a * m + bg * K_diag;

            // A diagonal
            A_mat_[ki * n + ki] = m * idt2 + C_diag * idt + K_diag;

            // Off-diagonal neighbours
            for (int d = 0; d < 4; ++d)
            {
                int ni = i + dx[d];
                int nj = j + dy[d];
                if (ni < 0 || ni >= Nx || nj < 0 || nj >= Ny) continue;

                int kn = ni * Ny + nj;
                float K_off = -k;
                float C_off = bg * K_off;

                A_mat_[ki * n + kn] = C_off * idt + K_off;
            }

            // b: mass + damping terms
            float b_i = m * (2.0f * x[ki] - x_prev[ki]) * idt2;

            // (C·x)[ki] = C_diag·x[ki] + sum_neighbours C_off·x[kn]
            float Cx_i = C_diag * x[ki];
            for (int d = 0; d < 4; ++d)
            {
                int ni = i + dx[d];
                int nj = j + dy[d];
                if (ni < 0 || ni >= Nx || nj < 0 || nj >= Ny) continue;

                int kn = ni * Ny + nj;
                Cx_i += bg * (-k) * x[kn];
            }

            b_vec_[ki] = b_i + Cx_i * idt;
        }
    }
}

//==============================================================================
// calculateFullSystemEco
//==============================================================================

/** Reduced 4-node 1-D chain.  Same math as 1-D but n is always N_ECO. */
void NetworkIf::calculateFullSystemEco()
{
    // Eco always operates on 4 nodes.  Resize if needed (should be pre-set).
    if (n_total != N_ECO)
        resize_ (N_ECO);

    buildSystem1DImpl_ (N_ECO);
}

//==============================================================================
// bow  — friction force injection
//==============================================================================

/**
 * Computes:
 *   v_struct = B·(x − x_prev)/dt   at bowPos_curr  (cubic read)
 *   v_rel    = v_bow − v_struct
 *   φ        = bowPhi_(v_rel)
 *   φ'       = bowDPhi_(v_rel)
 *
 * Results are stored in bow_phi / bow_dphi for use in NR().
 * bowPos_curr is updated from p_excitationRatio (LFO/MPE modulation would
 * be applied here by the voice before calling bow()).
 */
void NetworkIf::bow()
{
    bowPos_curr = p_excitationRatio;   // voice can override before calling

    // Continuous position in [0, n_total − 1]
    float pos = bowPos_curr * float (n_total - 1);
    pos = std::max (0.0f, std::min (pos, float (n_total - 1)));

    // Read structural velocity at bow position: v_struct = B·v^n
    //   v^n = (x^n − x^{n-1}) / dt  (approximation using previous step)
    std::vector<float> v (n_total);
    for (int i = 0; i < n_total; ++i)
        v[i] = (x[i] - x_prev[i]) / dt_;

    float v_struct = interpolation3 (v.data(), n_total, pos);
    float v_rel    = bowVelocity - v_struct;

    bow_phi  = bowPhi_  (v_rel);
    bow_dphi = bowDPhi_ (v_rel);
}

//==============================================================================
// NR — 3-iteration Newton–Raphson solve
//==============================================================================

/**
 * Solves   R(x_new) = A·x_new − b − G^T·φ(v_bow − B·(x_new−x^n)/dt) = 0
 *
 * with Jacobian:
 *   J = A + (φ'(u)/dt)·w·wᵀ        (rank-1 update)
 *
 * Initial guess:  x_new = 2·x − x_prev  (linear predictor)
 *
 * After solve: x_prev ← x,  x ← x_new
 */
void NetworkIf::NR()
{
    const int n    = n_total;
    const float idt = 1.0f / dt_;

    // Continuous bow position
    float bowP = bowPos_curr * float (n - 1);
    bowP = std::max (0.0f, std::min (bowP, float (n - 1)));

    // x_prev_snapshot: x at start of this step (for velocity calculation in NR)
    std::vector<float> x_n = x;

    // Initial guess: linear predictor
    std::vector<float> x_new (n);
    for (int i = 0; i < n; ++i)
        x_new[i] = 2.0f * x[i] - x_prev[i];

    // Precompute cubic weight vector w (N-dim, sparse: 4 non-zeros)
    std::vector<float> w (n, 0.0f);
    getCubicWeightVector (bowP, n, w);

    // Working copies for Jacobian solve
    std::vector<float> J (n * n);
    std::vector<float> R (n);

    for (int iter = 0; iter < 3; ++iter)
    {
        // ----- Evaluate friction at current x_new -----
        // v_struct = B·(x_new − x_n)/dt  =  w^T · (x_new − x_n)/dt
        float v_struct = 0.0f;
        for (int i = 0; i < n; ++i)
            v_struct += w[i] * (x_new[i] - x_n[i]);
        v_struct *= idt;

        float v_rel  = bowVelocity - v_struct;
        float phi    = bowPhi_  (v_rel);
        float dphi   = bowDPhi_ (v_rel);

        // ----- Residual:  R = A·x_new − b − G^T·φ  -----
        for (int i = 0; i < n; ++i)
        {
            float Ax_i = 0.0f;
            for (int j = 0; j < n; ++j)
                Ax_i += A_mat_[i * n + j] * x_new[j];

            // G^T·φ at row i = w[i]·φ
            R[i] = Ax_i - b_vec_[i] - w[i] * phi;
        }

        // ----- Jacobian:  J = A + (φ'/dt)·w·wᵀ  -----
        float scale = dphi * idt;
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                J[i * n + j] = A_mat_[i * n + j] + scale * w[i] * w[j];

        // ----- Solve J·δx = −R  -----
        // Negate R in-place (becomes RHS −R)
        for (int i = 0; i < n; ++i) R[i] = -R[i];

        if (!gaussianSolve (J, R, n))
            break;   // singular (shouldn't happen in practice)

        // R now contains δx
        for (int i = 0; i < n; ++i)
            x_new[i] += R[i];
    }

    // Advance time step
    x_prev = x_n;
    x      = x_new;
}

//==============================================================================
// getROutput
//==============================================================================

float NetworkIf::getROutput (float pos)
{
    float p = pos * float (n_total - 1);
    p = std::max (0.0f, std::min (p, float (n_total - 1)));
    return interpolation3 (x.data(), n_total, p);
}

//==============================================================================
// Interpolation operators
//==============================================================================

/**
 * Catmull–Rom cubic interpolation.
 *
 * Four sample points:  p[−1], p[0], p[1], p[2]  where 0 is floor(pos).
 * Weights:
 *   w[-1] = ½·(−t  + 2t² − t³)
 *   w[ 0] = ½·( 2  − 5t² + 3t³)
 *   w[ 1] = ½·( t  + 4t² − 3t³)
 *   w[ 2] = ½·(      −t² + t³)
 */
float NetworkIf::interpolation3 (const float* arr, int n, float pos)
{
    int   i = (int)std::floor (pos);
    float t = pos - float (i);

    // Clamp sample indices
    auto samp = [&](int idx) -> float {
        return arr[std::max (0, std::min (n - 1, idx))];
    };

    float p0 = samp (i - 1);
    float p1 = samp (i);
    float p2 = samp (i + 1);
    float p3 = samp (i + 2);

    float t2 = t * t;
    float t3 = t2 * t;

    float w0 = 0.5f * (-t3 + 2.0f * t2 - t);
    float w1 = 0.5f * (3.0f * t3 - 5.0f * t2 + 2.0f);
    float w2 = 0.5f * (-3.0f * t3 + 4.0f * t2 + t);
    float w3 = 0.5f * (t3 - t2);

    return w0 * p0 + w1 * p1 + w2 * p2 + w3 * p3;
}

/**
 * Catmull–Rom scatter / write operator.
 * Distributes  value  onto the 4 adjacent grid points using the same kernel.
 * Equivalent to  arr += w · value  for the 4 non-zero weights.
 */
void NetworkIf::interpolation3Mult (float* arr, int n, float pos, float value)
{
    int   i = (int)std::floor (pos);
    float t = pos - float (i);

    float t2 = t * t;
    float t3 = t2 * t;

    float w0 = 0.5f * (-t3 + 2.0f * t2 - t);
    float w1 = 0.5f * (3.0f * t3 - 5.0f * t2 + 2.0f);
    float w2 = 0.5f * (-3.0f * t3 + 4.0f * t2 + t);
    float w3 = 0.5f * (t3 - t2);

    auto addSamp = [&](int idx, float w) {
        int ci = std::max (0, std::min (n - 1, idx));
        arr[ci] += w * value;
    };

    addSamp (i - 1, w0);
    addSamp (i,     w1);
    addSamp (i + 1, w2);
    addSamp (i + 2, w3);
}

//==============================================================================
// Private helpers
//==============================================================================

void NetworkIf::resize_ (int n)
{
    n_total = n;

    x.assign     (n, 0.0f);
    x_prev.assign(n, 0.0f);

    A_mat_.assign (n * n, 0.0f);
    b_vec_.assign (n,     0.0f);
}

/** Gaussian elimination with partial pivoting.
 *  On return, rhs[i] contains the solution x[i] such that mat·x = rhs_original.
 *  mat and rhs are both overwritten. */
bool NetworkIf::gaussianSolve (std::vector<float>& mat,
                                std::vector<float>& rhs,
                                int n)
{
    for (int col = 0; col < n; ++col)
    {
        // Find pivot
        int   pivotRow = col;
        float pivotVal = std::abs (mat[col * n + col]);

        for (int row = col + 1; row < n; ++row)
        {
            float v = std::abs (mat[row * n + col]);
            if (v > pivotVal) { pivotVal = v; pivotRow = row; }
        }

        if (pivotVal < 1.0e-14f)
            return false;   // singular

        // Swap rows
        if (pivotRow != col)
        {
            for (int j = 0; j < n; ++j)
                std::swap (mat[col * n + j], mat[pivotRow * n + j]);
            std::swap (rhs[col], rhs[pivotRow]);
        }

        // Eliminate below
        float invDiag = 1.0f / mat[col * n + col];
        for (int row = col + 1; row < n; ++row)
        {
            float factor = mat[row * n + col] * invDiag;
            mat[row * n + col] = 0.0f;
            for (int j = col + 1; j < n; ++j)
                mat[row * n + j] -= factor * mat[col * n + j];
            rhs[row] -= factor * rhs[col];
        }
    }

    // Back-substitution
    for (int i = n - 1; i >= 0; --i)
    {
        float sum = rhs[i];
        for (int j = i + 1; j < n; ++j)
            sum -= mat[i * n + j] * rhs[j];
        rhs[i] = sum / mat[i * n + i];
    }

    return true;
}

void NetworkIf::getCubicWeightVector (float pos, int n, std::vector<float>& w)
{
    std::fill (w.begin(), w.end(), 0.0f);

    int   i = (int)std::floor (pos);
    float t = pos - float (i);
    float t2 = t * t;
    float t3 = t2 * t;

    float weights[4] = {
        0.5f * (-t3 + 2.0f * t2 - t),
        0.5f * (3.0f * t3 - 5.0f * t2 + 2.0f),
        0.5f * (-3.0f * t3 + 4.0f * t2 + t),
        0.5f * (t3 - t2)
    };

    int offsets[4] = { i - 1, i, i + 1, i + 2 };

    for (int k = 0; k < 4; ++k)
    {
        int ci = std::max (0, std::min (n - 1, offsets[k]));
        w[ci] += weights[k];   // += handles clamped duplicate indices
    }
}

// φ(v) = F_b · (tanh(α·v) + β·tanh³(α·v))
float NetworkIf::bowPhi_ (float v_rel) const
{
    float av  = alphaBow_ * v_rel;
    float th  = std::tanh (av);
    float th3 = th * th * th;
    return bowForceScale_ * (th + betaBow_ * th3);
}

// φ'(v) = F_b · α · (1 − tanh²(α·v)) · (1 + 3β·tanh²(α·v))
float NetworkIf::bowDPhi_ (float v_rel) const
{
    float av   = alphaBow_ * v_rel;
    float th   = std::tanh (av);
    float th2  = th * th;
    float sech2 = 1.0f - th2;                         // sech²(α·v)
    return bowForceScale_ * alphaBow_ * sech2 * (1.0f + 3.0f * betaBow_ * th2);
}
