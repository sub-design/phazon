# Signal Flow

> Полный DSP-пайплайн одного голоса Atoms.
> Источники: методы NetworkIf из бинаря, .baby пресеты, tooltip strings.

---

## 1. Верхний уровень (per note)

```
MIDI / MPE input
    ↓
BASynth::MPESynth  (voice allocation, mono/poly, host sync)
    ↓
BASynth::MPESynthVoice::renderNextBlock()
    ├── проверка активности голоса
    ├── подготовка внутреннего буфера
    ├── NetworkIf::renderNextBlock()   ← вся физика здесь
    ├── smoothed gain / fade in-out
    ├── RMS measurement (voice death logic)
    └── mix в output buffer
```

---

## 2. NetworkIf::renderNextBlock() — полный pipeline

### 2.1 Инициализация ноты (один раз при note-on)

```
setupNote(pitch, velocity, tuning, isMono)
    ↓
apply Octave, Detune
    ↓
changeProfile(Profile, ...)   ← выбор топологии сети
    ↓
changeDimensions(N)           ← размер сети
    ↓
refreshCoefficients()         ← UI params → physical coefficients
    ├── map BowForce → bowForceScale, α_bow
    ├── map SpringDamping → damping coeff
    ├── map MassDamping → HF damping coeff
    ├── map Nonlinearity → β (friction curve curvature)
    ├── map Octave/Detune → pitch scale → K_eff
    ├── build M, K, C matrices
    └── precompute A = M/dt² + C/dt + K
```

### 2.2 Per-sample loop

```
for each sample:

    ┌─ MODULATION UPDATE ─────────────────────────────────────────┐
    │  bowPos    = ExcitationRatio                                 │
    │            + MovementDEPTH * LFO(MovementFREQ, TYPE, SYNC)  │
    │            + σ_p * ξ(t)          ← Chaos                    │
    │                                                              │
    │  pickupPos = pickupBase                                      │
    │            + ModulationDEPTH * LFO(ModFREQ, TYPE, SYNC)     │
    │                                                              │
    │  K(t) = K_0 * (1 + VibratoDEPTH * LFO(VibratoFREQ, ...))   │
    │  → update spring coefficients                                │
    └─────────────────────────────────────────────────────────────┘
              ↓
    ┌─ SYSTEM ASSEMBLY ──────────────────────────────────────────┐
    │  if EcoMode:                                                │
    │      calculateFullSystemEco()                               │
    │  elif 1D anisotropic:                                       │
    │      calculateFullSystem1DAnisotropic()                     │
    │  elif 2D:                                                   │
    │      calculateFullSystem2D()                                │
    │  else:                                                      │
    │      calculateFullSystem2DAnisotropic()                     │
    │                                                             │
    │  Строит: A = M/dt² + C/dt + K                              │
    │          b = M*(2xⁿ - xⁿ⁻¹)/dt² + C*xⁿ/dt                │
    └─────────────────────────────────────────────────────────────┘
              ↓
    ┌─ BOW EXCITATION ───────────────────────────────────────────┐
    │  bow()                                                      │
    │  ├── compute bow position (bowPos)                          │
    │  ├── read structure velocity at bowPos:                     │
    │  │   v_struct = B · v  (via interpolation3)                 │
    │  ├── v_rel = v_bow - v_struct                               │
    │  ├── φ(v_rel) = F_b*(tanh(αv) + β*tanh³(αv))   ← working  │
    │  │              model, не доказано буквально                │
    │  └── scatter force into grid:                               │
    │      F_bow_nodes = Gᵀ · φ(v_rel)  (via interpolation3Mult) │
    └─────────────────────────────────────────────────────────────┘
              ↓
    ┌─ NONLINEAR SOLVE ──────────────────────────────────────────┐
    │  NR() / NRAnisotropic() / NREco()                           │
    │                                                             │
    │  Решает:                                                    │
    │  R(xⁿ⁺¹) = A·xⁿ⁺¹ - b - Gᵀ·φ(v_bow - B·(xⁿ⁺¹-xⁿ)/dt) = 0│
    │                                                             │
    │  Jacobian:                                                  │
    │  J = A + Gᵀ · φ'(u) · B/dt                                 │
    │                                                             │
    │  Newton step: J·δx = -R                                    │
    │  x_{k+1} = x_k + δx                                        │
    │                                                             │
    │  Итераций: вероятно 2-4 (real-time constraint)              │
    │  Оптимизация: возможно rank-1 / Sherman-Morrison            │
    └─────────────────────────────────────────────────────────────┘
              ↓
    ┌─ READOUT ──────────────────────────────────────────────────┐
    │  y(t) = getROutput(pickupPos)                               │
    │       = interpolation3(x, p_pickup(t))                     │
    └─────────────────────────────────────────────────────────────┘
              ↓
    ┌─ POST PROCESSING ──────────────────────────────────────────┐
    │  y = applyDrive(y, drive)       ← nonlinear waveshaping    │
    │  y = applyFilter(y, filterCutoff, filterResonance)         │
    │  output[n] = y                                              │
    └─────────────────────────────────────────────────────────────┘

    ┌─ POST BLOCK (после всех сэмплов) ─────────────────────────┐
    │  apply SpaceMix / SpaceSize  ← reverb/space                │
    │  voice fade / RMS check / clear logic                       │
    └─────────────────────────────────────────────────────────────┘
```

