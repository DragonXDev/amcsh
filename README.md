# 🚀 amcsh - Advanced Modern C Shell

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![C Version](https://img.shields.io/badge/C-C11-orange.svg)]()
[![Platform](https://img.shields.io/badge/platform-macOS%20|%20Linux-lightgrey.svg)]()

<div align="center">

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="assets/images/amcsh-dark.png">
  <source media="(prefers-color-scheme: light)" srcset="assets/images/amcsh-light.png">
  <img alt="amcsh Logo" src="assets/images/amcsh-light.png" width="400">
</picture>

*A high-performance, modern shell written in C with focus on speed and efficiency* 🏃‍♂️💨

</div>

## ✨ Features

- 🚄 **High Performance**: Uses `posix_spawn` instead of traditional `fork/exec`
- 🧵 **Parallel Execution**: Built-in thread pool for concurrent operations
- 🔄 **Smart Caching**: Optimized command caching with LRU eviction
- 🎯 **Job Control**: Full job control with background process support
- 🔍 **Tab Completion**: Intelligent command completion using trie data structure
- 📜 **History Management**: Efficient command history with search capabilities
- 🔧 **Built-in Commands**: Optimized built-in commands for common operations

## 🎯 Performance

AMCSH is designed with performance in mind:

- ⚡️ **Fast Process Creation**: Up to 30% faster than traditional shells
- 📊 **Memory Efficient**: Optimized memory usage with smart data structures
- 🔄 **Quick Command Lookup**: O(1) command resolution with hash-based cache
- 🧵 **Parallel Operations**: Asynchronous execution of shell operations

## 🚀 Getting Started

### Prerequisites

```bash
# Install CMake (macOS)
brew install cmake

# Install CMake (Ubuntu/Debian)
sudo apt-get install cmake
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/yourusername/amcsh.git
cd amcsh

# Create build directory
mkdir build && cd build

# Build
cmake ..
make

# Run AMCSH
./amcsh
```

## 💡 Usage

### Basic Commands

```bash
# Change directory
cd /path/to/directory

# List jobs
jobs

# Background processes
command &

# Bring job to foreground
fg %1

# Send job to background
bg %1
```

### Advanced Features

```bash
# Command history search
ctrl-r

# Tab completion
command<tab>

# Process substitution
command $(another_command)

# I/O redirection
command > output.txt 2> error.log
```

## 🏗 Architecture

AMCSH is built with a modular architecture:

```
amcsh/
├── src/
│   ├── main.c          # Main shell loop
│   ├── executor.c      # Command execution
│   ├── parser.c        # Command parsing
│   ├── builtins.c      # Built-in commands
│   ├── completion.c    # Tab completion
│   ├── history.c       # History management
│   ├── job_control.c   # Job control
│   ├── thread_pool.c   # Thread pool
│   └── cmd_cache.c     # Command cache
├── include/
│   ├── amcsh.h         # Main header
│   └── parser.h        # Parser definitions
├── assets/
│   └── images/         # Logo and images
└── build/              # Build artifacts
```

## 🔧 Configuration

AMCSH can be configured through environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `AMCSH_HISTORY_SIZE` | Maximum history entries | 1000 |
| `AMCSH_CACHE_SIZE` | Command cache size | 128 |
| `AMCSH_MAX_THREADS` | Thread pool size | 4 |

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guide](CONTRIBUTING.md) for details.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'feat: Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a PR

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 👥 Authors

- Amaar - *Initial work* - [GitHub Profile](https://github.com/dragonxdev)

## 🙏 Acknowledgments

- Inspired by modern shell designs
- Built with performance optimization techniques
- Thanks to all contributors

---
<div align="center">
Made by Amaar :)
</div>
