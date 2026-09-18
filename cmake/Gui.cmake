# GUI dependencies are fetched only when BUILD_GUI is enabled.
include(FetchContent)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL https://codeload.github.com/glfw/glfw/tar.gz/refs/tags/3.4
    URL_HASH SHA256=c038d34200234d071fae9345bc455e4a8f2f544ab60150765d7704e08f3dac01
)
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/tar.gz/refs/tags/v1.91.9b
    URL_HASH SHA256=8e1bbc76c71d74fef2fb85db7e7ca8eba13d6a86623c54992b60162db554ffdb
)
FetchContent_MakeAvailable(glfw imgui)
find_package(OpenGL REQUIRED)
add_library(imgui_gui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_gui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui_gui PUBLIC glfw OpenGL::GL)
add_executable(portaudio_compressor_gui src/gui_main.cpp src/AudioEngine.cpp)
target_link_libraries(portaudio_compressor_gui PRIVATE audio_dsp PortAudio imgui_gui)
if(APPLE)
    set_target_properties(portaudio_compressor_gui PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/GuiInfo.plist.in"
    )
    target_compile_definitions(portaudio_compressor_gui PRIVATE GL_SILENCE_DEPRECATION)
endif()
if(MSVC)
    target_compile_options(portaudio_compressor_gui PRIVATE /W4)
else()
    target_compile_options(portaudio_compressor_gui PRIVATE -Wall -Wextra -Wpedantic)
endif()
