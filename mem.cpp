#include "header.h"

// Get memory information from /proc/meminfo
MemoryInfo getMemoryInfo()
{
    MemoryInfo memInfo = {0};

    ifstream file("/proc/meminfo");
    string line;

    while (getline(file, line)) {
        if (line.find("MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %lu kB", &memInfo.totalRAM);
            memInfo.totalRAM *= 1024; // Convert to bytes
        }
        else if (line.find("MemFree:") == 0) {
            sscanf(line.c_str(), "MemFree: %lu kB", &memInfo.freeRAM);
            memInfo.freeRAM *= 1024;
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

    memInfo.usedRAM = memInfo.totalRAM - memInfo.freeRAM;
    memInfo.usedSwap = memInfo.totalSwap - memInfo.freeSwap;

    return memInfo;
}

// Get disk information using statvfs
DiskInfo getDiskInfo()
{
    DiskInfo diskInfo = {0};

    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        diskInfo.totalDisk = stat.f_blocks * stat.f_frsize;
        diskInfo.freeDisk = stat.f_bavail * stat.f_frsize;
        diskInfo.usedDisk = diskInfo.totalDisk - diskInfo.freeDisk;
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

// Calculate CPU usage for a specific process
double getProcessCPUUsage(const Proc& proc)
{
    static map<int, pair<long long, long long>> prevTimes;
    static map<int, long long> prevSystemTime;

    long long totalTime = proc.utime + proc.stime;

    // Get system uptime
    ifstream uptimeFile("/proc/uptime");
    double uptime;
    if (uptimeFile >> uptime) {
        long long systemTime = (long long)(uptime * sysconf(_SC_CLK_TCK));

        if (prevTimes.find(proc.pid) != prevTimes.end()) {
            long long timeDiff = totalTime - prevTimes[proc.pid].first;
            long long systemTimeDiff = systemTime - prevSystemTime[proc.pid];

            if (systemTimeDiff > 0) {
                double cpuUsage = (double)timeDiff / systemTimeDiff * 100.0;
                prevTimes[proc.pid] = {totalTime, systemTime};
                prevSystemTime[proc.pid] = systemTime;
                return cpuUsage;
            }
        }

        prevTimes[proc.pid] = {totalTime, systemTime};
        prevSystemTime[proc.pid] = systemTime;
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
