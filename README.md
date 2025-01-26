# amcsh - High-Performance Shell

A modern, high-performance shell designed for speed, efficiency, and usability.

## Features

- Minimal startup time and low-latency command execution
- Zero-cost abstractions for command parsing
- Asynchronous I/O and lightweight threading
- Intelligent command history with fuzzy matching
- Built-in command optimization
- POSIX compliant with optional extensions

## Building

Requirements:
- C compiler (gcc/clang)
- CMake (>= 3.10)
- libedit (for line editing)
- pthread

```bash
mkdir build
cd build
cmake ..
make
```

## Architecture

The shell is built with performance in mind:

- Command Parser: Zero-copy parsing with minimal allocations
- Execution Engine: Optimized process creation and management
- Job Control: Lightweight thread-based job management
- History: Efficient in-memory circular buffer with disk persistence

## License

MIT License
