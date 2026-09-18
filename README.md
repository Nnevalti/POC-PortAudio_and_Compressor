# POC - PortAudio, WAV Recorder and compressor

This repository is a sandbox, I used it first to make a simple in/out audio stream between two audio device. I then added a WAV Recorder that creates a WAV file based on the audio stream passing through my callback function.
Finally I started to develop a peak audio compressor, it's still a work in progress.

## Build (macOS)

The repository includes PortAudio as a submodule. Clone with submodules, then
configure and build with CMake:

```sh
git clone --recurse-submodules <repository-url>
cmake -S . -B build -G "Unix Makefiles"
cmake --build build --parallel 2
```

The current proof of concept uses the default stereo input and output devices.
macOS may ask the host application for microphone access before live capture can
start.

## Offline regression tests

The compressor regression suite uses fixed sample vectors and never opens an
audio device or records a WAV file:

```sh
ctest --test-dir build --output-on-failure
```



## ToDo List
- [x] POC Portaudio
- [x] Add WAV file recorder
    - [ ] Optimize + Clean WavRecorder code
    - [ ] Add Load Wav file functions
- [x] Start Compressor
- [x]   Threshold
- [x]   Knee
- [ ]   Side chain for multiple input channels
- [ ]   Attack and release enveloppe
- [x]   MakeUpGain
- [ ] ImGUI interface
    - [ ] Wav Record -> Start/Stop Recording button
    - [ ] Visualisation for compressor before after
- [ ] Limiter
- [ ] LUFS
- [ ] EQ (LP, BP, HP filters)
- [ ] 
