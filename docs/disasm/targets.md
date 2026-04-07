# Disasm Targets

> Функции для следующего этапа дизасма, с приоритетом и конкретными вопросами.
> Порядок важен — см. [[research/methodology]].

---

## Priority 1 — calculateFullSystem*()

**Функции:**
- `NetworkIf::calculateFullSystem1DAnisotropic()`
- `NetworkIf::calculateFullSystem2D()`
- `NetworkIf::calculateFullSystem2DAnisotropic()`
- `NetworkIf::calculateFullSystemEco()`

**Что подтвердить:**
- [ ] Unknown vector = только `x^{n+1}` или `[x^{n+1}, v^{n+1}]`?
- [ ] Структура A: dense / sparse / banded / tridiagonal?
- [ ] Чем 1D и 2D отличаются: только K? Или M, boundaries, topology?
- [ ] Anisotropic: разные коэффициенты по осям? Biharmonic term?
- [ ] Boundary conditions: fixed ends? Free? Periodic?
- [ ] Eco: что конкретно упрощено vs full mode?

---

## Priority 2 — bow()

**Функция:** `NetworkIf::bow()`

**Что подтвердить:**
- [ ] Точная форма φ(v_rel): tanh? tanh³? другое?
- [ ] Как BowWidth входит: Gaussian kernel? tent? cubic?
- [ ] Attack: ramp on F_b? или ramp on bow onset position?
- [ ] Nonlinearity: β в tanh³? или отдельный internal nonlinear term?
- [ ] bow() возвращает force vector или модифицирует b in-place?
- [ ] Как Movement модуляция входит в bow position?

---

## Priority 3 — NR() / NRAnisotropic() / NREco()

**Функции:**
- `NetworkIf::NR()`
- `NetworkIf::NRAnisotropic()`
- `NetworkIf::NREco()`

**Что подтвердить:**
- [ ] Predictor: `x = x_n`? `x = 2*x_n - x_nm1`? другой?
- [ ] Точное число итераций (ожидаем 2–4)
- [ ] Jacobian: full dense solve или rank-1 / Sherman-Morrison?
- [ ] Есть ли early stopping по tolerance?
- [ ] Чем NRAnisotropic отличается от NR?
- [ ] Чем NREco отличается: меньше итераций? другая система?

---

## Priority 4 — refreshCoefficients()

**Функция:** `NetworkIf::refreshCoefficients()`

**Что подтвердить:**
- [ ] Mapping BowForce → F_b, α: линейный? log?
- [ ] Mapping SpringDamping → c_s: нормализованный?
- [ ] Как Profile выбирает базовые таблицы M, K
- [ ] Pitch scaling: K_eff = K_0 * s²? Или другая зависимость?
- [ ] Кэшируется ли A целиком или только M, K, C отдельно?

---

## Priority 5 — interpolation3/3Mult()

**Функции:**
- `NetworkIf::interpolation3(float*, int, float)`
- `NetworkIf::interpolation3Mult(float*, int, float, float*)`
- `NetworkIf::extrapolation3(float*, int, float, float)`
- `NetworkIf::extrapolation3Mult(float*, int, float, float, float*)`

**Что подтвердить:**
- [ ] Cubic kernel: Catmull-Rom? B-spline? другой?
- [ ] Число затрагиваемых узлов: 4? 3?
- [ ] Boundary handling: clamp? reflect? wrap? extrapolation?
- [ ] Mult: weight distribution точно обратна read?

---

## Priority 6 — Chaos layer

**Где искать:** вероятно внутри `renderNextBlock()` или `calculateFullSystem*()`

**Что подтвердить:**
- [ ] Тип random process: filtered noise? LCG? Perlin?
- [ ] Точка входа: до solve? Внутри? После?
- [ ] Какие именно коэффициенты перturbed: K? α? p_bow? все?
- [ ] Масштабирование chaos param → σ

---

## Secondary targets

| Функция | Что хотим узнать |
|---|---|
| `changeProfile(Profile, vector<float>&, float&, int, string)` | Что содержат передаваемые arrays? Lookup tables для M/K? |
| `changeDimensions(int)` | N = число узлов прямо? Или масштабный коэффициент? |
| `getROutput(float)` | Отличается ли от `interpolation3()`? Есть ли post-processing? |
| `setParameterWithID(int, float, bool)` | Mapping int ID → param? Bool = immediate update? |
| `setHostSyncerProjectInfo(...)` | Как именно tempo sync реализован для LFO? |

---

## Паттерны для поиска в CFG

При анализе искать:

```
tanh, tanhf          → bow friction law φ(v)
loop count 2–4       → NR iterations
tridiagonal pattern  → 1D K matrix
5-point stencil      → 2D K matrix
cubic weights        → interpolation kernel
random/noise calls   → Chaos layer
SIMD patterns        → matrix-vector multiply
```
