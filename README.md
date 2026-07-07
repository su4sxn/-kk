# AtmoGrain

Simpele granular VST3/AU voor Apple Silicon. Drop een sample erin, hij hakt 'm
in korte grains met random pitch/positie/pan en gooit er een feedback-smear
overheen voor dat atmosferische effect.

Knoppen: Size, Density, Position, Jitter, PitchSpread, Width, Smear, Mix.

## Bouwen (3 commands, in Terminal)

```
xcode-select --install          # eenmalig, gratis, geen App Store nodig
cd AtmoGrain
cmake -B build -G Xcode
cmake --build build --config Release
```

Eerste build duurt ~10-15 min (JUCE wordt automatisch gedownload + gecompiled).
Elke build daarna is snel.

## Waar hij terechtkomt

CMake kopieert 'm automatisch naar:
- `~/Library/Audio/Plug-Ins/VST3/AtmoGrain.vst3`
- `~/Library/Audio/Plug-Ins/Components/AtmoGrain.component` (AU)

FL Studio moet je plugins opnieuw laten scannen (Options > Manage Plugins > Find plugins).

## Nodig

- Xcode Command Line Tools (via `xcode-select --install`, geen volle Xcode app nodig)
- CMake: `brew install cmake` (als je geen Homebrew hebt: `curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh | bash`)

Dat is het — geen handmatige JUCE download, geen losse SDK's.
