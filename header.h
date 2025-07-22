// To make sure you don't declare the function more than once by including the header multiple times.
#ifndef header_H
#define header_H

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <dirent.h>
#include <vector>
#include <iostream>
#include <cmath>
#include <chrono>
#include <future>
// lib to read from file
#include <fstream>
// for the name of the computer and the logged in user
#include <unistd.h>
#include <limits.h>
// this is for us to get the cpu information
// mostly in unix system
// not sure if it will work in windows
#include <cpuid.h>
// this is for the memory usage and other memory visualization
// for linux gotta find a way for windows
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
// for time and date
#include <ctime>
// ifconfig ip addresses
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <string>
#include <sstream>
#include <algorithm>
#include <netdb.h>
#include <thread>


using namespace std;

struct CPUStats
{
    long long int user;
    long long int nice;
    long long int system;
    long long int idle;
    long long int iowait;
    long long int irq;
    long long int softirq;
    long long int steal;
    long long int guest;
    long long int guestNice;
};

// processes `stat`
struct Proc
{
    int pid;
    string name;
    char state;
    long long int vsize;
    long long int rss;
    long long int utime;
    long long int stime;
};

struct IP4
{
    char *name;
    char addressBuffer[INET_ADDRSTRLEN];
};

struct Networks
{
    vector<IP4> ip4s;
};

struct RX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long frame;
    long long compressed;
    long long multicast;
};
struct TX
{
    long long bytes;
    long long packets;
    long long errs;
    long long drop;
    long long fifo;
    long long colls;
    long long carrier;
    long long compressed;
};

// System stats functions
string CPUinfo();
const char *getOsName();
string getUsername();
string getHostname();
CPUStats getCPUStats();
double getCPUUsage();
map<char, int> getProcessCountByState();
int getTotalTaskCount();
map<string, int> getTopStyleProcessCounts();

// Memory and processes functions
struct MemoryInfo {
    unsigned long totalRAM;
    unsigned long freeRAM;
    unsigned long usedRAM;
    unsigned long totalSwap;
    unsigned long freeSwap;
    unsigned long usedSwap;
};

struct DiskInfo {
    unsigned long totalDisk;
    unsigned long freeDisk;
    unsigned long usedDisk;
    string filesystem;
};

MemoryInfo getMemoryInfo();
DiskInfo getDiskInfo();
vector<Proc> getProcessList();
double getProcessCPUUsage(const Proc& proc);
double getProcessMemoryUsage(const Proc& proc);
char getCurrentProcessState(int pid);

// Network functions
struct NetworkInterface {
    string name;
    string ip;
    RX rx;
    TX tx;
};

vector<NetworkInterface> getNetworkInterfaces();

// Thermal and fan functions
struct ThermalInfo {
    double temperature;
    string label;
};

struct FanInfo {
    int speed;
    string label;
};

vector<ThermalInfo> getThermalInfo();
vector<FanInfo> getFanInfo();

// UI Enhancement functions
void setupEnhancedStyle();
void drawSectionHeader(const char* icon, const char* title, ImVec4 color);
void drawInfoCard(const char* label, const char* value, ImVec4 labelColor);
float GetCPUUsage(int pid);

#endif
