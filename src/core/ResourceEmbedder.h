#ifndef RESOURCEEMBEDDER_H
#define RESOURCEEMBEDDER_H

#include "common.h"

namespace Packer {

class ResourceEmbedder {
public:
    ResourceEmbedder();
    ~ResourceEmbedder();
    
    // Embed multiple EXEs as resources
    bool embedExecutables(const std::vector<PEInfo>& exeFiles, 
                         std::vector<uint8_t>& outputData,
                         bool waitForPrevious = true);
    
    // Create resource section
    bool createResourceSection(const std::vector<PEInfo>& exeFiles,
                              std::vector<uint8_t>& resourceData);
    
    // Compress data
    bool compressData(const std::vector<uint8_t>& input, 
                     std::vector<uint8_t>& output);
    
    // Generate resource manifest
    bool generateManifest(const std::vector<PEInfo>& exeFiles,
                         std::vector<uint8_t>& manifest,
                         bool waitForPrevious);
    
private:
    struct ResourceEntry {
        DWORD id;
        DWORD offset;
        DWORD size;
        DWORD originalSize;
        bool compressed;
        int executionOrder;
        wchar_t extension[8];  // Store file extension (e.g., ".exe", ".txt", ".bat")
    };
    
    std::vector<ResourceEntry> m_entries;
};

} // namespace Packer

#endif // RESOURCEEMBEDDER_H
