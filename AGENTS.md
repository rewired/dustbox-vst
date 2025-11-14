# AGENTS.md — DUSTBOX

## Project

**Name:** DUSTBOX (working title)
**Type:** Audio effect plugin (VST3 now, AU later) built with **JUCE 8.0.10** for **Windows/macOS**

## Roles & Ownership

* **Product Owner (you)** — Vision, UX priorities, final acceptance, SVG skin later.
* **Assistant (Software Architect & DSP Lead)** — Architecture, DSP design, parameter model, QA criteria.
* **Codex (C++/JUCE/VST3 Engineer)** — Production-quality C++ implementation, project scaffolding, DSP module stubs, APVTS, editor wiring, realtime safety.

## Vision (What It Does)

Creative **Lo-Fi coloration and movement**:

* **AGE (Tape):** wow/flutter, tone roll-off, hiss/noise
* **DIRT (Degrade):** saturation → bit-depth reduction → sample-rate reduction
* **PUMP (Groove):** tempo-synced ducking (fake sidechain)
* **Global:** wet/dry mix, output gain, hard bypass

Outcome for users: **instant dusty tape feel, crunchy downgrade, musical pump** with a few knobs.

---

## Hard Constraints (Do/Don’t)

* **Format:** VST3 (AU later) | **Framework:** JUCE
* **Platforms:** Windows & macOS
* **Realtime safety:** **No allocations, no file I/O, no locks** in `processBlock`
* **CPU target:** **< 1%** on modern CPUs @ 44.1/48 kHz stereo
* **Latency:** **Zero-latency** (no lookahead/compensation in MVP)
* **Parameters:** Automatable via **APVTS**, stable string IDs
* **Tempo:** Host tempo via `AudioPlayHead`, fallback **120 BPM**
* **UI (MVP):** Generic/grouped JUCE controls (no SVG yet)

---

## Architecture (MVP)

* **Processor** (`DustboxProcessor`): owns modules, smoothing, dry/wet, output, bypass ramp, tempo helper.
* **Modules** (independent classes with `prepare`, `reset`, `processBlock`):

  1. **TapeModule** — modulated-delay **wow/flutter**, LPF tone, hiss (switchable routing).
  2. **DirtModule** — **saturation → quantization → sample-hold downsample**.
  3. **ReverbModule** — ambience block with pre-delay, decay, damping, and mix controls ([ADR 0006](docs/adr/0006-reverb-module.md)).
  4. **PumpModule** — gain envelope with **fast decay + eased release**, host-synced.
* **Parameter Smoothing:** `juce::SmoothedValue` (10–30 ms; log mapping for cutoff; equal-power mix).
* **Noise Routing (user-selectable):**
  `WetPrePump` (ducked) / `WetPostPump` (not ducked) / `PostMix` (global hiss).
* **Processing order:** **Tape → Dirt → Reverb → Pump → Mix → Output**
  Parallel dry path; equal-power crossfade; output gain; click-free bypass ramp.

---

## Parameters (IDs • ranges • defaults)

**Tape / AGE**

* `tapeWowDepth` (0.00–1.00, **0.15**)
* `tapeWowRateHz` (0.10–5.00 Hz, **0.60**)
* `tapeFlutterDepth` (0.00–1.00, **0.08**)
* `tapeToneLowpassHz` (2000–20000 Hz, **11000**) — **log control**
* `tapeNoiseLevelDb` (-60 – -20 dB, **-48**)
* `tapeNoiseRoute` enum int { **0**=WetPrePump, 1=WetPostPump, 2=PostMix }

**Dirt**

* `dirtSaturationAmt` (0.00–1.00, **0.35**) — **perceptual drive curve**
* `dirtBitDepthBits` int (4–24, **12**) — **≥24 bypass quantize**
* `dirtSampleRateDiv` int (1–16, **2**) — **1 bypass downsample**

**Pump**

* `pumpAmount` (0.00–1.00, **0.35**)
* `pumpSyncNote` enum int { 0=¼, **1=⅛**, 2=1⁄16 }
* `pumpPhase` (0.00–1.00, **0.00**) — cycle offset

**Global**

