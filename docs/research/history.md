# History — Хронология выводов

> Последовательность открытий, включая ранние ошибки.
> Важно сохранять: показывает, почему текущая модель надёжнее предыдущей.

---

## Фаза 1: Анализ бинаря, ранняя архитектура

**Источники**: 001.md, 002.md, 003.md

### Что нашли
- Полная JUCE-архитектура: `StandaloneFilterApp` → `PluginAudioProcessor` → `BASynth::MPESynth`
- UI классы: `Crosshair`, `LFOslider`, `ControlPanel`, `PresetManagerView`
- License система: `WAPNagscreen`, `LicenseManager`, trial = 5 sec silence / 60 sec
- В бинаре есть `juce::SamplerSound`, `juce::SamplerVoice`

### Ранняя ошибочная гипотеза
> "Atoms — sample-based instrument. Sampler + MPE synth."

Казалось убедительным: SamplerSound/SamplerVoice в JUCE — стандартные классы для sampler-архитектуры.

---

## Фаза 2: Ключевой перелом — строка из бинаря

**Источник**: 005.md

### Находка
Строка из бинаря:
> *"Welcome to Atoms - a physical modeling synthesizer. Atoms simulates a virtual structure of masses and springs that produces sound when bowed."*

### Вывод
Это первое прямое подтверждение: Atoms — physical modeling, а не sampler.

`SamplerSound/Voice` — вероятно часть JUCE library, не источник звука.

### Также в этой фазе
- Найдены реальные классы DSP ядра: `NetworkIf`, `BASynth::MPESynthVoice`
- Найдены методы: `bow()`, `NR()`, `calculateFullSystem*()`, `getROutput()`
- Подтверждена MPE архитектура: pressure, pitchbend, timbre per note

---

## Фаза 3: Реальные .baby пресеты

**Источники**: 006.md, 007.md, 008.md

### Находка
Разбор реальных `.baby` preset файлов дал прямые paramID движка.

### Что получили
Полный список параметров:
```
springDamping, massDamping, excitationRatio, bowForce, bowWidth,
nonlinearity, drive, dimensions, massProfile, springProfile,
filterCutoff, filterResonance, SpaceMix, SpaceSize,
attack, release, monoMode, ecoMode, chaos, mpeMode, ...
```

### Что опровергли
- `reverbMix/reverbSize` → реальные ID: `SpaceMix/SpaceSize`
- "Trumpet" и "Pad 3" — это GM labels из JUCE, не factory content Atoms
- Финальное опровержение sampler-гипотезы: в `.baby` нет ни одного audio asset

### Новое открытие: Crosshair
Crosshair — не фиксированный контрол, а **универсальный XY-макро**.
Назначение: X→SpaceMix, Y→SpaceSize (из пресетов); другие пары — Filter, LFO, Motion.

---

## Фаза 4: NetworkIf — реальный API ядра

**Источники**: 010.md, 011.md

### Находка
Дизасм `BASynth::MPESynthVoice::renderNextBlock()` показал прямой вызов:
```
Network<float>::renderNextBlock(juce::dsp::AudioBlock<float>&, int, int)
```

### Что это значит
- DSP ядро — отдельный объект сети, не inline в voice
- `MPESynthVoice` только оркестрирует: gain, fade, RMS, mix — физику не считает

### Полный API NetworkIf (из бинаря)
```
renderNextBlock, setupNote, stopNote, resetStates,
refreshCoefficients, calculateFullSystem1DAnisotropic,
calculateFullSystem2D, calculateFullSystem2DAnisotropic,
calculateFullSystemEco, bow, NR, NRAnisotropic, NREco,
getROutput, changeDimensions, changeProfile,
interpolation1/3, interpolation1/3Mult, extrapolation1/3/Mult
setParameterWithID, setHostSyncerProjectInfo
```

---

## Фаза 5: Параметры, Modulation architecture

**Источник**: 013.md

### Ключевые выводы
- **Movement** = движение точки возбуждения (bow position)
- **Modulation** = движение точки съёма (pickup position) — источник chorus-like effect
- **Vibrato** = модуляция stiffness (`K(t) = K₀*(1 + A*LFO(t))`) — не post pitch LFO
- Каждый параметр имеет automation wrapper: MIN/MAX/RATE/TYPE/SYNC/INV/LFOmode

### Automation modes
```
OFF / LFO / MPE / DRIFT
```

---

## Фаза 6: Математическая реконструкция

**Источники**: 014.md–018.md, 020.md–023.md

### Последовательность
1. **014**: whitepaper-реконструкция — уравнения M, K, C, дискретизация, NR
2. **015**: матрицы для 1D/2D режимов (tridiagonal, 5-point stencil)
3. **016**: Jacobian и residual NR — формальная запись
4. **017**: NR оптимизации для real-time (rank-1, frozen Jacobian, 2-4 итерации)
5. **018**: Bow friction law — кандидаты φ(v), разделение bow vs Drive
6. **020**: calculateFullSystem*() — система уравнений одного шага
7. **021**: полный NR псевдокод с bow() внутри
8. **022**: interpolation3Mult() — read/write operator для continuous coupling
9. **023**: Chaos — реконструкция по уликам (не post-noise, а coefficient perturbation)

### Почему порядок важен
Правильный порядок анализа (из 019.md):
```
calculateFullSystem*()  →  bow()  →  NR()  →  refreshCoefficients()
```
NR() без знания системы и нелинейности — анализ вслепую.

---

## Фаза 7: Итоговая спецификация

**Источники**: 024.md, 025.md

### Результат
Полная engineering spec с:
- Уравнениями
- Parameter mapping таблицей
- Signal flow
- Minimal reimplementation blueprint
- Confidence map

### Финальное определение
> Atoms — это неявно интегрируемая нелинейная физическая сеть с контактным возбуждением,
> где все музыкальные параметры управляют не сигналом напрямую, а коэффициентами системы,
> которая генерирует звук сама.

---

## Сводная таблица перелётных точек

| Фаза | Источники | Ключевой вывод |
|---|---|---|
| 1 | 001–003 | Архитектура JUCE, UI классы. Ошибочная sampler-гипотеза |
| 2 | 005 | **Перелом**: строка "physical modeling synthesizer" |
| 3 | 006–008 | Реальные paramID из пресетов, опровержение sampler |
| 4 | 010–011 | NetworkIf API, подтверждение physical core |
| 5 | 013 | Movement/Modulation/Vibrato — физический смысл |
| 6 | 014–023 | Математика: уравнения, NR, bow law, interpolation |
| 7 | 024–025 | Engineering spec v1.0 |
