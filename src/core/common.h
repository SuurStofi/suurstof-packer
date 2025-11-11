#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <memory>
#include <Windows.h>

namespace Packer {

// File type enumeration
enum class FileType {
    EXECUTABLE,  // .exe, .com
    SCRIPT,      // .bat, .cmd, .ps1, .vbs
    DOCUMENT,    // .txt, .pdf, .doc, etc.
    ARCHIVE,     // .zip, .rar, .7z
    IMAGE,       // .jpg, .png, .bmp
    OTHER        // Any other file type
};

// Generic file information structure (replaces PEInfo)
struct FileInfo {
    std::wstring filePath;
    std::vector<uint8_t> fileData;
    size_t fileSize;
    FileType fileType;
    std::wstring originalName;
    std::wstring extension;
    int executionOrder;
    bool obfuscate;  // Only applicable for executables
    
    // PE-specific fields (only valid if fileType == EXECUTABLE)
    bool is64Bit;
    DWORD entryPoint;
    DWORD imageBase;
    
    FileInfo() : fileSize(0), fileType(FileType::OTHER), executionOrder(0), 
                 obfuscate(false), is64Bit(false), entryPoint(0), imageBase(0) {}
};

// Legacy typedef for backward compatibility
typedef FileInfo PEInfo;

// Obfuscation options
struct ObfuscationOptions {
    bool encryptStrings;
    bool obfuscateControlFlow;
    bool obfuscateImports;
    bool addAntiDebug;
    bool addJunkCode;
    
    ObfuscationOptions() : encryptStrings(true), obfuscateControlFlow(true),
                          obfuscateImports(true), addAntiDebug(true), 
                          addJunkCode(false) {}
};

// Output options
enum class OutputType {
    EXE,
    DLL
};

struct PackerOptions {
    OutputType outputType;
    std::wstring outputPath;
    bool obfuscateFinal;
    bool waitForPrevious;  // Wait for each file to finish before running next
    ObfuscationOptions obfuscationOpts;
    
    PackerOptions() : outputType(OutputType::EXE), obfuscateFinal(false), 
                     waitForPrevious(true) {}
};

} // namespace Packer

#endif // COMMON_H
