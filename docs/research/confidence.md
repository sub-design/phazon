# Confidence Map

> Что реально установлено, что реконструкция, что опровергнуто.
> Это критически важно для агентов и следующих шагов дизасма.

---

## ✅ Установлено (высокая уверенность)

Эти выводы подтверждены несколькими независимыми источниками одновременно
(бинарь + методы + пресеты + tooltips):

### Тип движка
- Atoms — physical modeling synthesizer, не sampler, не wavetable
- Подтверждение: строка из бинаря *"a physical modeling synthesizer... simulates a virtual structure of masses and springs that produces sound when bowed"*
- Подтверждение: в `.baby` пресетах нет ссылок на wav/flac/ogg/sample assets
- Подтверждение: параметры — физические величины (springDamping, massDamping, bowForce и т.д.)

### NetworkIf как физическое ядро
- Метод `NetworkIf::renderNextBlock()` существует и вызывается из `MPESynthVoice::renderNextBlock()`
- Все DSP-вычисления в `NetworkIf`, не в `MPESynthVoice`

### Структурные режимы
- `calculateFullSystem1DAnisotropic()` — 1D chain
- `calculateFullSystem2D()` — 2D lattice
- `calculateFullSystem2DAnisotropic()` — 2D anisotropic
- `calculateFullSystemEco()` — reduced order mode

### Bowed nonlinear excitation
- Метод `bow()` существует как отдельная функция в `NetworkIf`
- Tooltip подтверждают: *"pressure of the bow on the mass-spring network"*
- BowForce, BowWidth, ExcitationRatio — подтверждены из пресетов

### Interpolation / continuous coupling
- `interpolation3()`, `interpolation3Mult()` — реальные методы NetworkIf
- Необходимы для дробных позиций bow и pickup

### Vibrato = stiffness modulation
- Tooltip: *"varies the spring stiffness to create pitch vibrato"*
- Не post pitch LFO

### Два слоя damping
- `springDamping` и `massDamping` — разные физические коэффициенты
- Оба подтверждены из пресетов

### Implicit solver
- `NR()`, `NRAnisotropic()`, `NREco()` — реальные методы
- Implicit solve необходим для стабильности nонлинейной bow-модели

### Preset format
- `.baby` = JUCE ValueTree XML
- Полный список paramID из реальных файлов

### Pitch через коэффициенты
- Нет sample playback rate manipulation
- Pitch = масштабирование K (stiffness) → ω₀ ∝ √(k/m)

---

## ⚠️ Высокая уверенность, но не буквально доказано

Эти выводы сильно подкреплены косвенными данными, но тело функции не вскрыто:

### Форма bow friction law φ(v)
**Гипотеза**: `φ(v) = F_b * (tanh(α·v) + β·tanh³(α·v))`

Почему вероятно:
- Стандартная Välimäki/Smith bow model
- Tooltip: *"Adds noisiness from increased friction"*
- BowForce контролирует F_b и α, Nonlinearity контролирует β

**Что не доказано**: точная формула φ(v) без декомпиляции `bow()`

### Rank-1 / Sherman-Morrison оптимизация в NR
**Гипотеза**: NR не решает полный dense Jacobian, а использует rank-1 update

Почему вероятно:
- Real-time constraint
- Единственная нелинейность — bow contact (rank-1 structure)
- Типичная практика для physical modeling синтов

**Что не доказано**: без тела `NR()`

### Chaos = coefficient perturbation (не post-noise)
**Гипотеза**: Chaos вносит stochastic perturbation в K, α, p_bow

Почему вероятно:
- Chaos отключён в EcoMode (значит влияет на solve, а не на output)
- Tooltip: *"Chaos is disabled in ECO mode"*

**Что не доказано**: точная реализация

### Attack = bow excitation ramp
**Гипотеза**: Attack масштабирует onset bow force, а не envelope на output

Почему вероятно: интеграция в physical model более музыкальна
**Что не доказано**: без тела функции

---

## ⚠️ Средняя уверенность (реконструкция)

### Точные матрицы M, K для 1D/2D режимов
Наиболее вероятные структуры:

1D K (tridiagonal, string-like):
```
K_1D: k_s * tridiag(-1, 2, -1)
```

2D K (5-point stencil, membrane):
```
K_2D: k_s * (standard 2D Laplacian)
```

Anisotropic: разные коэффициенты по осям, возможно biharmonic term.

**Статус**: вероятно, но не доказано без `calculateFullSystem*()`.

### BowWidth — spatial spread
**Гипотеза**: BowWidth задаёт ширину Gaussian/tent kernel для force distribution
**Статус**: логично по физике, не подтверждено из функции

### dimensions — размер сети N
**Гипотеза**: `changeDimensions(N)` устанавливает число узлов
**Статус**: вероятно, точный mapping неизвестен

---

## ❌ Опровергнуто

Это важная часть — что мы думали раньше, но оказалось неверным:

### ~~Atoms = sampler~~
**Ранняя гипотеза** (файлы 001–003): "Atoms — sample-based instrument, sampler + MPE synth"

**Почему думали так**: в бинаре есть `juce::SamplerSound`, `juce::SamplerVoice`

**Опровержение** (файл 008, пресеты): в `.baby` нет ни одного audio asset reference. Параметры — физические, не sample-mapping. `SamplerSound/Voice` вероятно присутствуют как часть JUCE library, но не являются источником звука.

### ~~Trumpet, Pad 3 = factory sample sources~~
**Ранняя гипотеза**: найденные строки "Trumpet", "Muted Trumpet", "Pad 3 (polysynth)" — это реальные sample banks Atoms

**Опровержение**: эти строки — стандартные General MIDI instrument labels, встроенные в JUCE/binary как таблица. Не factory content Atoms.

### ~~Time-stretch движок~~
**Гипотеза**: Atoms делает независимое pitch/time изменение

**Опровержение**: нет следов elastique/RubberBand/phase vocoder. Pitch = физика (коэффициенты), не playback rate.

### ~~reverbMix / reverbSize как paramID~~
**Ранняя гипотеза**: параметры назывались `reverbMix`, `reverbSize`

**Опровержение**: реальные ID из пресетов — `SpaceMix`, `SpaceSize`.

---

## ❌ Не вскрыто (открытые вопросы)

Функции, требующие буквальной декомпиляции для подтверждения:

| Функция | Ключевые вопросы |
|---|---|
| `calculateFullSystem*()` | Exact unknown vector? Dense/sparse/banded A? Boundary conditions? |
| `bow()` | Точная формула φ(v)? Форма BowWidth kernel? |
| `NR()` | Rank-1 или full Jacobian? Точное число итераций? Predictor? |
| `refreshCoefficients()` | Exact mapping UI→physical? Нелинейности преобразования? |
| `interpolation3*()` | Exact cubic kernel? Boundary handling? |
| Chaos | Где именно входит? Тип random process? |
