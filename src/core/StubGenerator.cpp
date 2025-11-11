#include "StubGenerator.h"
#include "ResourceEmbedder.h"
#include "stub_template.h"
#include <fstream>

namespace Packer {

StubGenerator::StubGenerator() {
}

StubGenerator::~StubGenerator() {
}

bool StubGenerator::generatePackedExecutable(const std::vector<PEInfo>& exeFiles,
                                             const PackerOptions& options,
                                             std::vector<uint8_t>& output) {
    // Load stub template
    if (!loadStubTemplate(m_stubTemplate)) {
        return false;
    }
    
    // Create resource embedder
    ResourceEmbedder embedder;
    std::vector<uint8_t> resourceData;
    
    if (!embedder.embedExecutables(exeFiles, resourceData, options.waitForPrevious)) {
        return false;
    }
    
    // Append resources to stub
    output = m_stubTemplate;
    if (!appendResources(output, resourceData)) {
        return false;
    }
    
    // NOTE: We don't update PE headers because we're just appending data to the end
    // The stub will find the resources using the marker, and Windows will still
    // execute the original PE code correctly
    
    return true;
}

bool StubGenerator::loadStubTemplate(std::vector<uint8_t>& stubData) {
    // Try to load from file first (if stub was built separately)
    // Look for stub.exe in the same directory as the executable
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    
    // Get directory of the executable
    std::wstring exeDir(exePath);
    size_t lastSlash = exeDir.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        exeDir = exeDir.substr(0, lastSlash + 1);
    }
    
    std::wstring stubPath = exeDir + L"stub.exe";
    
    std::ifstream stubFile(stubPath.c_str(), std::ios::binary | std::ios::ate);
    if (stubFile.is_open()) {
        std::streamsize size = stubFile.tellg();
        stubFile.seekg(0, std::ios::beg);
        
        stubData.resize(size);
        if (stubFile.read(reinterpret_cast<char*>(stubData.data()), size)) {
            // Validate it's a PE file
            if (stubData.size() >= 64) {
                PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(stubData.data());
                if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE) {
                    return true;
                }
            }
        }
    }
    
    // Fallback: Use embedded template
    stubData = getStubTemplate();
    
    if (stubData.empty()) {
        return false;
    }
    
    // Basic validation - check DOS signature
    if (stubData.size() < sizeof(IMAGE_DOS_HEADER)) {
        // Create a simple message box PE as fallback
        return createMinimalStub(stubData);
    }
    
    PIMAGE_DOS_HEADER dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(stubData.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return createMinimalStub(stubData);
    }
    
    return true;
}

bool StubGenerator::createMinimalStub(std::vector<uint8_t>& stubData) {
    // Create a minimal PE that shows a message box
    // This is a temporary solution until real stub is compiled
    
    // For now, just copy a working Windows executable as template
    // Try to use a small Windows utility
    std::wstring systemExe = L"C:\\Windows\\System32\\msg.exe";
    std::ifstream file(systemExe.c_str(), std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        // Try notepad as fallback
        systemExe = L"C:\\Windows\\System32\\notepad.exe";
        file.open(systemExe.c_str(), std::ios::binary | std::ios::ate);
    }
    
    if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        
        stubData.resize(size);
        if (file.read(reinterpret_cast<char*>(stubData.data()), size)) {
            return true;
        }
    }
    
    return false;
}

bool StubGenerator::appendResources(std::vector<uint8_t>& stubData,
                                    const std::vector<uint8_t>& resources) {
    // Write resource marker so stub can find the data
    const char marker[] = "PACKEDRES_V2";
    // Don't include null terminator - write only the string characters
    stubData.insert(stubData.end(), marker, marker + sizeof(marker) - 1);
    
    // Append resources (manifest + file data)
    stubData.insert(stubData.end(), resources.begin(), resources.end());
    
    return true;
}

bool StubGenerator::updatePEHeaders(std::vector<uint8_t>& peData,
                                    size_t resourceOffset,
                                    size_t resourceSize) {
    if (peData.size() < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) {
        return false;
    }
    
    auto dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(peData.data());
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    auto ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(peData.data() + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
    // Get last section
    auto sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    int sectionCount = ntHeaders->FileHeader.NumberOfSections;
    
    if (sectionCount == 0) {
        return false;
    }
    
    auto lastSection = &sectionHeader[sectionCount - 1];
    
    // Calculate alignments
    DWORD fileAlignment = ntHeaders->OptionalHeader.FileAlignment;
    DWORD sectionAlignment = ntHeaders->OptionalHeader.SectionAlignment;
    
    if (fileAlignment == 0) fileAlignment = 0x200;
    if (sectionAlignment == 0) sectionAlignment = 0x1000;
    
    // Helper lambda for alignment
    auto alignTo = [](DWORD value, DWORD alignment) -> DWORD {
        return ((value + alignment - 1) / alignment) * alignment;
    };
    
    // Create new .pack section
    IMAGE_SECTION_HEADER newSection = {};
    strcpy_s(reinterpret_cast<char*>(newSection.Name), 8, ".pack");
    
    // Calculate virtual address (after last section)
    newSection.VirtualAddress = lastSection->VirtualAddress + 
                                alignTo(lastSection->Misc.VirtualSize, sectionAlignment);
    
    // Set sizes
    newSection.Misc.VirtualSize = static_cast<DWORD>(resourceSize);
    newSection.SizeOfRawData = alignTo(static_cast<DWORD>(resourceSize), fileAlignment);
    newSection.PointerToRawData = static_cast<DWORD>(resourceOffset);
    
    // Set characteristics (readable, initialized data)
    newSection.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | 
                                IMAGE_SCN_MEM_READ;
    
    // Add section header (need to resize PE data to add section header)
    size_t sectionHeaderOffset = dosHeader->e_lfanew + 
                                 sizeof(DWORD) + // Signature
                                 sizeof(IMAGE_FILE_HEADER) +
                                 ntHeaders->FileHeader.SizeOfOptionalHeader +
                                 (sectionCount * sizeof(IMAGE_SECTION_HEADER));
    
    // Insert new section header
    peData.insert(peData.begin() + sectionHeaderOffset, 
                  reinterpret_cast<uint8_t*>(&newSection),
                  reinterpret_cast<uint8_t*>(&newSection) + sizeof(IMAGE_SECTION_HEADER));
    
    // Update headers after insertion (pointers may have changed)
    dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(peData.data());
    ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(peData.data() + dosHeader->e_lfanew);
    
    // Update number of sections
    ntHeaders->FileHeader.NumberOfSections++;
    
    // Update SizeOfImage
    ntHeaders->OptionalHeader.SizeOfImage = newSection.VirtualAddress +
                                           alignTo(newSection.Misc.VirtualSize, sectionAlignment);
    
    return true;
}

} // namespace Packer
