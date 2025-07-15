#include "header.h"

// Get memory information from /proc/meminfo to match 'free -h' command
MemoryInfo getMemoryInfo()
{
    MemoryInfo memInfo = {0};

    ifstream file("/proc/meminfo");
    string line;
    unsigned long buffers = 0;
    unsigned long cached = 0;
    unsigned long sReclaimable = 0;

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
        else if (line.find("Buffers:") == 0) {
            sscanf(line.c_str(), "Buffers: %lu kB", &buffers);
            buffers *= 1024;
        }
        else if (line.find("Cached:") == 0) {
            sscanf(line.c_str(), "Cached: %lu kB", &cached);
            cached *= 1024;
        }
        else if (line.find("SReclaimable:") == 0) {
            sscanf(line.c_str(), "SReclaimable: %lu kB", &sReclaimable);
            sReclaimable *= 1024;
        }
    }

    // Calculate used memory the same way 'free' command does
    unsigned long buffCache = buffers + cached + sReclaimable;
    memInfo.usedRAM = memInfo.totalRAM - memInfo.freeRAM - buffCache;
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

