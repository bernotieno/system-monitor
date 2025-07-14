# 🖥️ Linux Desktop System Monitor

A comprehensive real-time system monitoring application built with C++ and Dear ImGui, designed specifically for Linux systems. Monitor CPU usage, memory consumption, network activity, thermal sensors, and running processes through an intuitive graphical interface.

![System Monitor](https://img.shields.io/badge/Platform-Linux-blue)
![Language](https://img.shields.io/badge/Language-C%2B%2B-orange)
![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-green)
![License](https://img.shields.io/badge/License-MIT-yellow)

## ✨ Features

### 🖥️ System Information
- **Operating System**: Display current OS information
- **User Details**: Show current username and hostname
- **CPU Information**: Detailed processor model and specifications
- **Process States**: Real-time count of running, sleeping, zombie, and stopped processes

### 📊 CPU & Hardware Monitoring
- **Real-time CPU Usage**: Live percentage with animated graphs
- **Interactive Controls**: FPS slider (1-120), Y-axis scaling (50-200%)
- **CPU History Graph**: 100-point rolling history with animation toggle
- **Thermal Monitoring**: Temperature sensors with color-coded warnings
- **Fan Monitoring**: RPM readings with status indicators

### 💾 Memory & Process Management
- **Memory Usage**: Visual RAM and SWAP usage with progress bars
- **Disk Usage**: Root filesystem usage monitoring
- **Process Table**: Sortable table with PID, Name, State, CPU%, Memory%
- **Process Filtering**: Real-time search and filter capabilities
- **Multi-Selection**: Select multiple processes with Ctrl+click

### 🌐 Network Monitoring
- **Interface Detection**: Automatic discovery of all network interfaces
- **IP Address Display**: Show assigned IP addresses for each interface
- **RX/TX Statistics**: Detailed receive and transmit statistics
- **Traffic Visualization**: Progress bars with auto-scaling units (B/KB/MB/GB)
- **Error Monitoring**: Track network errors, drops, and collisions

## 🛠️ Prerequisites

### System Requirements
- **Operating System**: Linux (Ubuntu, Debian, Fedora, Arch, etc.)
- **Compiler**: GCC with C++11 support
- **Graphics**: OpenGL 3.0+ support

### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libsdl2-dev libgl1-mesa-dev

# Fedora/RHEL
sudo dnf install gcc-c++ SDL2-devel mesa-libGL-devel

# Arch Linux
sudo pacman -S base-devel sdl2 mesa
```

## 🚀 Quick Start

### 1. Clone the Repository
```bash
git clone <repository-url>
cd system-monitor
```

### 2. Build the Application
```bash
make clean && make
```

### 3. Run the System Monitor
```bash
./monitor
```

## 📖 Detailed Installation Guide

### Step 1: Install Dependencies

#### Ubuntu/Debian Systems:
```bash
# Update package list
sudo apt-get update

# Install build tools and libraries
sudo apt-get install -y build-essential libsdl2-dev libgl1-mesa-dev

# Verify installation
sdl2-config --version
```

#### Fedora/CentOS/RHEL Systems:
```bash
# Install development tools
sudo dnf groupinstall "Development Tools"

# Install SDL2 and OpenGL
sudo dnf install SDL2-devel mesa-libGL-devel

# Verify installation
pkg-config --modversion sdl2
```

### Step 2: Compile the Project

The project uses a Makefile for easy compilation:

```bash
# Clean previous builds (optional)
make clean

# Compile the project
make

# Check for successful compilation
ls -la monitor
```

### Step 3: Run the Application

```bash
# Execute the system monitor
./monitor

# Run with specific permissions if needed
sudo ./monitor  # For accessing some system files
```

## 🎮 Usage Guide

### Interface Overview

The application displays three main panels:

1. **System Panel** (Left): System info, CPU usage, thermal data
2. **Memory & Processes Panel** (Top Right): RAM/SWAP usage, process table
3. **Network Panel** (Bottom): Network interface statistics

### Interactive Controls

#### CPU Monitoring:
- **Animate Checkbox**: Toggle real-time graph animation
- **FPS Slider**: Adjust update frequency (1-120 FPS)
- **Y-Scale Slider**: Modify graph scale (50-200%)

#### Process Management:
- **Filter Box**: Type to filter processes by name
- **Column Headers**: Click to sort by PID, Name, State, CPU%, Memory%
- **Multi-Select**: Hold Ctrl and click to select multiple processes

#### Network Monitoring:
- **Collapsible Headers**: Click interface names to expand/collapse details
- **Progress Bars**: Visual representation of network usage
- **Statistics Tables**: Detailed RX/TX information

### Keyboard Shortcuts
- **Ctrl+Click**: Multi-select processes
- **Escape**: Close application
- **Mouse Wheel**: Scroll through process list

## 🔧 Configuration

### Build Options

Modify the Makefile for custom build configurations:

```makefile
# Debug build
CXXFLAGS += -DDEBUG -g

# Release build
CXXFLAGS += -O3 -DNDEBUG

# Custom SDL2 path
CXXFLAGS += -I/custom/path/to/sdl2
```

### Runtime Configuration

The application automatically saves window positions and sizes in `imgui.ini`. Delete this file to reset to defaults.

## 🐛 Troubleshooting

### Common Issues

#### Compilation Errors:
```bash
# SDL2 not found
sudo apt-get install libsdl2-dev

# OpenGL headers missing
sudo apt-get install libgl1-mesa-dev

# Permission denied
chmod +x monitor
```

#### Runtime Issues:
```bash
# Cannot access /proc files
sudo ./monitor

# Display issues
export DISPLAY=:0
./monitor

# Missing libraries
ldd monitor  # Check dependencies
```

### Performance Tips

- **High CPU Usage**: Reduce FPS slider value
- **Memory Issues**: Limit process table to fewer entries
- **Network Lag**: Increase network update interval

## 📁 Project Structure

```
system-monitor/
├── main.cpp           # Main application and GUI implementation
├── system.cpp         # System information and hardware monitoring
├── mem.cpp           # Memory and process management
├── network.cpp       # Network interface monitoring
├── header.h          # Function declarations and data structures
├── Makefile          # Build configuration
├── imgui/            # Dear ImGui library
├── monitor           # Compiled executable (generated)
└── README.md         # This documentation
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Dear ImGui**: Excellent immediate mode GUI library
- **SDL2**: Cross-platform development library
- **Linux Kernel**: For providing comprehensive /proc and /sys interfaces

## 📞 Support

For issues, questions, or contributions:
- Open an issue on GitHub
- Check the troubleshooting section above
- Review the project documentation

---

**Made with ❤️ for the Linux community**
