cmake_minimum_required(VERSION 3.21)

project(PortAudioCompressorPOC LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The PortAudio submodule is part of this repository so a clean checkout has
# everything it needs to build the application.
# PortAudio's bundled CMake file predates CMake 3.5 compatibility removal.
# Keep the compatibility setting scoped to configuring this vendored version.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(PA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(PA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(portaudio EXCLUDE_FROM_ALL)

add_library(audio_dsp
    src/Compressor.cpp
    src/WavRecorder.cpp
)
target_include_directories(audio_dsp PUBLIC include)

add_executable(portaudio_compressor
    src/main.cpp
    src/StreamHandler.cpp
)

target_include_directories(portaudio_compressor PRIVATE include)
target_link_libraries(portaudio_compressor PRIVATE audio_dsp PortAudio)

enable_testing()
add_executable(compressor_regression_tests tests/CompressorTests.cpp)
target_link_libraries(compressor_regression_tests PRIVATE audio_dsp)
add_test(NAME compressor_regression_tests COMMAND compressor_regression_tests)

if(MSVC)
    target_compile_options(audio_dsp PRIVATE /W4)
    target_compile_options(portaudio_compressor PRIVATE /W4)
    target_compile_options(compressor_regression_tests PRIVATE /W4)
else()
    target_compile_options(audio_dsp PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(portaudio_compressor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(compressor_regression_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
