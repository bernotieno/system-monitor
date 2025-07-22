#include "header.h"

// Get memory information from /proc/meminfo to match 'free -h' command exactly
MemoryInfo getMemoryInfo()
{
    MemoryInfo memInfo = {0};

    ifstream file("/proc/meminfo");
    string line;
    unsigned long memAvailable = 0;

    while (getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu kB", &memInfo.totalRAM);
            memInfo.totalRAM *= 1024; // Convert to bytes
        }
        else if (line.find("MemFree:") == 0) {
            sscanf(line.c_str(), "MemFree: %lu kB", &memInfo.freeRAM);
            memInfo.freeRAM *= 1024;
        }
        else if (line.find("MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %lu kB", &memAvailable);
            memAvailable *= 1024;
        }
        else if (line.find("SwapTotal:") == 0) {
            sscanf(line.c_str(), "SwapTotal: %lu kB", &memInfo.totalSwap);
            memInfo.totalSwap *= 1024;
        }
        else if (line.find("SwapFree:") == 0) {
            sscanf(line.c_str(), "SwapFree: %lu kB", &memInfo.freeSwap);
            memInfo.freeSwap *= 1024;
        }
    }

    // Calculate used memory exactly the same way modern 'free' command does:
    // Used = MemTotal - MemAvailable
    // This matches the 'free -h' output perfectly
    memInfo.usedRAM = memInfo.totalRAM - memAvailable;
    memInfo.usedSwap = memInfo.totalSwap - memInfo.freeSwap;

    return memInfo;
}

// Get disk information using statvfs
DiskInfo getDiskInfo()
{
    DiskInfo diskInfo = {0};
    diskInfo.filesystem = "Unknown";

    // Get filesystem name from /proc/mounts
    ifstream mountsFile("/proc/mounts");
    string line;
    while (getline(mountsFile, line)) {
        istringstream iss(line);
        string device, mountpoint, fstype;
        if (iss >> device >> mountpoint >> fstype) {
            if (mountpoint == "/") {
                diskInfo.filesystem = device;
                break;
            }
        }
    }

    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        // Calculate the same way df does:
        // Total = total blocks * block size
        // Available = available blocks * block size (f_bavail accounts for reserved space)
        // Free = free blocks * block size (f_bfree includes reserved space)
        // Used = Total - Free (using f_bfree, not f_bavail, to match df calculation)
        diskInfo.totalDisk = stat.f_blocks * stat.f_frsize;
        diskInfo.freeDisk = stat.f_bavail * stat.f_frsize;  // Available to non-root users
        unsigned long actualFree = stat.f_bfree * stat.f_frsize;  // Total free including reserved
        diskInfo.usedDisk = diskInfo.totalDisk - actualFree;  // This matches df calculation
    }

    return diskInfo;
}

// Get list of all processes
vector<Proc> getProcessList()
{
    vector<Proc> processes;

    DIR* procDir = opendir("/proc");
    if (procDir == nullptr) return processes;

    struct dirent* entry;
    while ((entry = readdir(procDir)) != nullptr) {
        // Check if directory name is a number (PID)
        if (strspn(entry->d_name, "0123456789") == strlen(entry->d_name)) {
            Proc proc = {0};
            proc.pid = atoi(entry->d_name);

            // Read process name from /proc/PID/comm
            string commPath = "/proc/" + string(entry->d_name) + "/comm";
            ifstream commFile(commPath);
            if (commFile.is_open()) {
                getline(commFile, proc.name);
            }

            // Read process stats from /proc/PID/stat
            string statPath = "/proc/" + string(entry->d_name) + "/stat";
            ifstream statFile(statPath);
            string line;

            if (getline(statFile, line)) {
                // Parse stat file - format is complex, we need specific fields
                // The stat file format: pid (comm) state ppid pgrp session tty_nr tpgid flags minflt cminflt majflt cmajflt utime stime cutime cstime priority nice num_threads itrealvalue starttime vsize rss rsslim...
                istringstream iss(line);
                string token;
                vector<string> tokens;

                while (iss >> token) {
                    tokens.push_back(token);
                }

                if (tokens.size() >= 24) {
                    // Ensure we have a valid state character
                    if (!tokens[2].empty()) {
                        proc.state = tokens[2][0]; // 3rd field (0-indexed) - process state
                    } else {
                        proc.state = '?'; // Unknown state if empty
                    }
                    proc.vsize = stoll(tokens[22]); // 23rd field - virtual memory size
                    proc.rss = stoll(tokens[23]); // 24th field - resident set size
                    proc.utime = stoll(tokens[13]); // 14th field - user time
                    proc.stime = stoll(tokens[14]); // 15th field - system time
                } else {
                    // If we don't have enough fields, set default values
                    proc.state = '?';
                    proc.vsize = 0;
                    proc.rss = 0;
                    proc.utime = 0;
                    proc.stime = 0;
                }
            }

            processes.push_back(proc);
        }
    }

    closedir(procDir);
    return processes;
}

