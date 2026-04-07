# Architecture

> Источник: реконструкция по stripped binary — именам классов, методов, JUCE runtime связям.
> Это не исходник, но структура читается достаточно уверенно.

---

## 1. Верхний уровень (standalone wrapper)

```
juce::StandaloneFilterApp
└── juce::StandaloneFilterWindow
    └── juce::StandalonePluginHolder
        └── PluginAudioProcessor
```

`StandalonePluginHolder` держит: аудиодевайс, MIDI, save/load state (`askUserToSaveState`, `askUserToLoadState`).

---

## 2. DSP / State ядро

### PluginAudioProcessor
- Наследник `juce::AudioProcessor`
- Хранит параметры через `juce::AudioProcessorValueTreeState` (APVTS)
- Держит `ParameterManager` — прослойка над APVTS (ranges, IDs, display strings)
- Содержит `BASynth::MPESynth` — главный голосовой движок
- Держит preset/state serialization
- Содержит license state (`isLicensed`, `musicVersion`, trial logic)

### BASynth::MPESynth
Кастомная обёртка над `juce::MPESynthesiser`. Методы:
- `setShouldBypass`, `fadeOutAllVoices`, `customClearVoices`
- `setHostSyncerProjectInfo` — host tempo sync
- `handleMidiEvent`, `handleController`
- `noteAdded`, `notePressureChanged`, `notePitchbendChanged`
- `prepareToPlay`, `renderNextBlock`
- Mono/poly switching: `noteAdded` / legato logic

### BASynth::MPESynthVoice
Один голос. Оркестрирует физическое ядро, **не считает физику сам**.

```
BASynth::MPESynthVoice
├── note lifecycle (startNote / stopNote / clearCurrentNote)
├── gain smoothing / fade in-out
├── RMS / energy threshold (voice death logic)
├── internal temp buffer
└── NetworkIf  ← реальное физическое ядро
```

Вызов: `MPESynthVoice::renderNextBlock()` → `NetworkIf::renderNextBlock()`

---

## 3. Физическое ядро NetworkIf

Это сердце всего. Все DSP-вычисления — здесь.

```
NetworkIf
├── renderNextBlock(AudioBlock<float>&, int, int)
├── setupNote(float, float, float, bool)
├── stopNote()
├── resetStates(bool, bool)
├── refreshCoefficients()
├── calculateFullSystem1DAnisotropic()
├── calculateFullSystem2D()
├── calculateFullSystem2DAnisotropic()
├── calculateFullSystemEco()
├── bow()
├── NR()
├── NRAnisotropic()
├── NREco()
├── getROutput(float)
├── changeDimensions(int)
├── changeProfile(Profile, vector<float>&, float&, int, string)
├── setParameterWithID(int, float, bool)
├── setHostSyncerProjectInfo(...)
├── interpolation1/3(float*, int, float)
├── interpolation1/3Mult(float*, int, float, float*)
├── extrapolation1/3(float*, int, float, float)
└── extrapolation1/3Mult(float*, int, float, float, float*)
```

---

## 4. GUI слои

```
PluginEditor  (тонкая обёртка над AudioProcessorEditor)
└── PluginEditorComponent  (реальный UI контейнер)
    ├── BackgroundImage
    ├── ControlPanel  (принимает APVTS&)
    │   ├── ControlPanelButton  (toggle image / opacity)
    │   ├── ControlPanelButtonLookAndFeel
    │   ├── ThreeCycleButton  (3-позиционный переключатель)
    │   └── VisualiserControlPanel
    ├── Crosshair  (XY macro control, принимает APVTS& + 2 param IDs + ranges)
    │   └── CrosshairSliderLookAndFeel
    ├── LFOslider  (принимает PluginAudioProcessor* + APVTS& + paramID + range)
    │   ├── LFOsliderBackground
    │   ├── LFOTypeBoxLookAndFeel
    │   └── LFOValueKnobLookAndFeel
    ├── Preset UI
    │   ├── PresetWindowMenu  (принимает juce::File& + PluginAudioProcessor&)
    │   ├── PresetManagerView
    │   ├── PresetMenu
    │   ├── PresetManagerLookAndFeel
    │   ├── CustomPresetTooltip
    │   ├── RandomPresetButton
    │   ├── ColorPresetButton
    │   └── FilterByColourPanel
    ├── Tooltip system
    │   ├── MyTooltipWindow
    │   └── TooltipLookAndFeel
    └── Trial/License UI
        ├── WAPNagscreen  (принимает LicenseManager*)
        └── WAPNagscreenLicenseField
```

### Crosshair — XY macro control
Не один фиксированный контрол, а **универсальный XY-макро**, подключаемый к разным парам параметров.

```
Crosshair
├── paramX (string ID)
├── paramY (string ID)
├── rangeX
└── rangeY
```

Вероятные назначения по пресетам:

| Crosshair | X | Y |
|---|---|---|
| Space | SpaceMix | SpaceSize |
| Filter | FilterMIN | FilterMAX |
| LFO | lfoMin | lfoMax |
| Motion | MotionFREQ | MotionDEPTH |

### Visualiser
- `Visualiser::setProfileToSet`, `Visualiser::applyProfile`
- Знает о Profile-ах модели — не просто декоративный элемент

---

## 5. Preset система

Формат: `.baby` — JUCE ValueTree XML

```
Application Support/BABY Audio/presets/
├── Factory Presets/
│   └── *.baby
├── user packs/
└── imported packs/
```

Структура файла:
```xml
<STATE>
  <PARAM id="springDamping" value="0.43"/>
  <PARAM id="bowForce" value="0.55"/>
  ...
</STATE>
```

Preset — это **полный snapshot параметров физической модели**. Нет ссылок на audio assets.

Версионность: есть legacy preset detection (`Old preset!`, `Doesn't have LFO modes!`) и конвертация.

---

## 6. License / Trial

- Trial: 5 секунд тишины каждые 60 секунд
- Serial key input через `WAPNagscreenLicenseField`
- `LicenseManager` держит состояние авторизации

---

## Связи между слоями

```
PluginAudioProcessor
    ↕ APVTS (параметры)
BASynth::MPESynth
    ↕ per-note
BASynth::MPESynthVoice
    ↕ вызов renderNextBlock
NetworkIf  ← вся физика здесь
```

UI читает/пишет параметры через APVTS. DSP читает параметры через `setParameterWithID` или напрямую из APVTS.
