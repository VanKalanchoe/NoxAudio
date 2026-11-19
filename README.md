# NoxAudio

Standalone C++20 3D spatial audio, DSP effects, MetaSounds-style audio node-graph, and acoustic baking engine extracted from the Nox engine.

## Features

- **Lock-Free Audio Engine (`AudioEngine`)**: Dedicated mixer & streaming threads, voice concurrency & virtualization, Doppler shift, and miniaudio device I/O (WAV, FLAC, MP3, Ogg Vorbis).
- **Bus Mixer & Snapshots (`AudioMixer`)**: Hierarchical buses, pre/post-fader sends, sidechain ducking, YAML serialization (`.nmixer`), and smooth snapshot blending.
- **14 DSP Effects (`AudioEffects`, `AudioDSP`)**: 4-band EQ, Filter, Compressor, Look-ahead Limiter, Gate, Stereo Delay, FDN Reverb, Partitioned FFT Convolution Reverb, Chorus, Flanger, Phaser, 2x-Oversampled Distortion, Bitcrusher, and Steam Audio Acoustic Reverb.
- **Audio Node Graph (`AudioGraph`)**: Composable MetaSounds-style DSP graphs with topological compilation, sub-graph inlining, live parameter/trigger modulation, and built-in synthesis/sampling/DSP nodes.
- **3D Spatial Audio & Acoustic Baking (`SpatialAudio`, `AcousticBake`, `AcousticData`)**: Powered by Steam Audio (HRTF binaural rendering, volumetric occlusion & transmission, real-time reflections, baked probe volumes, and dynamic pathing) with raw `AcousticMesh` geometry input and zero engine coupling.
- **Pluggable Engine Hooks (`AudioHooks`, `AudioLog`)**: Custom logging, virtual file system reader, profiler zones, parallel-for dispatch, world-unit scaling, and asset registries.

## Dependencies

- C++20 compiler (Visual Studio 2026 + ClangCL or MSVC on Windows x64)
- CMake 3.25+
- `miniaudio` (fetched into `vendors/miniaudio`)
- `Steam Audio SDK v4.8.1` (fetched into `vendors/steamaudio`)
- `glm` and `yaml-cpp` (installed via `vcpkg`)

## Quick Start (Windows)

```bat
scripts\install_dependencies.bat
scripts\setup_build.bat
```

Open `build\NoxAudio.slnx` in Visual Studio 2026 or JetBrains Rider, or build from the command line:

```bat
cmake --build build --config Release --parallel
build\Release\NoxAudioTest.exe
```

Pass `--msvc` to `scripts\setup_build.bat` to use the MSVC toolset instead of ClangCL.

## CMake Integration

```cmake
add_subdirectory(NoxAudio)
target_link_libraries(MyApp PRIVATE NoxAudio::Core)
```

Aliases `NoxAudio::Core` and `NoxAudio::NoxAudio` are provided. Set `-DNOXAUDIO_BUILD_TESTS=OFF` when embedding into a larger project.

## License

MIT License — see [LICENSE](LICENSE).
