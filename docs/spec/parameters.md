# Parameters

> Источник: реальные `.baby` preset files + tooltip strings из бинаря + методы NetworkIf.
> Все paramID взяты из реальных пресетов, не из догадок.

---

## Automation wrapper (universal)

Большинство звуковых параметров имеют обвязку вида:

```
PARAM          ← base value
PARAMMIN       ← нижняя граница automation
PARAMMAX       ← верхняя граница automation
PARAMRATE      ← скорость LFO/motion
PARAMTYPE      ← тип осциллятора
PARAMSYNC      ← sync к host tempo
PARAMINV       ← инверсия
PARAMLFOmode   ← free / retrigger / hold
PARAMLOCK      ← защита от рандомизации
```

Итоговое значение параметра в каждый момент времени:
```
param(t) = base + LFO/motion + MPE_per_note + drift
```

---

## 1. Physical Model Core

| ParamID | Физический смысл | Где в коде | Эффект на звук | Уверенность |
|---|---|---|---|---|
| `springDamping` | Демпфирование пружин | `refreshCoefficients()` | Decay, затухание резонансов | ✅ высокая |
| `massDamping` | Демпфирование масс (ВЧ) | `refreshCoefficients()` | Сухость, убирает звонкость | ✅ высокая |
| `excitationRatio` | Нормализованная позиция смычка | `bow()` | Спектр: положение возбуждения | ✅ высокая |
| `excitationLocation` | Абсолютная позиция контакта | `bow()` | Тембральная окраска | ✅ высокая |
| `bowForce` | Давление/сила смычка | `bow()` | Трение, шумность, агрессия | ✅ высокая |
| `bowWidth` | Ширина контакта смычка | `bow()` force distribution | Характер атаки, шероховатость | ⚠️ средняя |
| `nonlinearity` | Степень нелинейности | `bow()` + solve residual | Богатство, грязь, нестабильность | ✅ высокая |
| `drive` | Нелинейный post-stage | post model | Насыщение, плотность | ✅ высокая |
| `dimensions` | Размер/разрешение сети | `changeDimensions()` | Плотность резонансов, тембр | ⚠️ средняя |
| `massProfile` | Профиль распределения масс | `setMassProfile()` | Инерция, modal structure | ✅ высокая |
| `springProfile` | Профиль жёсткостей/связей | `setSpringProfile()` | Гармоничность, "материал" | ✅ высокая |

### Profile (structural mode)

Управляет топологией всей сети. Вызывает `changeProfile()` → разные `calculateFullSystem*()`.

| Profile value | Тип сети | Функция |
|---|---|---|
| 1D | Струна/цепь | `calculateFullSystem1DAnisotropic()` |
| 2D | Мембрана/пластина | `calculateFullSystem2D()` |
| 2D anisotropic | Анизотропная пластина | `calculateFullSystem2DAnisotropic()` |
| Eco | Упрощённая | `calculateFullSystemEco()` |

---

## 2. Movement (движение точки возбуждения)

Формула:
```
p_bow(t) = ExcitationRatio + MovementDEPTH * LFO(MovementFREQ, TYPE, SYNC, INV)
```

| ParamID | Смысл | Уверенность |
|---|---|---|
| `MovementFREQ` | Частота движения смычка | ✅ |
| `MovementDEPTH` | Амплитуда движения | ✅ |
| `MovementSYNC` | Sync к host tempo | ✅ |
| `MovementTYPE` | Форма LFO | ✅ |
| `MovementINV` | Инверсия | ✅ |
| `MovementMIN/MAX/RATE/LFOmode` | Automation wrapper | ✅ |

---

## 3. Modulation (движение точки съёма)

Формула:
```
p_pickup(t) = pickupBase + ModulationDEPTH * LFO(ModulationFREQ, TYPE, SYNC, INV)
```

Это источник "chorus-like effect" (tooltip: *"Varies the pickup location along the mass-spring network"*).

| ParamID | Смысл | Уверенность |
|---|---|---|
| `ModulationFREQ` | Частота движения пикапа | ✅ |
| `ModulationDEPTH` | Амплитуда движения | ✅ |
| `ModulationSYNC` | Sync к host | ✅ |
| `ModulationTYPE` | Форма LFO | ✅ |
| `ModulationINV` | Инверсия | ✅ |
| `ModulationMIN/MAX/RATE/LFOmode` | Automation wrapper | ✅ |

---

## 4. Vibrato (модуляция жёсткости)

Это **не post pitch LFO**. Это модуляция stiffness коэффициента.

Формула:
```
K(t) = K_0 * (1 + VibratoDEPTH * LFO(VibratoFREQ, TYPE, SYNC, INV))
```

Tooltip подтверждает: *"varies the spring stiffness to create pitch vibrato"*.

