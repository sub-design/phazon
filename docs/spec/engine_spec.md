# Atoms Engine Spec v1.0

> **Тип**: Reverse engineering specification
> **Статус**: Реконструкция. Не дословный исходник.
> **Источники**: stripped binary, методы NetworkIf, реальные .baby preset files, tooltip strings
> **Дата**: 2025

---

## TL;DR

Atoms — это **MPE-aware nonlinear physical-modeling synthesizer**.

Каждая нота = отдельная дискретная **mass-spring сеть**, возбуждаемая **bowed contact interaction**, решаемая **неявно через Newton–Raphson** на каждом сэмпле.

Это не sampler. Не wavetable. Не subtractive synth.

---

## 1. Что это за класс системы

```
Atoms = real-time implicit nonlinear lattice solver

Каждая нота:
├── дискретная физическая сеть (1D chain / 2D lattice)
├── возбуждение: bowed contact friction (nonlinear)
├── решение: implicit Newton-Raphson
└── readout: интерполяция в движущейся точке
```

Все музыкальные параметры управляют **коэффициентами системы**, а не сигналом напрямую.

---

## 2. Верхнеуровневая архитектура

```
PluginAudioProcessor
├── AudioProcessorValueTreeState  (параметры)
├── ParameterManager
├── preset system (.baby)
└── BASynth::MPESynth
    ├── voice allocation / mono-poly
    ├── MPE routing
    └── BASynth::MPESynthVoice (per note)
        ├── gain / fade / RMS
        └── NetworkIf  ← весь DSP здесь
            ├── refreshCoefficients()
            ├── calculateFullSystem*()
            ├── bow()
            ├── NR*()
            ├── getROutput()
            └── interpolation/extrapolation
```

→ Подробнее: [[architecture]]

---

## 3. Физическая модель

### 3.1 State

```
xⁿ    ∈ ℝᴺ  — displacement вектор (N узлов)
xⁿ⁻¹  ∈ ℝᴺ  — предыдущий шаг
vⁿ = (xⁿ - xⁿ⁻¹) / dt
```

### 3.2 Structural matrices

```
M ∈ ℝᴺˣᴺ  — mass matrix (likely diagonal)
K ∈ ℝᴺˣᴺ  — stiffness (topology: 1D chain, 2D lattice, anisotropic)
C ∈ ℝᴺˣᴺ  — damping
```

### 3.3 Уравнение движения

```
M·ẍ + C·ẋ + K·x + n(x,ẋ) = F_bow(t)
```

### 3.4 Дискретизация (implicit)

```
A = M/dt² + C/dt + K
b = M*(2xⁿ - xⁿ⁻¹)/dt² + C*xⁿ/dt

→  A·xⁿ⁺¹ = b + F_bow(xⁿ⁺¹)   [нелинейная система]
```

---

## 4. Bow Contact Model

### 4.1 Позиция смычка

```
p_bow(t) = ExcitationRatio
         + MovementDEPTH * LFO_move(t)
         + σ_p * ξ(t)          ← Chaos
```

### 4.2 Relative velocity

```
v_rel = v_bow - v_struct(p_bow)

v_struct(p) = B · v    ← interpolation3, read operator
```

### 4.3 Friction law (working model, не доказано буквально)

```
φ(v) = F_b * (tanh(α·v) + β·tanh³(α·v))

φ'(v) = F_b * α * (1 - tanh²(α·v)) * (1 + 3β·tanh²(α·v))
```

Mapping параметров:
```
F_b ← BowForce
α   ← BowForce (friction slope)
β   ← Nonlinearity
```

### 4.4 Force injection

```
F_bow_nodes = Gᵀ · φ(v_rel)    ← interpolation3Mult, write operator
```

---

## 5. Nonlinear Solver NR()

### 5.1 Residual

```
R(xⁿ⁺¹) = A·xⁿ⁺¹ - b - Gᵀ · φ(v_bow - B·(xⁿ⁺¹-xⁿ)/dt) = 0
```

### 5.2 Jacobian

```
J = A + Gᵀ · φ'(u) · B/dt

где u = v_bow - B·(xⁿ⁺¹-xⁿ)/dt
```

### 5.3 Newton step

