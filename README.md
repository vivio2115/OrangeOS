# OrangeOS

[![OrangeOS Build](https://github.com/vivio2115/OrangeOS/actions/workflows/main.yml/badge.svg)](https://github.com/vivio2115/OrangeOS/actions/workflows/main.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Version: 1.0](https://img.shields.io/badge/Version:-1.0-orange)
![GitHub commit activity](https://img.shields.io/github/commit-activity/w/vivio2115/OrangeOS)


A small x86 operating system written in Assembly and C++.

> **Fun fact:** The name comes from my orange cat!

## Features

- **FAT32 File System** - Full read/write support for FAT32 formatted disks
- **Shell Interface** - Command-line shell with essential utilities
- **Text Editor** - Built-in vi-like editor for file creation and editing
- **Memory Management** - Custom heap allocator and paging implementation
- **Hardware Drivers** - ATA disk controller, VGA text mode, keyboard, timer

## Building

Requirements:
- `nasm` - Netwide Assembler
- `i686-elf-gcc` - Cross compiler for x86
- `make` - Build automation
- `qemu-system-i386` - For testing (optional)

Build the OS image:
```bash
make
```

Run in QEMU:
```bash
make run
```

## Shell Commands

- `help` - Display available commands
- `ls` - List files in current directory
- `cat <file>` - Display file contents
- `vi <file>` - Edit or create a file
- `mkdir <dir>` - Create a directory
- `echo <text>` - Print text to screen
- `clear` - Clear the screen
- `meminfo` - Show memory usage
- `reboot` - Restart the system
- `halt` - Shutdown the system

## Roadmap

### v1.2 (In Development)
Development happens on the `v1.2` branch with incremental updates. Once all features are complete and stable, it will be merged to main as the official v1.2 release.

Planned features:
- Shell refactoring - Better command parsing and modular structure
- Vi editor improvements - Search/replace, syntax highlighting, undo/redo
- PC Speaker support - Basic beep and sound effects
- Code quality improvements - Better structure, documentation, and maintainability
- Networking stack - Initial TCP/IP implementation

See [CHANGELOG.md](CHANGELOG.md) for version history.

## License

This project is licensed under the MIT License - see the LICENSE file for details.