// Calculate CPU usage for a specific process (matches top command calculation exactly)
double getProcessCPUUsage(const Proc& proc)
{
    static map<int, pair<long long, double>> prevTimes;
    static map<int, double> cachedCPUUsage; // Cache CPU usage values
    static map<int, double> lastCalculationTime; // Track when we last calculated for each process

    long long totalTime = proc.utime + proc.stime;

    // Get current time
    static auto start = chrono::steady_clock::now();
    auto now = chrono::steady_clock::now();
    double currentTime = chrono::duration<double>(now - start).count();

    // Get system uptime
    ifstream uptimeFile("/proc/uptime");
    double uptime;
    if (!uptimeFile || !(uptimeFile >> uptime)) {
        return 0.0;
    }

    // Check if we have previous data for this process
    if (prevTimes.find(proc.pid) != prevTimes.end()) {
        long long timeDiff = totalTime - prevTimes[proc.pid].first;
        double uptimeDiff = uptime - prevTimes[proc.pid].second;

        // Only calculate if we have a meaningful time difference (at least 2.5 seconds to match top's 3-second interval)
        if (uptimeDiff >= 2.5) {
            // Convert clock ticks to seconds and calculate percentage
            // This matches top's calculation: CPU% = (process_time_diff / real_time_diff) * 100
            double seconds = timeDiff / (double)sysconf(_SC_CLK_TCK);
            double cpuUsage = (seconds / uptimeDiff) * 100.0;

            // Clamp CPU usage to reasonable range (0-100%)
            cpuUsage = max(0.0, min(100.0, cpuUsage));

            // Update previous times and cache the result
            prevTimes[proc.pid] = {totalTime, uptime};
            cachedCPUUsage[proc.pid] = cpuUsage;
            lastCalculationTime[proc.pid] = currentTime;

            return cpuUsage;
        } else {
            // Time difference too small, return cached value if available
            if (cachedCPUUsage.find(proc.pid) != cachedCPUUsage.end()) {
                return cachedCPUUsage[proc.pid];
            }
        }
    }

    // First time seeing this process or no previous data, store initial values
    prevTimes[proc.pid] = {totalTime, uptime};
    cachedCPUUsage[proc.pid] = 0.0;
    lastCalculationTime[proc.pid] = currentTime;

    return 0.0;
}

// Get current process state in real-time (like top does)
char getCurrentProcessState(int pid)
{
    string statPath = "/proc/" + to_string(pid) + "/stat";
    ifstream statFile(statPath);
    string line;

    if (getline(statFile, line)) {
        // Find the last ')' to handle process names with spaces/parentheses
        size_t lastParen = line.find_last_of(')');
        if (lastParen != string::npos && lastParen + 2 < line.length()) {
            // State is the first character after ") "
            char state = line[lastParen + 2];
            
         
            
            return state;
        }
    }

    return '?';
}

// Calculate memory usage percentage for a process
double getProcessMemoryUsage(const Proc& proc)
{
    MemoryInfo memInfo = getMemoryInfo();
    if (memInfo.totalRAM > 0) {
        return (double)(proc.rss * getpagesize()) / memInfo.totalRAM * 100.0;
    }
    return 0.0;
}

float GetCPUUsage(int pid) {
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";

    // Helper lambda to measure process and system CPU time
    auto measure = [&]() -> std::pair<long, long> {
        // Open process stat file
        std::ifstream statFile(statPath);
        if (!statFile.is_open()) {
            // std::cerr << "Could not open file: " << statPath << std::endl;
            return {-1, -1};
        }

        // Read process stats
        std::string statLine;
        if (!std::getline(statFile, statLine)) {
            std::cerr << "Failed to read from file: " << statPath << std::endl;
            return {-1, -1};
        }

        // Parse stat line into fields
        std::istringstream iss(statLine);
        std::vector<std::string> statFields;
        std::string field;

        while (iss >> field) {
            statFields.push_back(field);
        }

        // Check we have enough fields
        if (statFields.size() < 15) {
            std::cerr << "Insufficient data in stat file for PID: " << pid << std::endl;
            return {-1, -1};
        }

        // Get process CPU times
        long utime = std::stol(statFields[13]);  // user mode time
        long stime = std::stol(statFields[14]);  // system mode time
        long total_time = utime + stime;

        // Get system-wide CPU times
        std::ifstream cpuFile("/proc/stat");
        if (!cpuFile.is_open()) {
            std::cerr << "Could not open /proc/stat" << std::endl;
            return {-1, -1};
        }

        std::string cpuLine;
        if (!std::getline(cpuFile, cpuLine)) {
            std::cerr << "Failed to read from /proc/stat" << std::endl;
            return {-1, -1};
        }

        // Parse CPU times from stat file
        std::istringstream cpuStream(cpuLine);
        std::string cpuLabel;
        long user, nice, system, idle, iowait, irq, softirq, steal;
        if (!(cpuStream >> cpuLabel >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) {
            std::cerr << "Failed to parse CPU data from /proc/stat" << std::endl;
            return {-1, -1};
        }
        long total_system_time = user + nice + system + idle + iowait + irq + softirq + steal;

        return {total_time, total_system_time};
    };

    // Take first measurement
    auto [total_time1, system_time1] = measure();
    if (total_time1 < 0 || system_time1 < 0) return -1.0;

    // Wait 3 seconds between measurements
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));  

    // Take second measurement
    auto [total_time2, system_time2] = measure();
    if (total_time2 < 0 || system_time2 < 0) return -1.0;

    // Get system constants
    long hertz = sysconf(_SC_CLK_TCK);
    if (hertz <= 0) {
        std::cerr << "Invalid clock ticks per second" << std::endl;
        return -1.0;
    }

    long numCores = sysconf(_SC_NPROCESSORS_ONLN);
    if (numCores <= 0) {
        std::cerr << "Invalid number of CPU cores" << std::endl;
        return -1.0;
    }

    // Calculate time differences
    float total_time_diff = static_cast<float>(total_time2 - total_time1) ;
    float system_time_diff = static_cast<float>(system_time2 - system_time1) ;

    if (system_time_diff <= 0) {
        std::cerr << "Invalid system time difference" << std::endl;
        return -1.0;
    }

    // Calculate CPU usage percentage (total_time_diff is the process time, system_time_diff is overall CPU time)
    float cpuUsage = 100.0 * (total_time_diff / system_time_diff);

    // Multiply by the number of cores to match top's calculation
    cpuUsage *= numCores;

    return cpuUsage;
}