* `mixWet` (0.00–1.00, **0.50**) — **equal-power** mix
* `outputGainDb` (-24 – +24 dB, **0**)
* `hardBypass` bool {0/1, **0**} — **1–2 ms** internal ramp on toggle

---

## Implementation Decisions (Locked)

* **Tape wobble:** chorus-style **modulated delay** (no time-stretching).
* **Pump shape:** **fast decay + eased release** (sidechain feel).
* **Quantizer bypass:** **skip work** when `bits ≥ 24`.
* **Downsample:** sample-and-hold when `div > 1`; no anti-alias in MVP.
* **Mix law:** **equal-power**.
* **Bypass:** click-safe **mini-ramp**.
* **IDs:** flat, host-friendly (with feature prefix in names).

---

## Project Layout (for Codex)

```
Source/
├─ Dsp/
│  ├─ modules/
│  │  ├─ TapeModule.h/.cpp
│  │  ├─ DirtModule.h/.cpp
│  │  └─ PumpModule.h/.cpp
│  ├─ utils/
│  │  ├─ ParameterSmoother.h
│  │  ├─ NoiseGenerator.h
│  │  ├─ DenormalGuard.h
│  │  └─ MathHelpers.h
│  └─ routing/ProcessingGraph.h        // linear for now
├─ Parameters/
│  ├─ ParameterIDs.h
│  ├─ ParameterSpec.h
│  └─ ParameterLayout.h                // builds APVTS layout
├─ Plugin/
│  ├─ DustboxProcessor.h/.cpp
│  ├─ DustboxEditor.h/.cpp             // grouped generic controls
│  └─ HostTempo.h
├─ Ui/GenericControls.h/.cpp
└─ Core/Version.h, BuildConfig.h
```

---

## Lifecycle (how it runs)

* **Construction:** create APVTS (all params), instantiate modules & smoothers.
* **prepareToPlay:** store `sr/blockSize`; allocate persistent dry buffer; call `module.prepare`; `reset`.
* **processBlock:**

  1. optional denormal guard
  2. **bypass ramp** (copy-through if active)
  3. fetch params (atomic) → advance **smoothers**
  4. query **host tempo** (fallback 120 BPM) → compute samples/cycle & phase offset → pass to Pump
  5. copy input → dryBuffer
  6. **Tape → Dirt → Reverb → Pump** on wet buffer
  7. **equal-power** mix dry/wet → `outputGainDb`
* **releaseResources:** free scratch if needed; keep state.

---

## First Delivery — Definition of Done (Scaffold)

1. **Builds** as VST3 on Win/macOS with JUCE; compiles cleanly.
2. **Folders & files** as above; no “god files”.
3. **APVTS** layout contains **all parameters**, correct ranges, defaults, value skews (log for LPF).
4. **Processor flow** implemented with **TODO** blocks for DSP math; no runtime allocations in audio thread.
5. **Modules** have `prepare/reset/processBlock` and internal state stubs (delay lines, LFO/phase, counters).
6. **Smoothing** implemented where specified; **equal-power** wet/dry; **bypass ramp**.
7. **Tempo helper** implemented; **120 BPM** fallback.
8. **Editor (MVP)** shows grouped controls incl. **Noise Route** selector; parameters automate in host.
9. **Code quality:** clear comments at file tops (responsibility, assumptions, TODOs).

---

## Out of Scope (MVP)

* SVG skin / final UI
* Oversampling, anti-alias filters, convolution, speaker sims
* Preset management beyond host defaults
* AU build (planned later)

---

## Acceptance Checklist (PO)

* [ ] Parameters visible & automatable in DAW; ranges & defaults feel musical.
* [ ] Bypass click-free; CPU below target; no xruns.
* [ ] Mix is equal-power; output gain trims correctly.
* [ ] Pump syncs to host; 120 BPM fallback works when tempo missing.
* [ ] Noise routing switch behaves as specified.
* [ ] Codebase modular, readable, and ready for DSP fill-in.

---

## Open Items (tracked)

* Envelope **shape constants** (fine-tune decay/release curves after listening).
* Exact **smoothing times** per control may be nudged by ear.
* Final **default values** can be tweaked during first listening pass.
