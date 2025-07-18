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
                istringstream iss(line);
                string token;
                vector<string> tokens;

                while (iss >> token) {
                    tokens.push_back(token);
                }

                if (tokens.size() >= 24) {
                    proc.state = tokens[2][0]; // 3rd field (0-indexed)
                    proc.vsize = stoll(tokens[22]); // 23rd field
                    proc.rss = stoll(tokens[23]); // 24th field
                    proc.utime = stoll(tokens[13]); // 14th field
                    proc.stime = stoll(tokens[14]); // 15th field
                }
            }

            processes.push_back(proc);
        }
    }

    closedir(procDir);
    return processes;
}

// Calculate CPU usage for a specific process (matches top command calculation)
double getProcessCPUUsage(const Proc& proc)
{
    static map<int, pair<long long, double>> prevTimes;

    long long totalTime = proc.utime + proc.stime;

    // Get system uptime
    ifstream uptimeFile("/proc/uptime");
    double uptime;
    if (uptimeFile >> uptime) {
        if (prevTimes.find(proc.pid) != prevTimes.end()) {
            long long timeDiff = totalTime - prevTimes[proc.pid].first;
            double uptimeDiff = uptime - prevTimes[proc.pid].second;

            if (uptimeDiff > 0) {
                // Convert clock ticks to seconds and calculate percentage
                double seconds = timeDiff / (double)sysconf(_SC_CLK_TCK);
                double cpuUsage = (seconds / uptimeDiff) * 100.0;

                prevTimes[proc.pid] = {totalTime, uptime};
                return cpuUsage;
            }
        }

        prevTimes[proc.pid] = {totalTime, uptime};
    }

    return 0.0;
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