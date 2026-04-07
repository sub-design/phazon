# Methodology — Как копать дизасм

> Оптимальный порядок и подход для продолжения reverse engineering.
> Источник: выводы из 019.md + опыт фаз 4–6.

---

## Принцип: копать снизу вверх

NR() — это только решатель. Чтобы его понять, надо сначала знать:
1. **Что** он решает (`calculateFullSystem*()`)
2. **Какую нелинейность** вносит (`bow()`)

Без этого — анализ residual и Jacobian вслепую.

---

## Оптимальный порядок функций

```
1. calculateFullSystem*()   ← скелет математики
2. bow()                    ← нелинейность
3. NR()                     ← solver (после двух предыдущих прозрачен)
4. refreshCoefficients()    ← UI → physics mapping
5. interpolation3Mult()     ← coupling operators
6. Chaos                    ← perturbation layer
```

---

## 1. calculateFullSystem*()

### Что мы хотим узнать
- Unknown vector: только `x^{n+1}` или `[x^{n+1}, v^{n+1}]`?
- Структура матрицы A: dense / sparse / banded / block?
- Чем 1D и 2D реально отличаются: только K? Или ещё M, boundary, topology?
- Anisotropic: разные коэффициенты по осям? Диагональные связи? Biharmonic term?
- Boundary conditions: фиксированные концы? Периодические? Free?

### Что ожидать найти
```c++
// Вероятная структура:
A = M/dt² + C/dt + K;
b = M*(2*x_n - x_nm1)/dt² + C*x_n/dt;
// + возможные буферы для residual, temporary state
```

### Почему это открывает NR
Если подтвердится эта форма → NR почти автоматически сужается до:
- обычный Newton solve
- rank-1 corrected solve
- partially frozen Jacobian

---

## 2. bow()

### Что мы хотим узнать
- Точная формула φ(v_rel)
- Форма BowWidth kernel (Gaussian? tent? cubic?)
- Как Attack влияет: onset ramp на F_b? или на bow position onset?
- Где именно входит Nonlinearity: β в φ(v)? или дополнительный internal nonlinear term?
- Возвращает bow() force vector или модифицирует b in-place?

### Что ожидать найти
```c++
// Вероятно:
float p_bow = excitationRatio + movement_mod;
float v_struct = interpolation3(vel_array, N, p_bow);  // read B
float v_rel = v_bow - v_struct;
float f_contact = bowForce * (tanh(alpha*v_rel) + beta*tanh3(alpha*v_rel));
interpolation3Mult(force_array, N, p_bow, f_contact);  // write G^T
```

---

## 3. NR()

### Что мы хотим узнать (после системы и bow)
- Точный residual (подтвердить гипотезу)
- Jacobian: full dense solve или rank-1 correction?
- Predictor: есть ли начальное приближение лучше x_n?
- Точное число итераций
- Есть ли early stopping по tolerance?

### Ожидаемый паттерн
```c++
// predictor
x_new = 2*x_n - x_nm1;   // или просто x_n

for (int iter = 0; iter < MAX_ITER; iter++) {
    // residual
    float v_rel = v_bow - B*(x_new - x_n)/dt;
    float f = phi(v_rel);
    R = A*x_new - b - G_T * f;

    // Jacobian (rank-1 or full)
    J = A + G_T * dphi(v_rel) * B / dt;

    // solve
    x_new += solve(J, -R);
}
```

---

## 4. refreshCoefficients()

### Что мы хотим узнать
- Точный нелинейный mapping UI value → physical coefficient
  - Например: `BowForce` — линейный или log/exp?
  - `SpringDamping` — нормализованный или direct?
- Как Profile выбирает базовые таблицы M, K
- Кэшируется ли A целиком или только отдельные компоненты?
- Как pitch scaling применяется к K

---

## 5. interpolation3Mult()

### Что мы хотим узнать
- Точный cubic kernel (Catmull-Rom? B-spline? другой?)
- Boundary handling (clamp? reflect? wrap?)
- Mult: разбрасывает в соседние N или N+1 узлов?

---

## 6. Chaos

### Что мы хотим узнать
- Тип random process: filtered white noise? Perlin? LCG?
- Точная точка входа: до NR? Внутри solve? После?
- σ-масштабирование: global или per-coefficient?

---

## Практические советы для дизасма

### Начинать с сигнатур
Методы NetworkIf stripped, но сигнатуры говорят много:
- `calculateFullSystem1DAnisotropic()` — нет аргументов → берёт state из `this`
- `changeProfile(Profile, vector<float>&, float&, int, string)` → принимает profile + ref arrays
- `setParameterWithID(int, float, bool)` → int ID, float value, bool немедленно

### Искать паттерны
- Loads from float arrays → likely M, K, C coefficient tables
- SIMD intrinsics → matrix-vector multiply (уточняет dense vs sparse)
- Small loops (2–4 iterations) → NR iteration count
- tanh / exp calls → friction law или waveshaping

### Инструменты
- Ghidra / IDA: деманглинг C++ имён для `BASynth::`, `NetworkIf::` символов
- Binary Ninja: если stripped — паттерн-matching по calling conventions
- `strings` + `nm` уже дали хороший слой (фазы 1–3)
- Следующий слой: CFG + basic block analysis конкретных функций

---

## Чего не делать

- **Не начинать с NR()** — без `calculateFullSystem*()` и `bow()` анализ вслепую
- **Не доверять GM instrument names** в строках — это JUCE tables, не Atoms content
- **Не принимать наличие `juce::SamplerSound`** как доказательство sampler-архитектуры
