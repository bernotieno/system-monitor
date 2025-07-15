#include "header.h"

// Get network interface information
vector<NetworkInterface> getNetworkInterfaces()
{
    vector<NetworkInterface> interfaces;

    // Get IP addresses using getifaddrs
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        return interfaces;
    }

    map<string, string> interfaceIPs;

    // First pass: collect IP addresses
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;

        int family = ifa->ifa_addr->sa_family;

        if (family == AF_INET) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                               host, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST);
            if (s == 0) {
                interfaceIPs[ifa->ifa_name] = host;
            }
        }
    }

    freeifaddrs(ifaddr);

    // Second pass: read network statistics from /proc/net/dev
    ifstream netFile("/proc/net/dev");
    string line;

    // Skip header lines
    getline(netFile, line);
    getline(netFile, line);

    while (getline(netFile, line)) {
        // Parse network interface statistics
        size_t colonPos = line.find(':');
        if (colonPos == string::npos) continue;

        string interfaceName = line.substr(0, colonPos);
        // Remove leading whitespace
        interfaceName.erase(0, interfaceName.find_first_not_of(" \t"));

        string stats = line.substr(colonPos + 1);
        istringstream iss(stats);

        NetworkInterface netInterface;
        netInterface.name = interfaceName;
        netInterface.ip = interfaceIPs[interfaceName];

        // Parse RX stats (receive)
        iss >> netInterface.rx.bytes >> netInterface.rx.packets
            >> netInterface.rx.errs >> netInterface.rx.drop
            >> netInterface.rx.fifo >> netInterface.rx.frame
            >> netInterface.rx.compressed >> netInterface.rx.multicast;

        // Parse TX stats (transmit)
        iss >> netInterface.tx.bytes >> netInterface.tx.packets
            >> netInterface.tx.errs >> netInterface.tx.drop
            >> netInterface.tx.fifo >> netInterface.tx.colls
            >> netInterface.tx.carrier >> netInterface.tx.compressed;

        interfaces.push_back(netInterface);
    }

    return interfaces;
}
