# Atoms Engine — Reverse Engineering Docs

> **BABY Audio — Atoms VST/AU**
> Reverse engineering через бинарь + дизасм + .baby пресеты

---

## Что это

Структурированная документация результатов reverse engineering синтезатора Atoms.
Источники: stripped binary, имена методов, .baby preset files, tooltip strings из бинаря.

Это **не исходный код Atoms**. Это наиболее сильная инженерная реконструкция.

---

## Статус

| Слой | Статус |
|---|---|
| Архитектура (классы, JUCE) | ✅ установлено |
| Physical model type | ✅ установлено |
| Полный список параметров (.baby) | ✅ установлено |
| Signal flow / pipeline | ✅ высокая уверенность |
| NR solver structure | ✅ высокая уверенность |
| Bow friction law φ(v) | ⚠️ реконструкция, не доказано |
| Точные матрицы M, K, C | ⚠️ реконструкция |
| Тела calculateFullSystem*() | ❌ не вскрыты |
| Тело NR() | ❌ не вскрыто |
| Тело bow() | ❌ не вскрыто |

---

## Навигация

```
docs/
├── README.md                   ← ты здесь
│
├── spec/
│   ├── engine_spec.md          ← ГЛАВНЫЙ ДОКУМЕНТ. Полная инженерная спека v1.0
│   ├── architecture.md         ← Классы, JUCE structure, UI слои
│   ├── parameters.md           ← Все paramID, физический смысл, mapping
│   └── signal_flow.md          ← Pipeline, solver, псевдокод renderNextBlock
│
├── research/
│   ├── confidence.md           ← Что доказано / гипотезы / опровергнуто
│   ├── history.md              ← Хронология выводов, включая ранние ошибки
│   └── methodology.md          ← Как копать дизасм, порядок функций
│
├── disasm/
│   └── targets.md              ← Список функций для следующего дизасма
│
└── next_steps.md               ← Что делать дальше
```

---

## Ключевой вывод (одной строкой)

> Atoms — это **MPE-aware nonlinear physical-modeling synthesizer**.
> Каждая нота = отдельная дискретная mass-spring сеть, возбуждаемая bowed contact,
> решаемая неявно через Newton–Raphson на каждом сэмпле.

Это **не sampler**, **не wavetable**, **не subtractive synth**.