---

## 3. Физическая модель (state)

### State переменные
```
xⁿ   ∈ ℝᴺ   — смещения узлов (текущий шаг)
xⁿ⁻¹ ∈ ℝᴺ   — смещения (предыдущий шаг)
vⁿ = (xⁿ - xⁿ⁻¹) / dt   — скорости
```

### Structural coefficients
```
M ∈ ℝᴺˣᴺ  — матрица масс (likely diagonal)
K ∈ ℝᴺˣᴺ  — матрица жёсткостей (topology-dependent)
C ∈ ℝᴺˣᴺ  — damping (likely diagonal or banded)
```

### Уравнение движения
```
M·ẍ + C·ẋ + K·x + n(x,ẋ) = F_bow(t)
```

### Дискретизация (implicit)
```
ẍ ≈ (xⁿ⁺¹ - 2xⁿ + xⁿ⁻¹) / dt²
ẋ ≈ (xⁿ⁺¹ - xⁿ) / dt

A = M/dt² + C/dt + K
b = M*(2xⁿ - xⁿ⁻¹)/dt² + C*xⁿ/dt
```

---

## 4. Interpolation / Continuous-Discrete coupling

Позиции bow и pickup — **дробные**, не привязаны к конкретным узлам.

```
interpolation3(array, N, pos)
→  кубическое чтение: y = Σ wₖ(μ) · uₖ
   (read operator B)

interpolation3Mult(array, N, pos, weights)
→  кубическое распределение силы: uₖ += wₖ(μ) · f
   (write operator Gᵀ)
```

Это даёт:
- непрерывную точку смычка (Movement)
- непрерывный пикап (Modulation / chorus effect)
- элегантную структуру Jacobian в NR

---

## 5. Режимы сети

| calculateFullSystem*() | Тип | Характер звука |
|---|---|---|
| `calculateFullSystem1DAnisotropic()` | 1D chain | Струноподобный |
| `calculateFullSystem2D()` | 2D lattice | Мембрана/пластина |
| `calculateFullSystem2DAnisotropic()` | 2D anisotropic | Анизотропная пластина |
| `calculateFullSystemEco()` | Reduced order | Легковесный, Chaos выключен |

---

## 6. Pitch механизм

Pitch **не через sample playback rate**, а через масштабирование коэффициентов:
```
ω₀ ∝ √(k/m)
K_eff = K_0 · s_pitch²
```

При note-on:
```
s_pitch = 2^((targetNote + octave - rootNote) / 12) · globalTune · mpePitchBend · detune
```

---

## 7. Minimal reimplementation blueprint

```python
for each sample:
    updateParams()             # movement, modulation, vibrato, chaos

    A, b = buildLinearSystem() # calculateFullSystem*()

    x = predictor(x_n, x_nm1) # initial guess for NR

    for i in range(2, 4):      # NR iterations
        v_struct = B @ (x - x_n) / dt
        v_rel    = v_bow - v_struct
        f        = phi(v_rel)          # bow friction law
        R        = A @ x - b - G.T @ f
        J        = A + G.T @ dphi(v_rel) @ B / dt
        x       += solve(J, -R)

    y = readout(x, p_pickup)   # getROutput()
    y = drive(y)
    y = filter(y)
    y = space(y)
```
