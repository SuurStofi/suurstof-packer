#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <shellapi.h>
#include <shlwapi.h>
#include <tlhelp32.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

// Helper function to launch process with CreateProcess
bool LaunchProcessDirect(const std::wstring& filePath, bool waitForCompletion) {
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    
    // Create a mutable copy of the command line
    std::vector<wchar_t> cmdLine(filePath.begin(), filePath.end());
    cmdLine.push_back(L'\0');
    
    if (CreateProcessW(
        NULL,
        cmdLine.data(),
        NULL,
        NULL,
        FALSE,
        CREATE_NEW_CONSOLE,
        NULL,
        NULL,
        &si,
        &pi
    )) {
        if (waitForCompletion && pi.hProcess) {
            WaitForSingleObject(pi.hProcess, INFINITE);
        }
        
        if (pi.hProcess) CloseHandle(pi.hProcess);
        if (pi.hThread) CloseHandle(pi.hThread);
        
        return true;
    }
    
    return false;
}

// Manifest structure (must match ResourceEmbedder)
#pragma pack(push, 1)
struct ResourceEntry {
    uint32_t id;
    uint32_t offset;
    uint32_t size;
    uint32_t originalSize;
    uint8_t compressed;
    uint32_t executionOrder;
    wchar_t extension[8];  // 16 bytes
};

struct ManifestHeader {
    char magic[4];      // "PACK"
    uint32_t version;
    uint32_t entryCount;
    uint8_t waitForPrevious;  // 1 = wait for each to finish, 0 = run all at once
};
#pragma pack(pop)

// Marker to find resources section
const char RESOURCE_MARKER[] = "PACKEDRES_V2";

bool findResourceSection(std::vector<uint8_t>& exeData, size_t& outOffset) {
    size_t markerSize = sizeof(RESOURCE_MARKER) - 1; // Exclude null terminator
    
    if (exeData.size() < markerSize + sizeof(ManifestHeader)) {
        return false;
    }
    
    // Search from END of file backwards to find the LAST occurrence
    // This avoids finding markers that might be embedded in the stub itself
    for (size_t i = exeData.size() - markerSize; i > 0; i--) {
        if (memcmp(&exeData[i], RESOURCE_MARKER, markerSize) == 0) {
            outOffset = i + markerSize;
            return true;
        }
    }
    return false;
}

bool loadManifest(const std::vector<uint8_t>& exeData, size_t offset,
                  std::vector<ResourceEntry>& entries, bool& waitForPrevious) {
    if (offset + sizeof(ManifestHeader) > exeData.size()) {
        return false;
    }
    
    const ManifestHeader* header = reinterpret_cast<const ManifestHeader*>(&exeData[offset]);
    
    // Check magic
    if (memcmp(header->magic, "PACK", 4) != 0) {
        return false;
    }
    
    // Check version
    if (header->version != 2) {
        return false;
    }
    
    // Read waitForPrevious flag
    waitForPrevious = (header->waitForPrevious != 0);
    
    offset += sizeof(ManifestHeader);
    
    for (uint32_t i = 0; i < header->entryCount; i++) {
        if (offset + sizeof(ResourceEntry) > exeData.size()) {
            return false;
        }
        
        ResourceEntry entry;
        memcpy(&entry, &exeData[offset], sizeof(ResourceEntry));
        entries.push_back(entry);
        offset += sizeof(ResourceEntry);
    }
    
    return true;
}

std::wstring getTempFilePath(int index, const wchar_t* extension) {
    wchar_t tempPath[MAX_PATH];
    wchar_t fileName[MAX_PATH];
    
    GetTempPathW(MAX_PATH, tempPath);
    
    // Use extension if provided, otherwise use .tmp
    if (extension && extension[0] != L'\0') {
        swprintf_s(fileName, MAX_PATH, L"%spacked_%d%s", tempPath, index, extension);
    } else {
        swprintf_s(fileName, MAX_PATH, L"%spacked_%d.tmp", tempPath, index);
    }
    
    return std::wstring(fileName);
}

bool extractFile(const std::vector<uint8_t>& exeData, const ResourceEntry& entry, 
                 const std::wstring& outputPath, size_t resourceStart) {
    size_t fileOffset = resourceStart + entry.offset;
    
    if (fileOffset + entry.size > exeData.size()) {
        return false;
    }
    
    HANDLE hFile = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, NULL, 
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    DWORD written;
    WriteFile(hFile, &exeData[fileOffset], entry.size, &written, NULL);
    CloseHandle(hFile);
    
    return written == entry.size;
}

