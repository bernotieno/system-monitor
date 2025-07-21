#include "header.h"
#include <SDL.h>

/*
NOTE : You are free to change the code as you wish, the main objective is to make the
       application work and pass the audit.

       It will be provided the main function with the following functions :

       - `void systemWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the system window on your screen
       - `void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the memory and processes window on your screen
       - `void networkWindow(const char *id, ImVec2 size, ImVec2 position)`
            This function will draw the network window on your screen
*/

// About Desktop OpenGL function loaders:
//  Modern desktop OpenGL doesn't have a standard portable header file to load OpenGL function pointers.
//  Helper libraries are often used for this purpose! Here we are supporting a few common ones (gl3w, glew, glad).
//  You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h> // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h> // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h> // Initialize with gladLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
#include <glad/gl.h> // Initialize with gladLoadGL(...) or gladLoaderLoadGL()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
#define GLFW_INCLUDE_NONE      // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/Binding.h> // Initialize with glbinding::Binding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
#define GLFW_INCLUDE_NONE        // GLFW including OpenGL headers causes ambiguity or multiple definition errors.
#include <glbinding/glbinding.h> // Initialize with glbinding::initialize()
#include <glbinding/gl/gl.h>
using namespace gl;
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

// systemWindow, display information for the system monitorization
void systemWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Static variables for throttling updates
    static float lastStaticUpdate = 0;
    static float lastTaskUpdate = 0;
    static string cachedOsName = "";
    static string cachedUsername = "";
    static string cachedHostname = "";
    static string cachedCPUinfo = "";
    static map<char, int> cachedProcessStates;
    static map<string, int> cachedTopStyleCounts;

    float currentTime = ImGui::GetTime();

    // Update static system info only once (or very rarely)
    if (cachedOsName.empty() || currentTime - lastStaticUpdate > 60.0f) {
        cachedOsName = getOsName();
        cachedUsername = getUsername();
        cachedHostname = getHostname();
        cachedCPUinfo = CPUinfo();
        lastStaticUpdate = currentTime;
    }

    // Update task counts every 3 seconds (matches top's default refresh rate)
    if (currentTime - lastTaskUpdate > 3.0f) {
        cachedProcessStates = getProcessCountByState();
        cachedTopStyleCounts = getTopStyleProcessCounts();
        lastTaskUpdate = currentTime;
    }

    // System Information Section
    if (ImGui::CollapsingHeader("System Information", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("OS: %s", cachedOsName.c_str());
        ImGui::Text("Username: %s", cachedUsername.c_str());
        ImGui::Text("Hostname: %s", cachedHostname.c_str());
        ImGui::Text("CPU: %s", cachedCPUinfo.c_str());
        ImGui::Separator();

        // Process count by state (matching 'top' command format)
        ImGui::Text("Tasks: %d total, %d running, %d sleeping, %d stopped, %d zombie",
                   cachedTopStyleCounts["total"],
                   cachedTopStyleCounts["running"],
                   cachedTopStyleCounts["sleeping"],
                   cachedTopStyleCounts["stopped"],
                   cachedTopStyleCounts["zombie"]);

        // Detailed breakdown (for debugging/additional info)
        if (ImGui::CollapsingHeader("Detailed Process States")) {
            ImGui::Indent();
            ImGui::Text("Running (R): %d", cachedProcessStates['R']);
            ImGui::Text("Sleeping (S): %d", cachedProcessStates['S']);
            ImGui::Text("Idle (I): %d", cachedProcessStates['I']);
            ImGui::Text("Disk Sleep (D): %d", cachedProcessStates['D']);
            ImGui::Text("Zombie (Z): %d", cachedProcessStates['Z']);
            ImGui::Text("Stopped (T): %d", cachedProcessStates['T']);
            ImGui::Text("Tracing stop (t): %d", cachedProcessStates['t']);
            ImGui::Text("Dead (X): %d", cachedProcessStates['X']);
            ImGui::Unindent();
        }
    }

    // System Monitoring Tabs
    if (ImGui::BeginTabBar("SystemMonitoringTabs")) {
        // CPU Tab
        if (ImGui::BeginTabItem("CPU")) {
            static vector<float> cpuHistory;
            static bool animate = true;
            static float fps = 60.0f;
            static float yScale = 100.0f;
            static double cachedCPU = 0.0;
            static float lastCPUUpdate = 0;

            float currentTime = ImGui::GetTime();

            // Update CPU data every 3 seconds (matches top's default refresh rate)
            if (currentTime - lastCPUUpdate > 3.0f) {
                cachedCPU = getCPUUsage();
                lastCPUUpdate = currentTime;
            }

            // Only update if animation is enabled
            if (animate) {
                cpuHistory.push_back((float)cachedCPU);

                // Keep only last 100 values
                if (cpuHistory.size() > 100) {
                    cpuHistory.erase(cpuHistory.begin());
                }
            }

            ImGui::Checkbox("Animate", &animate);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y-Scale", &yScale, 50.0f, 200.0f);

            // CPU percentage overlay
            ImGui::Text("CPU Usage: %.1f%%", cachedCPU);

            // Plot CPU usage graph
            if (!cpuHistory.empty()) {
                ImGui::PlotLines("CPU %", cpuHistory.data(), cpuHistory.size(),
                               0, nullptr, 0.0f, yScale, ImVec2(0, 80));
            }

            ImGui::EndTabItem();
        }

        // Thermal Tab
        if (ImGui::BeginTabItem("Thermal")) {
            static vector<vector<float>> thermalHistory;
            static bool animate = true;
            static float fps = 60.0f;
            static float yScale = 100.0f;
            static vector<ThermalInfo> cachedThermalInfo;
            static float lastThermalUpdate = 0;

            float currentTime = ImGui::GetTime();

            // Update thermal data every 3 seconds (matches top's default interval)
            if (currentTime - lastThermalUpdate > 3.0f) {
                cachedThermalInfo = getThermalInfo();
                lastThermalUpdate = currentTime;
            }

            // Initialize thermal history if needed
            if (thermalHistory.size() != cachedThermalInfo.size()) {
                thermalHistory.resize(cachedThermalInfo.size());
            }

            // Only update if animation is enabled
            if (animate) {
                // Update thermal history
                for (size_t i = 0; i < cachedThermalInfo.size(); i++) {
                    thermalHistory[i].push_back((float)cachedThermalInfo[i].temperature);

                    // Keep only last 100 values
                    if (thermalHistory[i].size() > 100) {
                        thermalHistory[i].erase(thermalHistory[i].begin());
                    }
                }
            }

            ImGui::Checkbox("Animate", &animate);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y-Scale", &yScale, 50.0f, 200.0f);

            if (cachedThermalInfo.empty()) {
                ImGui::Text("No thermal sensors found");
            } else {
                for (size_t i = 0; i < cachedThermalInfo.size(); i++) {
                    const auto& thermal = cachedThermalInfo[i];
                    ImGui::Text("%s: %.1f°C", thermal.label.c_str(), thermal.temperature);

                    // Temperature overlay with color coding
                    ImVec4 color = ImVec4(0, 1, 0, 1); // Green
                    if (thermal.temperature > 70) color = ImVec4(1, 1, 0, 1); // Yellow
                    if (thermal.temperature > 85) color = ImVec4(1, 0, 0, 1); // Red

                    ImGui::SameLine();
                    ImGui::TextColored(color, "[%.1f°C]", thermal.temperature);

                    // Plot thermal history
                    if (!thermalHistory[i].empty()) {
                        ImGui::PlotLines(("Temp " + thermal.label).c_str(),
                                       thermalHistory[i].data(),
                                       thermalHistory[i].size(),
                                       0, nullptr, 0.0f, yScale, ImVec2(0, 80));
                    }
                }
            }

            ImGui::EndTabItem();
        }

        // Fan Tab
        if (ImGui::BeginTabItem("Fan")) {
            static vector<vector<float>> fanHistory;
            static bool animate = true;
            static float fps = 60.0f;
            static float yScale = 5000.0f; // Higher scale for RPM
            static vector<FanInfo> cachedFanInfo;
            static float lastFanUpdate = 0;

            float currentTime = ImGui::GetTime();

            // Update fan data every 3 seconds (matches top's default interval)
            if (currentTime - lastFanUpdate > 3.0f) {
                cachedFanInfo = getFanInfo();
                lastFanUpdate = currentTime;
            }

            // Initialize fan history if needed
            if (fanHistory.size() != cachedFanInfo.size()) {
                fanHistory.resize(cachedFanInfo.size());
            }

            // Only update if animation is enabled
            if (animate) {
                // Update fan history
                for (size_t i = 0; i < cachedFanInfo.size(); i++) {
                    fanHistory[i].push_back((float)cachedFanInfo[i].speed);

                    // Keep only last 100 values
                    if (fanHistory[i].size() > 100) {
                        fanHistory[i].erase(fanHistory[i].begin());
                    }
                }
            }

            ImGui::Checkbox("Animate", &animate);
            ImGui::SameLine();
            ImGui::SliderFloat("FPS", &fps, 1.0f, 120.0f);
            ImGui::SliderFloat("Y-Scale", &yScale, 1000.0f, 10000.0f);

            if (cachedFanInfo.empty()) {
                ImGui::Text("No fan sensors found");
            } else {
                for (size_t i = 0; i < cachedFanInfo.size(); i++) {
                    const auto& fan = cachedFanInfo[i];
                    ImGui::Text("%s: %d RPM", fan.label.c_str(), fan.speed);

                    // Fan status indicator
                    ImVec4 color = fan.speed > 0 ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1);
                    ImGui::SameLine();
                    ImGui::TextColored(color, "[%s]", fan.speed > 0 ? "ACTIVE" : "STOPPED");

                    // Plot fan history
                    if (!fanHistory[i].empty()) {
                        ImGui::PlotLines(("RPM " + fan.label).c_str(),
                                       fanHistory[i].data(),
                                       fanHistory[i].size(),
                                       0, nullptr, 0.0f, yScale, ImVec2(0, 80));
                    }
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// memoryProcessesWindow, display information for the memory and processes information
void memoryProcessesWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Memory Usage Section
    if (ImGui::CollapsingHeader("Memory Usage", ImGuiTreeNodeFlags_DefaultOpen)) {
        MemoryInfo memInfo = getMemoryInfo();

        // Helper function to format bytes with appropriate units
        auto formatBytes = [](unsigned long bytes) -> string {
            const char* units[] = {"B", "KB", "MB", "GB", "TB"};
            int unit = 0;
            double size = (double)bytes;

            while (size >= 1024.0 && unit < 4) {
                size /= 1024.0;
                unit++;
            }

            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.1f %s", size, units[unit]);
            return string(buffer);
        };

        // RAM Usage
        float ramUsage = (float)memInfo.usedRAM / memInfo.totalRAM;
        ImGui::Text("RAM: %s / %s",
                   formatBytes(memInfo.usedRAM).c_str(),
                   formatBytes(memInfo.totalRAM).c_str());
        ImGui::ProgressBar(ramUsage, ImVec2(-1, 0), "");
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("%.1f%%", ramUsage * 100.0f);

        // SWAP Usage
        if (memInfo.totalSwap > 0) {
            float swapUsage = (float)memInfo.usedSwap / memInfo.totalSwap;
            ImGui::Text("SWAP: %s / %s",
                       formatBytes(memInfo.usedSwap).c_str(),
                       formatBytes(memInfo.totalSwap).c_str());
            ImGui::ProgressBar(swapUsage, ImVec2(-1, 0), "");
            ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("%.1f%%", swapUsage * 100.0f);
        }

        // Disk Usage (Used/Size format)
        DiskInfo diskInfo = getDiskInfo();
        float diskUsage = (float)diskInfo.usedDisk / diskInfo.totalDisk;
        ImGui::Text("Disk: %s / %s",
                   formatBytes(diskInfo.usedDisk).c_str(),
                   formatBytes(diskInfo.totalDisk).c_str());
        ImGui::ProgressBar(diskUsage, ImVec2(-1, 0), "");
        ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
        ImGui::Text("%.1f%%", diskUsage * 100.0f);
    }

    // Process Monitor Section
    if (ImGui::CollapsingHeader("Process Monitor", ImGuiTreeNodeFlags_DefaultOpen)) {
        static char filter[256] = "";
        static vector<int> selectedProcesses;

        ImGui::Text("Filter:");
        ImGui::SameLine();
        ImGui::InputText("##filter", filter, sizeof(filter));

        // Get process list
        static vector<Proc> processes;
        static float lastUpdate = 0;
        float currentTime = ImGui::GetTime();

        // Update process list every 3 seconds (matches top's default refresh rate)
        if (currentTime - lastUpdate > 3.0f) {
            processes = getProcessList();
            lastUpdate = currentTime;
        }

        // Filter processes
        vector<Proc> filteredProcesses;
        string filterStr = string(filter);
        transform(filterStr.begin(), filterStr.end(), filterStr.begin(), ::tolower);

        for (const auto& proc : processes) {
            if (filterStr.empty()) {
                filteredProcesses.push_back(proc);
            } else {
                string procName = proc.name;
                transform(procName.begin(), procName.end(), procName.begin(), ::tolower);
                if (procName.find(filterStr) != string::npos) {
                    filteredProcesses.push_back(proc);
                }
            }
        }

        // Process table
        if (ImGui::BeginTable("ProcessTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                             ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("State", ImGuiTableColumnFlags_WidthFixed, 60.0f);
            ImGui::TableSetupColumn("CPU%", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("MEM%", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableHeadersRow();

            // Display processes (limit to first 100 for performance)
            int displayCount = min(100, (int)filteredProcesses.size());
            for (int i = 0; i < displayCount; i++) {
                const auto& proc = filteredProcesses[i];

                ImGui::TableNextRow();

                // Multi-row selection
                ImGui::TableSetColumnIndex(0);
                bool isSelected = find(selectedProcesses.begin(), selectedProcesses.end(), proc.pid)
                                != selectedProcesses.end();

                if (ImGui::Selectable(to_string(proc.pid).c_str(), isSelected,
                                    ImGuiSelectableFlags_SpanAllColumns)) {
                    if (ImGui::GetIO().KeyCtrl) {
                        // Multi-select with Ctrl
                        if (isSelected) {
                            selectedProcesses.erase(
                                remove(selectedProcesses.begin(), selectedProcesses.end(), proc.pid),
                                selectedProcesses.end());
                        } else {
                            selectedProcesses.push_back(proc.pid);
                        }
                    } else {
                        // Single select
                        selectedProcesses.clear();
                        selectedProcesses.push_back(proc.pid);
                    }
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%s", proc.name.c_str());

                ImGui::TableSetColumnIndex(2);
                // Read process state fresh every time (like top does)
                char currentState = getCurrentProcessState(proc.pid);
                ImGui::Text("%c", currentState);

                ImGui::TableSetColumnIndex(3);
                double cpuUsage = getProcessCPUUsage(proc);
                ImGui::Text("%.1f", cpuUsage);

                ImGui::TableSetColumnIndex(4);
                double memUsage = getProcessMemoryUsage(proc);
                ImGui::Text("%.1f", memUsage);
            }

            ImGui::EndTable();
        }

        if (!selectedProcesses.empty()) {
            ImGui::Text("Selected processes: %d", (int)selectedProcesses.size());
        }
    }

    ImGui::End();
}

// network, display information network information
void networkWindow(const char *id, ImVec2 size, ImVec2 position)
{
    ImGui::Begin(id);
    ImGui::SetWindowSize(id, size);
    ImGui::SetWindowPos(id, position);

    // Get network interfaces
    static vector<NetworkInterface> interfaces;
    static float lastUpdate = 0;
    float currentTime = ImGui::GetTime();

    // Update network data every 3 seconds (matches top's default interval)
    if (currentTime - lastUpdate > 3.0f) {
        interfaces = getNetworkInterfaces();
        lastUpdate = currentTime;
    }

    if (interfaces.empty()) {
        ImGui::Text("No network interfaces found");
        ImGui::End();
        return;
    }

    // Helper function to format bytes with appropriate units
    auto formatBytes = [](long long bytes) -> string {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = (double)bytes;

        while (size >= 1024.0 && unit < 4) {
            size /= 1024.0;
            unit++;
        }

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
        return string(buffer);
    };

    // Network Interface Information
    if (ImGui::CollapsingHeader("Network Interfaces", ImGuiTreeNodeFlags_DefaultOpen)) {
        for (const auto& iface : interfaces) {
            ImGui::Text("Interface: %s", iface.name.c_str());
            if (!iface.ip.empty()) {
                ImGui::Text("IP Address: %s", iface.ip.c_str());
            }
            ImGui::Separator();
        }
    }

    // Network Usage Tabs
    if (ImGui::BeginTabBar("NetworkUsageTabs")) {
        // RX (Receive) Tab
        if (ImGui::BeginTabItem("RX (Receive)")) {
            static map<string, vector<float>> rxHistory;

            for (const auto& iface : interfaces) {
                if (ImGui::CollapsingHeader(iface.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Update RX history
                    rxHistory[iface.name].push_back((float)iface.rx.bytes);
                    if (rxHistory[iface.name].size() > 100) {
                        rxHistory[iface.name].erase(rxHistory[iface.name].begin());
                    }

                    // RX Table
                    if (ImGui::BeginTable(("RXTable" + iface.name).c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Metric");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableSetupColumn("Metric");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Bytes");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", formatBytes(iface.rx.bytes).c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Packets");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.rx.packets);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Errors");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.rx.errs);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Dropped");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.rx.drop);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("FIFO");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.rx.fifo);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Frame");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.rx.frame);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Compressed");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.rx.compressed);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Multicast");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.rx.multicast);

                        ImGui::EndTable();
                    }

                    // RX Visual representation
                    if (!rxHistory[iface.name].empty()) {
                        ImGui::Text("RX Usage Graph:");
                        ImGui::PlotLines(("RX " + iface.name).c_str(),
                                       rxHistory[iface.name].data(),
                                       rxHistory[iface.name].size(),
                                       0, formatBytes(iface.rx.bytes).c_str(),
                                       0.0f, FLT_MAX, ImVec2(0, 80));
                    }
                }
            }
            ImGui::EndTabItem();
        }

        // TX (Transmit) Tab
        if (ImGui::BeginTabItem("TX (Transmit)")) {
            static map<string, vector<float>> txHistory;

            for (const auto& iface : interfaces) {
                if (ImGui::CollapsingHeader(iface.name.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Update TX history
                    txHistory[iface.name].push_back((float)iface.tx.bytes);
                    if (txHistory[iface.name].size() > 100) {
                        txHistory[iface.name].erase(txHistory[iface.name].begin());
                    }

                    // TX Table
                    if (ImGui::BeginTable(("TXTable" + iface.name).c_str(), 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                        ImGui::TableSetupColumn("Metric");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableSetupColumn("Metric");
                        ImGui::TableSetupColumn("Value");
                        ImGui::TableHeadersRow();

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Bytes");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%s", formatBytes(iface.tx.bytes).c_str());
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Packets");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.tx.packets);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Errors");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.tx.errs);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Dropped");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.tx.drop);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("FIFO");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.tx.fifo);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Collisions");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.tx.colls);

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Carrier");
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%lld", iface.tx.carrier);
                        ImGui::TableSetColumnIndex(2); ImGui::Text("Compressed");
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%lld", iface.tx.compressed);

                        ImGui::EndTable();
                    }

                    // TX Visual representation
                    if (!txHistory[iface.name].empty()) {
                        ImGui::Text("TX Usage Graph:");
                        ImGui::PlotLines(("TX " + iface.name).c_str(),
                                       txHistory[iface.name].data(),
                                       txHistory[iface.name].size(),
                                       0, formatBytes(iface.tx.bytes).c_str(),
                                       0.0f, FLT_MAX, ImVec2(0, 80));
                    }
                }
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

// Main code
int main(int, char **)
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // GL 3.0 + GLSL 130
    const char *glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window *window = SDL_CreateWindow("Linux System Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
    bool err = gl3wInit() != 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
    bool err = glewInit() != GLEW_OK;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
    bool err = gladLoadGL() == 0;
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD2)
    bool err = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) == 0; // glad2 recommend using the windowing library loader instead of the (optionally) bundled one.
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING2)
    bool err = false;
    glbinding::Binding::initialize();
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLBINDING3)
    bool err = false;
    glbinding::initialize([](const char *name) { return (glbinding::ProcAddress)SDL_GL_GetProcAddress(name); });
#else
    bool err = false; // If you use IMGUI_IMPL_OPENGL_LOADER_CUSTOM, your loader is likely to requires some form of initialization.
#endif
    if (err)
    {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // render bindings
    ImGuiIO &io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // background color
    // note : you are free to change the style of the application
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        {
            ImVec2 mainDisplay = io.DisplaySize;
            memoryProcessesWindow("== Memory and Processes ==",
                                  ImVec2((mainDisplay.x / 2) - 20, (mainDisplay.y / 2) + 30),
                                  ImVec2((mainDisplay.x / 2) + 10, 10));
            // --------------------------------------
            systemWindow("== System ==",
                         ImVec2((mainDisplay.x / 2) - 10, (mainDisplay.y / 2) + 30),
                         ImVec2(10, 10));
            // --------------------------------------
            networkWindow("== Network ==",
                          ImVec2(mainDisplay.x - 20, (mainDisplay.y / 2) - 60),
                          ImVec2(10, (mainDisplay.y / 2) + 50));
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
