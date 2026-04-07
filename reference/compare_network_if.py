from __future__ import annotations

import math
import pathlib
import subprocess
import sys

import numpy as np

from network_if_reference import PhysicalCoefficients, render_sample


def load_cpp_trace(binary: pathlib.Path) -> tuple[np.ndarray, np.ndarray]:
    proc = subprocess.run([str(binary), "trace"], capture_output=True, text=True, check=True)
    outputs: list[float] = []
    states: list[list[float]] = []

    for line in proc.stdout.splitlines():
        if not line.startswith("TRACE "):
            continue
        _, _, sample, *state = line.split()
        outputs.append(float(sample))
        states.append([float(v) for v in state])

    if not outputs:
        raise RuntimeError("trace output was empty")

    return np.array(outputs), np.array(states)


def build_python_trace() -> tuple[np.ndarray, np.ndarray]:
    coeff = PhysicalCoefficients(
        p_spring_damping=0.18,
        p_mass_damping=0.015,
        p_bow_force=0.42,
        p_nonlinearity=0.33,
        note_frequency=440.0 * 2.0 ** ((60 - 69) / 12.0),
        n_total=8,
        sample_rate=48000.0,
    )

    x = np.zeros(8)
    x_prev = np.zeros(8)
    outputs: list[float] = []
    states: list[np.ndarray] = []

    for _ in range(128):
        y, x, x_prev = render_sample(
            x=x,
            x_prev=x_prev,
            coeff=coeff,
            bow_pos_norm=0.31,
            bow_velocity=1.5,
            pickup_pos_norm=0.70,
            mode="1d",
        )
        outputs.append(float(y))
        states.append(np.array(x, copy=True))

    return np.array(outputs), np.vstack(states)


def main() -> int:
    binary = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path("build/phazon_verify")
    cpp_outputs, cpp_states = load_cpp_trace(binary)
    py_outputs, py_states = build_python_trace()

    if cpp_outputs.shape != py_outputs.shape or cpp_states.shape != py_states.shape:
        print("FAIL shape mismatch")
        return 1

    output_error = np.max(np.abs(cpp_outputs - py_outputs))
    state_error = np.max(np.abs(cpp_states - py_states))

    print(f"max_output_error={output_error:.9e}")
    print(f"max_state_error={state_error:.9e}")

    ok = output_error < 1.0e-5 and state_error < 1.0e-5
    print("PASS nr_compare" if ok else "FAIL nr_compare")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
