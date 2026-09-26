# VoxChain – all-in-one vocal channel strip (VST3)

Input Gain -> High-pass -> 4-band EQ -> Compressor -> De-esser -> Saturation -> Output Gain -> Lookahead Limiter.
Every module (EQ, Compressor, De-esser, Saturation, Limiter) has its own on/off switch.

## Get the Windows plugin (no compiler needed): GitHub Actions
1. Create a new GitHub repository and upload the contents of this folder (keep the `.github` folder).
2. Open the **Actions** tab -> "Build Windows VST3" -> wait ~10 min for the green tick.
3. Open the run, download the artifact **VoxChain-VST3-Windows** and unzip it.
4. Copy the **VoxChain.vst3** folder to `C:\Program Files\Common Files\VST3\`.
5. FL Studio: Options > Manage plugins > **Find installed plugins** (verify). Add it as an effect on a mixer insert.

## Or build locally on Windows
Install Visual Studio 2022 (C++ desktop workload), CMake and Git, then run `build_windows.bat`.

## Signal chain & tips
- **High-pass**: removes rumble below the set frequency. Always active; set low (~20 Hz) to effectively disable it.
- **EQ**: low shelf, two peaking bands, high shelf. The graph shows the live combined response.
- **Compressor**: standard threshold/ratio/knee/attack/release/makeup. GR meter shows current reduction.
- **De-esser**: split-band — only attenuates the sibilant frequency band, leaves the rest of the voice alone. Use LISTEN to solo the band being detected while you dial in the frequency.
- **Saturation**: Drive = amount of tanh-style harmonic saturation, Warmth = adds tube-style even harmonics, Mix = dry/wet blend (use less than 100% for parallel saturation).
- **Limiter**: true lookahead peak limiter, sets the plugin's reported latency (compensated automatically by the host). Turning it off removes the latency too.

## Layout
- `Source/Biquad.h`, `Eq.h`, `Compressor.h`, `DeEsser.h`, `Saturator.h`, `Limiter.h`, `ChannelCore.h` — DSP, no JUCE dependency
- `Source/Plugin*.{h,cpp}` — JUCE wrapper and GUI
- `tests/` — DSP tests (`runcore.cpp` + `test_core.py`) and a headless wrapper/render test

## License note
JUCE is AGPLv3 or commercial. Fine for personal use; if you distribute or sell the plugin, comply with JUCE's licence.
