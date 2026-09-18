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

## Desktop GUI (macOS)

The optional GUI uses [Dear ImGui](https://github.com/ocornut/imgui) with
[GLFW](https://www.glfw.org/). CMake downloads pinned sources on the first GUI
configuration, so that step needs internet access:

```sh
cmake -S . -B build -DBUILD_GUI=ON
cmake --build build --parallel 2
./build/portaudio_compressor_gui.app/Contents/MacOS/portaudio_compressor_gui
```

The window opens with audio stopped. Start audio to monitor the default stereo
input through the default stereo output at 44.1 kHz. Adjust threshold, ratio,
knee, and makeup gain while listening; before/after meters and peak histories
show the signal levels. Attack/release and linked-channel detection are still
pending. The GUI is available through CMake; the existing command-line target
remains available.

Start/stop recording saves paired input and compressed WAV files in unique
session folders under `recordings/`, relative to the working directory. Recording
changes briefly pause playback; saving a long recording can take time because
audio is currently buffered in memory. Stopping audio also finalizes recording.

A window/render smoke test runs without opening audio devices or recording:

```sh
./build/portaudio_compressor_gui.app/Contents/MacOS/portaudio_compressor_gui --smoke-test
```

## Offline regression tests

The offline regression suites verify compressor sample values and WAV recording
headers, sample data, and repeated recording sessions. They never open an audio
device; recorder tests use temporary files that are removed afterward:

```sh
ctest --test-dir build --output-on-failure
```


Recording currently retains audio in memory until stopped. Recording calls must
be serialized; the live application stops its audio callback before saving files.
The compressor writes directly into the playback buffer, avoiding a temporary
allocation on each callback.

## ToDo List
- [x] POC Portaudio
- [x] Add WAV file recorder
    - [x] Clean WavRecorder lifecycle, format handling, and buffered appends
    - [ ] Move recording buffers to a background writer for bounded memory use
    - [ ] Add Load Wav file functions
- [x] Start Compressor
- [x]   Threshold
- [x]   Knee
- [ ]   Side chain for multiple input channels
- [ ]   Attack and release enveloppe
- [x]   MakeUpGain
- [x] Dear ImGui interface
    - [x] WAV record start/stop controls
    - [x] Before/after peak meters and history
- [ ] Limiter
- [ ] LUFS
- [ ] EQ (LP, BP, HP filters)
- [ ] 
