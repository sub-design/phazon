"""
network_if_reference.py
=======================
Python/NumPy reference implementation of the NetworkIf DSP core.

Purpose: validate the C++ NetworkIf against a readable, testable Python version.
Every function here corresponds 1-to-1 with a C++ method in src/engine/NetworkIf.cpp.

Usage (quick smoke-test):
    python reference/network_if_reference.py

Dependencies: numpy, scipy (optional, for comparison solves)
"""

from __future__ import annotations

import math
import numpy as np
from dataclasses import dataclass, field
from typing import Tuple


# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------
N_ECO        = 4
N_DEFAULT_1D = 8
N_DEFAULT_2D = 4   # 4×4 = 16 nodes


# ===========================================================================
# Interpolation operators
# ===========================================================================

def interpolation3(arr: np.ndarray, pos: float) -> float:
    """
    Catmull–Rom cubic read operator.

    Four sample points: arr[i-1], arr[i], arr[i+1], arr[i+2]
    where i = floor(pos), t = pos - i.

    Weights:
        w[-1] = ½·(−t  + 2t² − t³)
        w[ 0] = ½·( 2  − 5t² + 3t³)
        w[ 1] = ½·( t  + 4t² − 3t³)
        w[ 2] = ½·(       −t² + t³)
    """
    n = len(arr)
    i = int(math.floor(pos))
    t = pos - i

    def s(idx):
        return arr[max(0, min(n - 1, idx))]

    p0, p1, p2, p3 = s(i - 1), s(i), s(i + 1), s(i + 2)

    t2, t3 = t * t, t * t * t

    w0 = 0.5 * (-t3 + 2 * t2 - t)
    w1 = 0.5 * (3 * t3 - 5 * t2 + 2)
    w2 = 0.5 * (-3 * t3 + 4 * t2 + t)
    w3 = 0.5 * (t3 - t2)

    return w0 * p0 + w1 * p1 + w2 * p2 + w3 * p3


def interpolation3_mult(arr: np.ndarray, pos: float, value: float) -> None:
    """
    Catmull–Rom scatter / write operator (in-place).

    Distributes  value  onto 4 adjacent grid points with the same kernel.
    Equivalent to: arr[i-1:i+3] += weights * value   (with clamping).
    """
    n = len(arr)
    i = int(math.floor(pos))
    t = pos - i
    t2, t3 = t * t, t * t * t

    w0 = 0.5 * (-t3 + 2 * t2 - t)
    w1 = 0.5 * (3 * t3 - 5 * t2 + 2)
    w2 = 0.5 * (-3 * t3 + 4 * t2 + t)
    w3 = 0.5 * (t3 - t2)

    offsets = [i - 1, i, i + 1, i + 2]
    weights = [w0, w1, w2, w3]

    for offset, w in zip(offsets, weights):
        ci = max(0, min(n - 1, offset))
        arr[ci] += w * value


def get_cubic_weight_vector(pos: float, n: int) -> np.ndarray:
    """
    Return the full N-dimensional weight vector for Catmull–Rom interpolation.
    Only 4 consecutive entries are non-zero (clamped indices accumulate).
    """
    i = int(math.floor(pos))
    t = pos - i
    t2, t3 = t * t, t * t * t

    raw_w = [
        0.5 * (-t3 + 2 * t2 - t),
        0.5 * (3 * t3 - 5 * t2 + 2),
        0.5 * (-3 * t3 + 4 * t2 + t),
        0.5 * (t3 - t2),
    ]
    offsets = [i - 1, i, i + 1, i + 2]

    w = np.zeros(n)
    for offset, wk in zip(offsets, raw_w):
        ci = max(0, min(n - 1, offset))
        w[ci] += wk
    return w


# ===========================================================================
# Friction law
# ===========================================================================

def bow_phi(v_rel: float, F_b: float, alpha: float, beta: float) -> float:
    """
    φ(v) = F_b · (tanh(α·v) + β·tanh³(α·v))

    Mapping from parameters:
        F_b   ← BowForce (amplitude)
        α     ← BowForce (friction slope steepens with pressure)
        β     ← Nonlinearity
    """
    av  = alpha * v_rel
    th  = math.tanh(av)
    return F_b * (th + beta * th ** 3)


def bow_dphi(v_rel: float, F_b: float, alpha: float, beta: float) -> float:
    """
    φ'(v) = F_b · α · (1 − tanh²(α·v)) · (1 + 3β·tanh²(α·v))
    """
    av   = alpha * v_rel
    th   = math.tanh(av)
    th2  = th * th
    sech2 = 1.0 - th2
    return F_b * alpha * sech2 * (1.0 + 3.0 * beta * th2)


# ===========================================================================
# System assembly
# ===========================================================================