| ParamID | Смысл | Уверенность |
|---|---|---|
| `vibratoFreq` | Частота vibrato | ✅ |
| `vibratoDepth` | Глубина | ✅ |
| `vibratoSync` | Sync | ✅ |
| `VibratoTYPE` | Форма LFO | ⚠️ средняя |
| `VibratoINV` | Инверсия | ⚠️ средняя |
| `VibratoMIN/MAX/RATE/LFOmode` | Automation wrapper | ✅ |

---

## 5. Pitch и Note Setup

Pitch реализован через **масштабирование коэффициентов сети**, а не через sample playback rate:
```
ω₀ ∝ √(k/m)   →   K_eff = K_0 * s_pitch²
```

| ParamID | Смысл | Где | Уверенность |
|---|---|---|---|
| `octave` | Транспозиция по октавам | `setupNote()`, `refreshCoefficients()` | ✅ |
| `detune` | Микро-расстройка | note setup / coefficient scaling | ✅ |
| `attack` | Ramp на bow excitation (вероятно) | `setupNote()` / voice wrapper | ⚠️ средняя |
| `release` | Fade после note-off | `MPESynthVoice` | ✅ |
| `releaseLock` | Фиксация tail поведения | release handling | ⚠️ средняя |
| `monoMode` | Mono/poly голосовая логика | `MPESynth` | ✅ |

---

## 6. Filter

| ParamID | Смысл | Уверенность |
|---|---|---|
| `filterCutoff` | Частота среза | ✅ |
| `filterResonance` | Резонанс | ✅ |
| `filterType` | Тип фильтра / режим automation | ⚠️ средняя |
| `filterMin/Max` | Диапазон automation | ✅ |
| `filterInv` | Инверсия движения | ✅ |
| `filterLock` | Защита от рандомизации | ✅ |

---

## 7. Space / Reverb

Правильные paramID из реальных пресетов: `SpaceMix` / `SpaceSize` (не `reverbMix`/`reverbSize`).

| ParamID | Смысл | Уверенность |
|---|---|---|
| `SpaceMix` | Wet/dry mix | ✅ |
| `SpaceSize` | Размер/decay | ✅ |
| `SpaceMIN/MAX/RATE/TYPE/SYNC/INV/LFOmode` | Automation wrapper | ✅ |

Вероятное Crosshair назначение: X→SpaceMix, Y→SpaceSize.

---

## 8. Chaos и EcoMode

| ParamID | Смысл | Где | Уверенность |
|---|---|---|---|
| `chaos` | Стохастическая вариативность физической модели | system assembly / coefficient perturbation | ⚠️ средняя-высокая |
| `ecoMode` | Упрощённый режим сети | `calculateFullSystemEco()` | ✅ |

Важно: Chaos **отключён в EcoMode** — значит это внутренний слой модели, а не post-noise.

Наиболее вероятная модель Chaos:
```
K_eff(t)     = K_0 * (1 + σ_K * ξ(t))
α_bow_eff(t) = α_0 * (1 + σ_α * ξ(t))
p_bow_eff(t) = p_bow(t) + σ_p * ξ(t)

где ξ(t) — smoothed random process
```

---

## 9. MPE

| ParamID | Смысл | Уверенность |
|---|---|---|
| `mpeMode` | Включает per-note MPE routing | ✅ |
| `automationMode` | OFF / LFO / MPE / DRIFT | ✅ |
| `driftAmount` | Амплитуда drift модуляции | ✅ |

MPE применяется через `MPESynthVoice::applyMPEvalue(double)` — централизованная функция.

Источники модуляции на параметр:
```
param(t) = base + LFO + MPE_pressure + drift
```

---

## 10. Сводная таблица: param → место в псевдокоде

| ParamID | Блок |
|---|---|
| `Profile`, `massProfile`, `springProfile` | `changeProfile()`, `calculateFullSystem*()` |
| `springDamping`, `massDamping` | `refreshCoefficients()` |
| `dimensions` | `changeDimensions()` |
| `excitationRatio` | `bowPos = ...` |
| `bowForce`, `bowWidth` | `bow()` |
| `attack` | `bow()` onset ramp |
| `nonlinearity` | `bow()` + NR residual |
| `drive` | `applyDrive()` |
| `octave`, `detune` | note setup |
| `MovementFREQ/DEPTH/...` | `bowPos += osc(...)` |
| `ModulationFREQ/DEPTH/...` | `pickupPos += osc(...)` |
| `vibratoFreq/Depth/...` | `springScale = 1 + osc(...)` |
| `filterCutoff`, `filterResonance` | `applyFilter()` |
| `SpaceMix`, `SpaceSize` | post FX |
| `ecoMode` | выбор `calculateFullSystemEco()` |
| `chaos` | perturbation внутри system assembly |
| `mpeMode` | per-note modulation routing |