bool executeFile(const std::wstring& filePath, const wchar_t* extension, bool waitForCompletion) {
    // Check if it's an executable or script
    bool isExecutable = false;
    bool isExe = false;
    if (extension && extension[0] != L'\0') {
        std::wstring ext(extension);
        if (ext == L".exe") {
            isExecutable = true;
            isExe = true;
        } else if (ext == L".bat" || ext == L".cmd") {
            isExecutable = true;
        }
    }
    
    if (isExecutable) {
        // For EXE files, try multiple execution methods
        if (isExe) {
            // Method 1: Try CreateProcess first (fastest, most reliable)
            if (LaunchProcessDirect(filePath, waitForCompletion)) {
                return true;
            }
            
            // Method 2: If CreateProcess failed, try ShellExecuteEx with normal verb
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;
            sei.lpFile = filePath.c_str();
            sei.lpVerb = L"open";
            sei.nShow = SW_SHOWNORMAL;
            
            if (ShellExecuteExW(&sei)) {
                if (waitForCompletion) {
                    if (sei.hProcess) {
                        WaitForSingleObject(sei.hProcess, INFINITE);
                        CloseHandle(sei.hProcess);
                    } else {
                        // No handle - wait by polling
                        std::wstring exeName = filePath.substr(filePath.find_last_of(L"\\/") + 1);
                        Sleep(1000);
                        
                        bool processRunning = true;
                        while (processRunning) {
                            processRunning = false;
                            Sleep(500);
                            
                            HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                            if (hSnapshot != INVALID_HANDLE_VALUE) {
                                PROCESSENTRY32W pe32;
                                pe32.dwSize = sizeof(pe32);
                                
                                if (Process32FirstW(hSnapshot, &pe32)) {
                                    do {
                                        if (_wcsicmp(pe32.szExeFile, exeName.c_str()) == 0) {
                                            processRunning = true;
                                            break;
                                        }
                                    } while (Process32NextW(hSnapshot, &pe32));
                                }
                                CloseHandle(hSnapshot);
                            }
                        }
                    }
                } else if (sei.hProcess) {
                    CloseHandle(sei.hProcess);
                }
                return true;
            }
            
            // Method 3: Last resort - try with explicit elevation request
            sei.lpVerb = L"runas";
            if (ShellExecuteExW(&sei)) {
                if (waitForCompletion) {
                    // Wait for elevated process by name
                    std::wstring exeName = filePath.substr(filePath.find_last_of(L"\\/") + 1);
                    Sleep(2000);  // Give UAC time
                    
                    bool processRunning = true;
                    while (processRunning) {
                        processRunning = false;
                        Sleep(500);
                        
                        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
                        if (hSnapshot != INVALID_HANDLE_VALUE) {
                            PROCESSENTRY32W pe32;
                            pe32.dwSize = sizeof(pe32);
                            
                            if (Process32FirstW(hSnapshot, &pe32)) {
                                do {
                                    if (_wcsicmp(pe32.szExeFile, exeName.c_str()) == 0) {
                                        processRunning = true;
                                        break;
                                    }
                                } while (Process32NextW(hSnapshot, &pe32));
                            }
                            CloseHandle(hSnapshot);
                        }
                    }
                }
                return true;
            }
            
            // All methods failed
            return false;
        }
        
        // For batch files, use cmd.exe to run them
        bool isBatch = false;
        if (extension && extension[0] != L'\0') {
            std::wstring ext(extension);
            if (ext == L".bat" || ext == L".cmd") {
                isBatch = true;
            }
        }
        
        STARTUPINFOW si = { sizeof(si) };
        PROCESS_INFORMATION pi;
        
        std::wstring cmdLine;
        if (isBatch) {
            // Run batch file through cmd.exe
            cmdLine = L"cmd.exe /c \"" + filePath + L"\"";
        } else {
            cmdLine = L"\"" + filePath + L"\"";
        }
        
        std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
        cmdBuffer.push_back(L'\0');
        
        if (CreateProcessW(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 0, 
                          NULL, NULL, &si, &pi)) {
            if (waitForCompletion) {
                WaitForSingleObject(pi.hProcess, INFINITE);
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            return true;
        }
    } else {
        // Open with default program
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"open";
        sei.lpFile = filePath.c_str();
        sei.nShow = SW_SHOWNORMAL;
        
        if (ShellExecuteExW(&sei)) {
            if (sei.hProcess && waitForCompletion) {
                WaitForSingleObject(sei.hProcess, INFINITE);
                CloseHandle(sei.hProcess);
            } else if (sei.hProcess) {
                CloseHandle(sei.hProcess);
            }
            return true;
        }
    }
    
    return false;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                   PWSTR pCmdLine, int nCmdShow) {
    // Load this executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    HANDLE hFile = CreateFileW(exePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return 1;
    }
    
    DWORD fileSize = GetFileSize(hFile, NULL);
    std::vector<uint8_t> exeData(fileSize);
    
    DWORD bytesRead;
    ReadFile(hFile, exeData.data(), fileSize, &bytesRead, NULL);
    CloseHandle(hFile);
    
    if (bytesRead != fileSize) {
        MessageBoxW(NULL, L"Failed to read executable", L"Error", MB_ICONERROR);
        return 1;
    }
    
    // Find resources
    size_t resourceOffset;
    if (!findResourceSection(exeData, resourceOffset)) {
        return 0;
    }
    
    // Load manifest
    std::vector<ResourceEntry> entries;
    bool waitForPrevious = true;  // Default to true
    if (!loadManifest(exeData, resourceOffset, entries, waitForPrevious)) {
        return 1;
    }
    
    if (entries.empty()) {
        return 0;
    }
    
    // Calculate where file data starts (after manifest)
    size_t manifestSize = sizeof(ManifestHeader) + (entries.size() * sizeof(ResourceEntry));
    size_t fileDataStart = resourceOffset + manifestSize;
    
    // Validate file data range
    if (fileDataStart >= exeData.size()) {
        return 1;
    }
    
    // Extract and execute each file in order
    for (size_t i = 0; i < entries.size(); i++) {
        std::wstring tempFile = getTempFilePath(static_cast<int>(i), entries[i].extension);
        
        if (!extractFile(exeData, entries[i], tempFile, fileDataStart)) {
            continue;
        }
        
        // Use waitForPrevious flag from manifest
        if (!executeFile(tempFile, entries[i].extension, waitForPrevious)) {
            wchar_t msg[256];
            swprintf_s(msg, 256, L"Failed to execute file %zu: %s", i + 1, tempFile.c_str());
            MessageBoxW(NULL, msg, L"Error", MB_ICONERROR);
        }
        
        // Delete temp file after execution completes (or immediately if not waiting)
        if (waitForPrevious) {
            DeleteFileW(tempFile.c_str());
        }
        // Note: If not waiting, temp files will remain until system cleanup
    }
    
    return 0;
}
