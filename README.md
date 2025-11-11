# Packer + Obfuscator

A low-level executable packer and obfuscator written in C++ with Qt GUI.

## Features

- 📦 **Multi-EXE Packing**: Combine multiple executables into a single file
- 🔒 **Code Obfuscation**: Obfuscate individual files and final output
- 🎯 **Launch Order Control**: Drag-and-drop to set execution sequence
- 💾 **Flexible Output**: Generate as EXE or DLL
- 🖥️ **Simple GUI**: User-friendly interface for all operations

## Architecture

### Core Components

1. **PE Parser**: Read and analyze PE (Portable Executable) files
2. **Resource Embedder**: Embed multiple EXEs as resources
3. **Obfuscator Engine**: Apply various obfuscation techniques
4. **Stub Loader**: Runtime unpacker and launcher
5. **GUI**: Qt-based interface

### Technology Stack

- **Language**: C++ (C++17/20)
- **GUI Framework**: Qt 6
- **Build System**: CMake
- **PE Manipulation**: Custom + Windows API
- **Compression**: zlib or LZMA

## How It Works

### Packing Process

```
Input EXEs → Parse PE Headers → (Optional) Obfuscate Each →
Compress → Embed as Resources → Generate Stub → (Optional) Obfuscate Final →
Output Packed EXE/DLL
```

### Runtime Process

```
Packed EXE Launched → Stub Extracts Resources →
Decompress → Load in Order → Execute First → Wait → Execute Second → ...
```

## Building

### Prerequisites

- Visual Studio 2019+ (Windows)
- CMake 3.20+
- Qt 6.x
- Windows SDK

### Build Steps

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```


### Phase 3: GUI
- File import interface
- Drag-drop ordering
- Configuration options
- Progress indicators

### Phase 4: Advanced Features
- Custom icons
- Version info preservation
- Digital signature handling
- Multiple compression algorithms
