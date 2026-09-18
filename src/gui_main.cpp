#include "AudioEngine.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr std::size_t historySize = 240;
constexpr float meterFloor = -60.0f;

float decibels(float peak) {
    return peak > 0.001f ? 20.0f * std::log10(peak) : meterFloor;
}

void configureStyle() {
    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(26, 22);
    style.FramePadding = ImVec2(12, 8);
    style.ItemSpacing = ImVec2(12, 12);
    style.WindowRounding = 0;
    style.FrameRounding = 5;
    style.GrabRounding = 5;
    style.ScrollbarRounding = 5;
    auto* colors = style.Colors;
    colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.068f, 0.085f, 1);
    colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.15f, 0.18f, 1);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.21f, 0.25f, 1);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.20f, 0.28f, 0.32f, 1);
    colors[ImGuiCol_Button] = ImVec4(0.12f, 0.34f, 0.35f, 1);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.16f, 0.45f, 0.44f, 1);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.12f, 0.52f, 0.48f, 1);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.32f, 0.82f, 0.72f, 1);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.45f, 0.95f, 0.85f, 1);
    colors[ImGuiCol_PlotLines] = ImVec4(0.32f, 0.82f, 0.72f, 1);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.32f, 0.82f, 0.72f, 1);
    colors[ImGuiCol_Text] = ImVec4(0.89f, 0.93f, 0.95f, 1);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.57f, 0.65f, 0.70f, 1);
}

void levelMeter(const char* label, float peak) {
    const float db = decibels(peak);
    char overlay[48];
    if (peak <= 0) {
        std::snprintf(overlay, sizeof(overlay), "Silent");
    } else {
        std::snprintf(overlay, sizeof(overlay), "%.1f dBFS%s", db,
                      peak >= 1 ? "  /  CLIP" : "");
    }
    ImGui::TextUnformatted(label);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
        peak >= 1 ? ImVec4(0.95f, 0.35f, 0.30f, 1) : ImVec4(0.32f, 0.82f, 0.72f, 1));
    ImGui::ProgressBar(std::clamp((db - meterFloor) / -meterFloor, 0.0f, 1.0f),
                       ImVec2(-1, 25), overlay);
    ImGui::PopStyleColor();
}

void glfwError(int error, const char* description) {
    std::fprintf(stderr, "GLFW error %d: %s\n", error, description);
}
} // namespace

int main(int argc, char** argv) {
    bool smokeTest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--smoke-test") == 0) {
            smokeTest = true;
        } else {
            std::fprintf(stderr, "Usage: %s [--smoke-test]\n", argv[0]);
            return 1;
        }
    }

    glfwSetErrorCallback(glfwError);
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    GLFWwindow* window = glfwCreateWindow(960, 780, "Compressor / Audio Lab", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwSetWindowSizeLimits(window, 640, 640, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    configureStyle();
    if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 150")) {
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    AudioEngine engine;
    float threshold = -20, ratio = 5, knee = 0, makeup = 3;
    engine.setParameters(threshold, ratio, knee, makeup);
    std::array<float, historySize> inputHistory{}, outputHistory{};
    inputHistory.fill(meterFloor);
    outputHistory.fill(meterFloor);
    int historyOffset = 0;
    double lastSample = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(window) && (!smokeTest || frames < 5)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        const auto* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Compressor", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings);
        ImGui::SetWindowFontScale(1.65f);
        ImGui::TextUnformatted("Compressor");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::TextDisabled("AUDIO LAB  /  Stereo dynamics");
        ImGui::Spacing();

        ImGui::BeginDisabled(smokeTest);
        if (ImGui::Button(engine.isRunning() ? "Stop audio" : "Start audio", ImVec2(140, 38))) {
            if (engine.isRunning()) engine.stopAudio();
            else engine.startAudio();
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(!engine.isRunning());
        if (ImGui::Button(engine.isRecording() ? "Stop & save recording" : "Record WAV", ImVec2(190, 38))) {
            if (engine.isRecording()) engine.stopRecording();
            else engine.startRecording();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(engine.isRecording() ? ImVec4(1, 0.45f, 0.4f, 1) :
                           ImVec4(0.50f, 0.80f, 0.72f, 1),
                           "%s", engine.isRecording() ? "RECORDING" :
                           engine.isRunning() ? "AUDIO ON" : "AUDIO OFF");
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", engine.status().c_str());
        ImGui::TextDisabled("Default devices / stereo / 44.1 kHz");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::BeginTable("Processing", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("SHAPE YOUR SOUND");
            ImGui::Spacing();
            ImGui::PushItemWidth(-16);
            bool changed = false;
            ImGui::TextUnformatted("Threshold");
            changed |= ImGui::SliderFloat("##threshold", &threshold, -60, 0, "%.1f dB", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TextUnformatted("Ratio");
            changed |= ImGui::SliderFloat("##ratio", &ratio, 1, 20, "%.1f : 1", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TextUnformatted("Knee");
            changed |= ImGui::SliderFloat("##knee", &knee, 0, 24, "%.1f dB", ImGuiSliderFlags_AlwaysClamp);
            ImGui::TextUnformatted("Makeup gain");
            changed |= ImGui::SliderFloat("##makeup", &makeup, 0, 24, "%.1f dB", ImGuiSliderFlags_AlwaysClamp);
            if (changed) engine.setParameters(threshold, ratio, knee, makeup);
            ImGui::PopItemWidth();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("LIVE LEVELS");
            ImGui::Spacing();
            const float input = engine.isRunning() ? engine.inputPeak() : 0;
            const float output = engine.isRunning() ? engine.outputPeak() : 0;
            levelMeter("Input / before compression", input);
            levelMeter("Output / after compression", output);
            ImGui::TextDisabled("Peak level / -60 to 0 dBFS");
            ImGui::Spacing();
            ImGui::TextWrapped("Attack / release and stereo channel linking are upcoming. Compression currently follows each sample immediately.");
            ImGui::EndTable();
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextUnformatted("PEAK HISTORY");
        ImGui::SameLine();
        ImGui::TextDisabled("Last 8 seconds / dBFS");
        const double now = glfwGetTime();
        if (now - lastSample >= 1.0 / 30.0) {
            inputHistory[historyOffset] = decibels(engine.isRunning() ? engine.inputPeak() : 0);
            outputHistory[historyOffset] = decibels(engine.isRunning() ? engine.outputPeak() : 0);
            historyOffset = (historyOffset + 1) % static_cast<int>(historySize);
            lastSample = now;
        }
        ImGui::PlotLines("##inputHistory", inputHistory.data(), static_cast<int>(historySize),
            historyOffset, "Input / before", meterFloor, 6, ImVec2(-1, 70));
        ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.55f, 0.67f, 1, 1));
        ImGui::PlotLines("##outputHistory", outputHistory.data(), static_cast<int>(historySize),
            historyOffset, "Output / after", meterFloor, 6, ImVec2(-1, 70));
        ImGui::PopStyleColor();
        ImGui::TextDisabled("Recent peaks, not a waveform. Values below -60 dBFS are hidden.");
        ImGui::TextWrapped("Recording buffers in memory. Starting or saving a recording briefly pauses audio; stop audio also saves the recording.");
        ImGui::End();

        ImGui::Render();
        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.055f, 0.068f, 0.085f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        ++frames;
    }

    engine.stopAudio();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