```
J · δx = -R
x_{k+1} = x_k + δx
```

### 5.4 Real-time оптимизации (вероятно)

- Ограниченное число итераций: 2–4
- Reuse factorization линейной части A
- Возможно rank-1 / Sherman-Morrison update для нелинейной коррекции

Варианты: `NR()`, `NRAnisotropic()`, `NREco()`

---

## 6. Readout

```
y(t) = interpolation3(x, p_pickup(t))

p_pickup(t) = pickupBase + ModulationDEPTH * LFO_mod(t)
```

Движущийся пикап → chorus-like effect.

---

## 7. Temporal Modulation

### Movement (bow position)
```
p_bow(t) = ExcitationRatio + MovementDEPTH * LFO_move(t)
```

### Modulation (pickup position)
```
p_pickup(t) = base + ModulationDEPTH * LFO_mod(t)
```

### Vibrato (stiffness modulation)
```
K(t) = K_0 * (1 + VibratoDEPTH * LFO_vib(t))
```

Это **не post pitch LFO** — модулируется сама жёсткость сети.

---

## 8. Chaos

Smoothed stochastic perturbation физической модели (не post-noise).

```
K_eff(t)     = K_0 * (1 + σ_K * ξ(t))
α_bow_eff(t) = α_0 * (1 + σ_α * ξ(t))
p_bow_eff(t) = p_bow(t) + σ_p * ξ(t)

ξ(t) = smoothed random process
```

Отключён в EcoMode — значит вычислительно дорогой или дестабилизирующий solve.

---

## 9. Post Processing

```
y → Drive    (nonlinear waveshaping / harmonic clipping)
  → Filter   (low-pass + resonance)
  → Space    (reverb, SpaceMix + SpaceSize)
```

---

## 10. Pitch Mechanism

```
ω₀ ∝ √(k/m)   →   pitch через масштабирование K, не через playback rate

K_eff = K_0 · s_pitch²

s_pitch = 2^((note + octave - root) / 12) · tune · mpeBend · detune
```

---

## 11. Structural Modes

| Mode | Функция | Тип сети |
|---|---|---|
| 1D Anisotropic | `calculateFullSystem1DAnisotropic()` | Цепь (string-like) |
| 2D | `calculateFullSystem2D()` | Решётка (membrane) |
| 2D Anisotropic | `calculateFullSystem2DAnisotropic()` | Анизотропная пластина |
| Eco | `calculateFullSystemEco()` | Reduced order, без Chaos |

---

## 12. Полный сигнальный путь

```
params
  ↓
refreshCoefficients()
  ↓
for each sample:
    movement / modulation / vibrato / chaos
    ↓
    calculateFullSystem*()
    ↓
    bow()
    ↓
    NR solve
    ↓
    getROutput()
    ↓
    drive → filter → space
```

→ Подробнее с псевдокодом: [[signal_flow]]

---

## 13. Preset System

`.baby` = JUCE ValueTree XML snapshot параметров. Нет audio assets.

```xml
<STATE>
  <PARAM id="springDamping" value="0.43"/>
  <PARAM id="bowForce"      value="0.55"/>
  ...
</STATE>
```

→ Все paramID: [[parameters]]

---

## 14. Confidence Map

| Компонент | Уверенность |
|---|---|
| Тип системы: physical modeling | ✅ установлено |
| NetworkIf как DSP ядро | ✅ установлено |
| 1D/2D/anisotropic/eco режимы | ✅ установлено |
| Bowed nonlinear excitation | ✅ установлено |
| Interpolation coupling | ✅ установлено |
| Stiffness-based vibrato | ✅ установлено |
| Два слоя damping | ✅ установлено |
| Implicit solver | ✅ высокая уверенность |
| Форма φ(v): tanh-based | ⚠️ рабочая гипотеза |
| Профиль таблиц M/K | ⚠️ реконструкция |
| Exact unknown vector в NR | ⚠️ реконструкция |
| Rank-1 оптимизация | ⚠️ вероятно |
| Точная реализация Chaos | ❌ не вскрыто |
| Cubic kernel interpolation3* | ❌ не вскрыто |

→ Подробнее: [[research/confidence]]
