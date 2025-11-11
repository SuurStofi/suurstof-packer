#include "PEParser.h"
#include <fstream>
#include <algorithm>

namespace Packer {

PEParser::PEParser() {
}

PEParser::~PEParser() {
}

bool PEParser::loadFile(const std::wstring& filePath, PEInfo& peInfo) {
    // Open file
    std::ifstream file(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return false;
    }
    
    // Get file size
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // Read file data
    peInfo.fileData.resize(size);
    if (!file.read(reinterpret_cast<char*>(peInfo.fileData.data()), size)) {
        return false;
    }
    
    peInfo.filePath = filePath;
    peInfo.fileSize = size;
    
    // Validate PE
    if (!isValidPE(peInfo.fileData)) {
        return false;
    }
    
    // Get architecture
    peInfo.is64Bit = is64BitPE(peInfo.fileData);
    
    // Get entry point and image base
    auto ntHeaders = getNTHeaders(peInfo.fileData);
    if (ntHeaders) {
        peInfo.entryPoint = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        peInfo.imageBase = static_cast<DWORD>(ntHeaders->OptionalHeader.ImageBase);
    }
    
    // Extract filename
    size_t lastSlash = filePath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        peInfo.originalName = filePath.substr(lastSlash + 1);
    } else {
        peInfo.originalName = filePath;
    }
    
    return true;
}

bool PEParser::isValidPE(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) {
        return false;
    }
    
    auto dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());
    
    // Check DOS signature
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }
    
    // Check NT headers offset
    if (static_cast<size_t>(dosHeader->e_lfanew) >= data.size() - sizeof(IMAGE_NT_HEADERS)) {
        return false;
    }
    
    auto ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(data.data() + dosHeader->e_lfanew);
    
    // Check PE signature
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }
    
    return true;
}

bool PEParser::is64BitPE(const std::vector<uint8_t>& data) {
    auto ntHeaders = getNTHeaders(data);
    if (!ntHeaders) {
        return false;
    }
    
    return ntHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
}

bool PEParser::extractResources(const PEInfo& peInfo, std::vector<uint8_t>& resources) {
    // TODO: Implement resource extraction
    return true;
}

bool PEParser::getSections(const PEInfo& peInfo, std::vector<IMAGE_SECTION_HEADER>& sections) {
    auto ntHeaders = getNTHeaders(peInfo.fileData);
    if (!ntHeaders) {
        return false;
    }
    
    auto sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
    
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++) {
        sections.push_back(sectionHeader[i]);
    }
    
    return true;
}

bool PEParser::getImports(const PEInfo& peInfo, std::vector<std::string>& imports) {
    // TODO: Implement import table parsing
    return true;
}

IMAGE_DOS_HEADER* PEParser::getDOSHeader(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(IMAGE_DOS_HEADER)) {
        return nullptr;
    }
    return reinterpret_cast<IMAGE_DOS_HEADER*>(const_cast<uint8_t*>(data.data()));
}

IMAGE_NT_HEADERS* PEParser::getNTHeaders(const std::vector<uint8_t>& data) {
    auto dosHeader = getDOSHeader(data);
    if (!dosHeader || dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        return nullptr;
    }
    
    if (static_cast<size_t>(dosHeader->e_lfanew) >= data.size() - sizeof(IMAGE_NT_HEADERS)) {
        return nullptr;
    }
    
    return reinterpret_cast<IMAGE_NT_HEADERS*>(
        const_cast<uint8_t*>(data.data()) + dosHeader->e_lfanew
    );
}

bool PEParser::validateHeaders(const std::vector<uint8_t>& data) {
    return isValidPE(data);
}

} // namespace Packer
