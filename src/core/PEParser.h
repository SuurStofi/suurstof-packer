#ifndef PEPARSER_H
#define PEPARSER_H

#include "common.h"
#include <memory>

namespace Packer {

class PEParser {
public:
    PEParser();
    ~PEParser();
    
    // Load and parse PE file
    bool loadFile(const std::wstring& filePath, PEInfo& peInfo);
    
    // Validate PE file
    bool isValidPE(const std::vector<uint8_t>& data);
    
    // Get PE architecture (32 or 64 bit)
    bool is64BitPE(const std::vector<uint8_t>& data);
    
    // Extract resources from PE
    bool extractResources(const PEInfo& peInfo, std::vector<uint8_t>& resources);
    
    // Get section information
    bool getSections(const PEInfo& peInfo, std::vector<IMAGE_SECTION_HEADER>& sections);
    
    // Get import table
    bool getImports(const PEInfo& peInfo, std::vector<std::string>& imports);
    
private:
    IMAGE_DOS_HEADER* getDOSHeader(const std::vector<uint8_t>& data);
    IMAGE_NT_HEADERS* getNTHeaders(const std::vector<uint8_t>& data);
    
    bool validateHeaders(const std::vector<uint8_t>& data);
};

} // namespace Packer

#endif // PEPARSER_H
