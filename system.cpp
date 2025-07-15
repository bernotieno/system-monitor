#include "header.h"

// get cpu id and information from /proc/cpuinfo
string CPUinfo()
{
    ifstream file("/proc/cpuinfo");
    string line;

    while (getline(file, line)) {
        if (line.find("model name") == 0) {
            size_t colonPos = line.find(':');
            if (colonPos != string::npos) {
                string cpuName = line.substr(colonPos + 1);
                // Remove leading whitespace
                cpuName.erase(0, cpuName.find_first_not_of(" \t"));
                return cpuName;
            }
        }
    }

    return "Unknown CPU";
}

// getOsName, this will get the OS of the current computer
const char *getOsName()
{
#ifdef _WIN32
    return "Windows 32-bit";
#elif _WIN64
    return "Windows 64-bit";
#elif __APPLE__ || __MACH__
    return "Mac OSX";
#elif __linux__
    return "Linux";
#elif __FreeBSD__
    return "FreeBSD";
#elif __unix || __unix__
    return "Unix";
#else
    return "Other";
#endif
}

// Get current username
string getUsername()
{
    char* username = getenv("USER");
    if (username == nullptr) {
        username = getenv("USERNAME"); // Windows fallback
    }
    return username ? string(username) : "Unknown";
}

// Get hostname
string getHostname()
{
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return string(hostname);
    }
    return "Unknown";
}

// Read CPU stats from /proc/stat
CPUStats getCPUStats()
{
    CPUStats stats = {0};
    ifstream file("/proc/stat");
    string line;

    if (getline(file, line)) {
        sscanf(line.c_str(), "cpu %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld",
               &stats.user, &stats.nice, &stats.system, &stats.idle,
               &stats.iowait, &stats.irq, &stats.softirq, &stats.steal,
               &stats.guest, &stats.guestNice);
    }

    return stats;
}

// Calculate CPU usage percentage
double getCPUUsage()
{
    static CPUStats prevStats = {0};
    static bool firstCall = true;

    CPUStats currentStats = getCPUStats();

    if (firstCall) {
        prevStats = currentStats;
        firstCall = false;
        return 0.0;
    }

    long long prevIdle = prevStats.idle + prevStats.iowait;
    long long idle = currentStats.idle + currentStats.iowait;

    long long prevNonIdle = prevStats.user + prevStats.nice + prevStats.system +
                           prevStats.irq + prevStats.softirq + prevStats.steal;
    long long nonIdle = currentStats.user + currentStats.nice + currentStats.system +
                       currentStats.irq + currentStats.softirq + currentStats.steal;

    long long prevTotal = prevIdle + prevNonIdle;
    long long total = idle + nonIdle;

    long long totalDiff = total - prevTotal;
    long long idleDiff = idle - prevIdle;

    double cpuPercentage = 0.0;
    if (totalDiff != 0) {
        cpuPercentage = (double)(totalDiff - idleDiff) / totalDiff * 100.0;
    }

    prevStats = currentStats;
    return cpuPercentage;
}

// Get process count by state
map<char, int> getProcessCountByState()
{
    map<char, int> stateCounts;
    stateCounts['R'] = 0; // Running
    stateCounts['S'] = 0; // Sleeping
    stateCounts['D'] = 0; // Disk sleep
    stateCounts['Z'] = 0; // Zombie
    stateCounts['T'] = 0; // Stopped
    stateCounts['t'] = 0; // Tracing stop
    stateCounts['X'] = 0; // Dead

    DIR* procDir = opendir("/proc");
    if (procDir == nullptr) return stateCounts;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream statFile(statPath);
            string line;

            if (getline(statFile, line)) {
                // Parse the stat file to get the state (3rd field)
                size_t pos = line.find(')');
                if (pos != string::npos && pos + 2 < line.length()) {
                    char state = line[pos + 2];
                    if (stateCounts.find(state) != stateCounts.end()) {
                        stateCounts[state]++;
                    }
                }
            }
        }
    }

    closedir(procDir);
    return stateCounts;
}

// Get total task count (matches 'top' command output)
int getTotalTaskCount()
{
    map<char, int> stateCounts = getProcessCountByState();
    int total = 0;

    // Sum all process states to get total
    for (const auto& pair : stateCounts) {
        total += pair.second;
    }

    return total;
}

// Get thermal information from /sys/class/thermal and /proc/acpi/ibm/thermal
vector<ThermalInfo> getThermalInfo()
{
    vector<ThermalInfo> thermalData;
    bool foundThermal = false;

    // First try IBM ACPI thermal (ThinkPad specific)
    ifstream ibmThermalFile("/proc/acpi/ibm/thermal");
    if (ibmThermalFile.is_open()) {
        string line;
        if (getline(ibmThermalFile, line)) {
            // Format is typically: "temperatures:   42 0 0 0 0 0 0 0"
            if (line.find("temperatures:") != string::npos) {
                istringstream iss(line.substr(line.find(":") + 1));
                int temp;
                int sensorIndex = 0;

                while (iss >> temp) {
                    if (temp > 0) { // Only add non-zero temperatures
                        ThermalInfo info;
                        info.temperature = temp; // IBM ACPI reports directly in Celsius
                        info.label = "IBM Sensor " + to_string(sensorIndex);
                        thermalData.push_back(info);
                        foundThermal = true;
                    }
                    sensorIndex++;
                }
            }
        }
    }

    // If no IBM thermal found, try standard thermal zones
    if (!foundThermal) {
        for (int i = 0; i < 10; i++) { // Check up to 10 thermal zones
            string tempPath = "/sys/class/thermal/thermal_zone" + to_string(i) + "/temp";
            string typePath = "/sys/class/thermal/thermal_zone" + to_string(i) + "/type";

            ifstream tempFile(tempPath);
            ifstream typeFile(typePath);

            if (tempFile.is_open() && typeFile.is_open()) {
                int tempMilliC;
                string type;

                if (tempFile >> tempMilliC && getline(typeFile, type)) {
                    ThermalInfo info;
                    info.temperature = tempMilliC / 1000.0; // Convert from millicelsius
                    info.label = type;
                    thermalData.push_back(info);
                }
            }
        }
    }

    return thermalData;
}

// Get fan information from /sys/class/hwmon
vector<FanInfo> getFanInfo()
{
    vector<FanInfo> fanData;

    for (int i = 0; i < 10; i++) { // Check up to 10 hwmon devices
        string fanPath = "/sys/class/hwmon/hwmon" + to_string(i) + "/fan1_input";
        string labelPath = "/sys/class/hwmon/hwmon" + to_string(i) + "/fan1_label";

        ifstream fanFile(fanPath);
        ifstream labelFile(labelPath);

        if (fanFile.is_open()) {
            int fanSpeed;
            string label = "Fan " + to_string(i + 1);

            if (fanFile >> fanSpeed) {
                // Try to get label if available
                string labelStr;
                if (labelFile.is_open() && getline(labelFile, labelStr)) {
                    label = labelStr;
                }

                FanInfo info;
                info.speed = fanSpeed;
                info.label = label;
                fanData.push_back(info);
            }
        }
    }

    return fanData;
}