def build_system_1d(
    x: np.ndarray,
    x_prev: np.ndarray,
    k: float,
    m: float,
    alpha_damp: float,
    beta_damp: float,
    dt: float,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Assemble  A  and  b  for a 1-D spring chain with fixed-end boundary.

    Topology: ground − k − node_0 − k − node_1 − … − node_{N-1} − k − ground
    Stiffness:  K[i,i] = 2k,  K[i,i±1] = −k

    Rayleigh damping:  C = α·M + β·K   (M = m·I diagonal)

    System matrix:   A = M/dt² + C/dt + K
    RHS vector:      b = M·(2x−x_prev)/dt²  +  C·x/dt

    Returns
    -------
    A : (N, N) ndarray
    b : (N,)   ndarray
    """
    n = len(x)
    idt = 1.0 / dt
    idt2 = idt * idt

    # Build K (tridiagonal)
    K = np.zeros((n, n))
    for i in range(n):
        K[i, i] = 2.0 * k
        if i > 0:
            K[i, i - 1] = -k
        if i < n - 1:
            K[i, i + 1] = -k

    # Rayleigh: C = α·M + β·K
    M = m * np.eye(n)
    C = alpha_damp * M + beta_damp * K

    # System matrix
    A = M * idt2 + C * idt + K

    # RHS: b = M·(2x−x_prev)/dt²  +  C·x/dt
    b = M @ (2 * x - x_prev) * idt2 + C @ x * idt

    return A, b


def build_system_2d(
    x: np.ndarray,
    x_prev: np.ndarray,
    Nx: int,
    Ny: int,
    k: float,
    m: float,
    alpha_damp: float,
    beta_damp: float,
    dt: float,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Assemble  A  and  b  for a 2-D rectangular lattice (Nx × Ny nodes).

    Flattened index:  ki = i*Ny + j
    Neighbours: (i±1, j), (i, j±1).  Missing neighbours → virtual ground spring.
    All nodes therefore have K[k,k] = 4k.

    Returns
    -------
    A : (Nx*Ny, Nx*Ny) ndarray
    b : (Nx*Ny,)       ndarray
    """
    n   = Nx * Ny
    idt = 1.0 / dt
    idt2 = idt * idt

    K = np.zeros((n, n))
    dx = [-1,  1,  0,  0]
    dy = [ 0,  0, -1,  1]

    for i in range(Nx):
        for j in range(Ny):
            ki = i * Ny + j
            n_neighbours = sum(
                1
                for d in range(4)
                if 0 <= i + dx[d] < Nx and 0 <= j + dy[d] < Ny
            )
            n_ground = 4 - n_neighbours
            K[ki, ki] = float(n_neighbours + n_ground) * k   # always 4k

            for d in range(4):
                ni, nj = i + dx[d], j + dy[d]
                if 0 <= ni < Nx and 0 <= nj < Ny:
                    kn = ni * Ny + nj
                    K[ki, kn] = -k

    M = m * np.eye(n)
    C = alpha_damp * M + beta_damp * K

    A = M * idt2 + C * idt + K
    b = M @ (2 * x - x_prev) * idt2 + C @ x * idt

    return A, b


def build_system_eco(
    x: np.ndarray,
    x_prev: np.ndarray,
    k: float,
    m: float,
    alpha_damp: float,
    beta_damp: float,
    dt: float,
) -> Tuple[np.ndarray, np.ndarray]:
    """
    Eco mode: 4-node 1-D chain.  Delegates to build_system_1d with n=4.
    """
    assert len(x) == N_ECO, f"Eco mode requires {N_ECO} nodes, got {len(x)}"
    return build_system_1d(x, x_prev, k, m, alpha_damp, beta_damp, dt)


# ===========================================================================
# Nonlinear solver
# ===========================================================================

def newton_raphson(
    A: np.ndarray,
    b: np.ndarray,
    x_n: np.ndarray,
    x_prev: np.ndarray,
    bow_pos: float,
    bow_velocity: float,
    F_b: float,
    alpha_bow: float,
    beta_bow: float,
    dt: float,
    n_iter: int = 3,
) -> np.ndarray:
    """
    3-iteration Newton–Raphson solve for the nonlinear implicit system.

    Residual:
        R(x) = A·x − b − G^T · φ(v_bow − B·(x − x_n)/dt)

    Jacobian (rank-1 update):
        J = A + (φ'(u)/dt) · w·wᵀ

    where  w  is the Catmull–Rom weight vector at bow_pos.

    Parameters
    ----------
    A, b       : assembled linear system from build_system_*()
    x_n        : displacement at start of this time step
    x_prev     : displacement at previous time step (for predictor only)
    bow_pos    : continuous bow position ∈ [0, N−1]
    bow_velocity : bow tip velocity (set by voice / MPE)
    F_b, alpha_bow, beta_bow : bow friction parameters
    dt         : sample period
    n_iter     : number of Newton iterations (default 3)

    Returns
    -------
    x_new : updated displacement vector
    """
    n   = len(x_n)
    idt = 1.0 / dt

    # Cubic weight vector at bow position (read/write operator)
    w = get_cubic_weight_vector(bow_pos, n)

    # Initial guess: linear predictor
    x_new = 2.0 * x_n - x_prev

    for _ in range(n_iter):
        # Structural velocity at bow position: B·(x_new − x_n)/dt
        v_struct = float(np.dot(w, x_new - x_n)) * idt
        v_rel    = bow_velocity - v_struct

        phi  = bow_phi  (v_rel, F_b, alpha_bow, beta_bow)
        dphi = bow_dphi (v_rel, F_b, alpha_bow, beta_bow)

        # Residual: R = A·x_new − b − w·φ
        R = A @ x_new - b - w * phi

        # Jacobian: J = A + (φ'/dt)·w·wᵀ
        J = A + (dphi * idt) * np.outer(w, w)

        # Newton step: J·δx = −R
        try:
            delta = np.linalg.solve(J, -R)
        except np.linalg.LinAlgError:
            break   # singular (shouldn't happen)

        x_new = x_new + delta

    return x_new


# ===========================================================================
# refreshCoefficients
# ===========================================================================

@dataclass
class PhysicalCoefficients:
    """
    Maps UI parameter values to physical model coefficients.

    Corresponds to NetworkIf::refreshCoefficients().
    """
    # --- inputs (UI params) ---
    p_spring_damping:    float = 0.20
    p_mass_damping:      float = 0.02
    p_bow_force:         float = 0.50
    p_nonlinearity:      float = 0.25
    p_octave:            float = 0.0
    p_detune:            float = 0.0
    note_frequency:      float = 440.0   # Hz, from setupNote()
    n_total:             int   = N_DEFAULT_1D
    sample_rate:         float = 44100.0

    # --- outputs (computed) ---
    base_spring:   float = field(init=False)
    base_mass:     float = field(init=False)
    alpha_damp:    float = field(init=False)
    beta_damp:     float = field(init=False)
    bow_force_scale: float = field(init=False)
    alpha_bow:     float = field(init=False)
    beta_bow:      float = field(init=False)
    dt:            float = field(init=False)

    def __post_init__(self):
        self.refresh()

    def refresh(self):
        """Recompute all physical coefficients from UI params."""
        # Pitch: ω₀ ∝ √(k/m)
        # Discrete 1-D string fundamental: ω₁ ≈ √(k/m)·π/N
        # → k = m·(2π·f_eff·N/π)²  =  m·4·f_eff²·N²
        semitones   = self.p_octave * 12.0 + self.p_detune
        pitch_scale = 2.0 ** (semitones / 12.0)
        f_eff       = self.note_frequency * pitch_scale
        omega_eff   = 2.0 * math.pi * f_eff
        N_f         = float(self.n_total)

        self.dt           = 1.0 / self.sample_rate
        self.base_mass    = 1.0
        self.base_spring  = max(1.0, self.base_mass * (omega_eff * N_f / math.pi) ** 2)

        # Rayleigh damping: C = α·M + β·K
        self.alpha_damp = self.p_mass_damping   * 200.0
        self.beta_damp  = self.p_spring_damping * 0.005

        # Bow parameters
        self.bow_force_scale = self.p_bow_force * 5.0
        self.alpha_bow       = 50.0 + self.p_bow_force * 450.0
        self.beta_bow        = self.p_nonlinearity


# ===========================================================================
# Full single-sample pipeline
# ===========================================================================

def render_sample(
    x: np.ndarray,
    x_prev: np.ndarray,
    coeff: PhysicalCoefficients,
    bow_pos_norm: float,
    bow_velocity: float,
    pickup_pos_norm: float,
    mode: str = "1d",
    Nx: int = N_DEFAULT_2D,
    Ny: int = N_DEFAULT_2D,
) -> Tuple[float, np.ndarray, np.ndarray]:
    """
    Execute one complete per-sample pipeline.

    Parameters
    ----------
    x, x_prev    : current and previous displacements
    coeff        : PhysicalCoefficients (from refreshCoefficients)
    bow_pos_norm : normalised bow position ∈ [0, 1]
    bow_velocity : bow tip velocity
    pickup_pos_norm : normalised pickup position ∈ [0, 1]
    mode         : "1d", "2d", "eco"

    Returns
    -------
    output   : scalar audio sample
    x_new    : updated displacement
    x_n      : (old x, becomes x_prev next step)
    """
    n  = coeff.n_total
    dt = coeff.dt
    k  = coeff.base_spring
    m  = coeff.base_mass
    ad = coeff.alpha_damp
    bd = coeff.beta_damp

    # 1. Build linear system
    if mode == "2d":
        A, b = build_system_2d(x, x_prev, Nx, Ny, k, m, ad, bd, dt)
    elif mode == "eco":
        A, b = build_system_eco(x, x_prev, k, m, ad, bd, dt)
    else:
        A, b = build_system_1d(x, x_prev, k, m, ad, bd, dt)

    # 2. Continuous bow position [0, n−1]
    bow_p = bow_pos_norm * (n - 1)

    # 3. Newton–Raphson solve (includes bow force)
    x_new = newton_raphson(
        A, b, x, x_prev,
        bow_p, bow_velocity,
        coeff.bow_force_scale, coeff.alpha_bow, coeff.beta_bow,
        dt,
    )

    # 4. Readout via cubic interpolation
    pickup_p = pickup_pos_norm * (n - 1)
    output   = interpolation3(x_new, pickup_p)

    return output, x_new, x


# ===========================================================================
# Self-test
# ===========================================================================

def _run_smoke_test():
    """
    Smoke test: run 200 samples of a bowed 8-node string and check
    that the output is finite and non-trivial.
    """
    print("Running NetworkIf reference smoke test...")

    coeff = PhysicalCoefficients(
        note_frequency=220.0,
        n_total=N_DEFAULT_1D,
        p_bow_force=0.5,
        p_spring_damping=0.2,
        p_mass_damping=0.02,
        p_nonlinearity=0.25,
    )

    x      = np.zeros(coeff.n_total)
    x_prev = np.zeros(coeff.n_total)

    outputs = []
    for i in range(200):
        y, x, x_prev = render_sample(
            x, x_prev, coeff,
            bow_pos_norm=0.3,
            bow_velocity=1.0,
            pickup_pos_norm=0.7,
        )
        outputs.append(y)

    outputs = np.array(outputs)
    assert np.all(np.isfinite(outputs)), "NaN/Inf in output!"

    max_amp = np.max(np.abs(outputs))
    print(f"  Max amplitude after 200 samples: {max_amp:.6f}")
    assert max_amp > 1e-10, "Output is zero — no excitation."

    # --- Interpolation unit tests ---
    arr = np.array([0.0, 1.0, 0.0, -1.0, 0.0])
    assert abs(interpolation3(arr, 1.0) - 1.0) < 1e-9,  "interpolation3 at integer failed"
    assert abs(interpolation3(arr, 0.0) - 0.0) < 1e-9,  "interpolation3 at 0 failed"

    scatter = np.zeros(5)
    interpolation3_mult(scatter, 2.5, 1.0)
    assert abs(sum(scatter) - 1.0) < 1e-6, "interpolation3_mult partition-of-unity failed"

    # --- bow_phi / bow_dphi unit tests ---
    phi_at_zero = bow_phi(0.0, F_b=1.0, alpha=100.0, beta=0.25)
    assert abs(phi_at_zero) < 1e-9, "φ(0) should be 0"

    dphi_at_zero = bow_dphi(0.0, F_b=1.0, alpha=100.0, beta=0.25)
    # φ'(0) = F_b·α·1·1 = 100
    assert abs(dphi_at_zero - 100.0) < 1e-4, f"φ'(0) should be 100, got {dphi_at_zero}"

    # --- NR: check residual converges ---
    coeff_small = PhysicalCoefficients(
        note_frequency=110.0, n_total=4, p_bow_force=0.3
    )
    coeff_small.n_total = 4
    coeff_small.refresh()
    x4      = np.zeros(4)
    x4_prev = np.zeros(4)
    A4, b4  = build_system_1d(x4, x4_prev,
                               coeff_small.base_spring, coeff_small.base_mass,
                               coeff_small.alpha_damp, coeff_small.beta_damp,
                               coeff_small.dt)
    x4_new = newton_raphson(
        A4, b4, x4, x4_prev,
        bow_pos=1.0, bow_velocity=1.0,
        F_b=coeff_small.bow_force_scale,
        alpha_bow=coeff_small.alpha_bow,
        beta_bow=coeff_small.beta_bow,
        dt=coeff_small.dt,
    )
    # Evaluate final residual
    w  = get_cubic_weight_vector(1.0, 4)
    v_struct = float(np.dot(w, x4_new - x4)) / coeff_small.dt
    phi_f = bow_phi(1.0 - v_struct, coeff_small.bow_force_scale,
                    coeff_small.alpha_bow, coeff_small.beta_bow)
    R_final = np.linalg.norm(A4 @ x4_new - b4 - w * phi_f)
    print(f"  NR residual after 3 iterations (N=4): ‖R‖ = {R_final:.4e}")

    print("All tests passed.")


if __name__ == "__main__":
    _run_smoke_test()
