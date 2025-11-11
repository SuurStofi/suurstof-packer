#include "ResourceEmbedder.h"
#include <algorithm>

#ifdef USE_ZLIB
#include <zlib.h>
#endif

namespace Packer {

ResourceEmbedder::ResourceEmbedder() {
}

ResourceEmbedder::~ResourceEmbedder() {
}

bool ResourceEmbedder::embedExecutables(const std::vector<PEInfo>& exeFiles, 
                                       std::vector<uint8_t>& outputData,
                                       bool waitForPrevious) {
    // Create resource data FIRST (this populates m_entries)
    std::vector<uint8_t> resourceData;
    if (!createResourceSection(exeFiles, resourceData)) {
        return false;
    }
    
    // Generate manifest AFTER (uses m_entries populated above)
    std::vector<uint8_t> manifest;
    if (!generateManifest(exeFiles, manifest, waitForPrevious)) {
        return false;
    }
    
    // Combine manifest and resources
    outputData.insert(outputData.end(), manifest.begin(), manifest.end());
    outputData.insert(outputData.end(), resourceData.begin(), resourceData.end());
    
    return true;
}

bool ResourceEmbedder::createResourceSection(const std::vector<PEInfo>& exeFiles,
                                             std::vector<uint8_t>& resourceData) {
    m_entries.clear();
    
    DWORD currentOffset = 0;
    DWORD resourceId = 100; // Start from resource ID 100
    
    for (const auto& exeFile : exeFiles) {
        ResourceEntry entry;
        entry.id = resourceId++;
        entry.offset = currentOffset;
        entry.originalSize = static_cast<DWORD>(exeFile.fileSize);
        entry.executionOrder = exeFile.executionOrder;
        
        // Store file extension
        std::wstring ext = L"." + exeFile.extension;
        wcsncpy_s(entry.extension, 8, ext.c_str(), _TRUNCATE);
        
        // Don't compress - stub doesn't have decompression code yet
        std::vector<uint8_t> data = exeFile.fileData;
        entry.compressed = false;
        entry.size = entry.originalSize;
        
        // Add to resource data
        resourceData.insert(resourceData.end(), data.begin(), data.end());
        
        currentOffset += entry.size;
        m_entries.push_back(entry);
    }
    
    return true;
}

bool ResourceEmbedder::compressData(const std::vector<uint8_t>& input,
                                   std::vector<uint8_t>& output) {
    if (input.empty()) {
        return false;
    }
    
#ifdef USE_ZLIB
    // Calculate maximum compressed size
    uLongf compressedSize = compressBound(static_cast<uLong>(input.size()));
    output.resize(compressedSize);
    
    // Compress with zlib
    int result = compress2(
        output.data(),
        &compressedSize,
        input.data(),
        static_cast<uLong>(input.size()),
        Z_BEST_COMPRESSION  // Maximum compression
    );
    
    if (result != Z_OK) {
        return false;
    }
    
    // Resize to actual compressed size
    output.resize(compressedSize);
    
    // Only use compression if it actually reduces size
    if (output.size() >= input.size()) {
        output = input;
        return false;  // Not compressed
    }
    
    return true;  // Successfully compressed
#else
    // Simple RLE compression fallback when zlib is not available
    output.clear();
    output.reserve(input.size());
    
    size_t i = 0;
    while (i < input.size()) {
        uint8_t value = input[i];
        size_t count = 1;
        
        // Count consecutive identical bytes (max 255)
        while (i + count < input.size() && input[i + count] == value && count < 255) {
            count++;
        }
        
        if (count > 3) {
            // Use RLE encoding: 0xFF (marker) + count + value
            output.push_back(0xFF);
            output.push_back(static_cast<uint8_t>(count));
            output.push_back(value);
        } else {
            // Just copy the bytes
            for (size_t j = 0; j < count; j++) {
                output.push_back(value);
            }
        }
        
        i += count;
    }
    
    // Only use compression if it actually reduces size
    if (output.size() >= input.size()) {
        output = input;
        return false;  // Not compressed
    }
    
    return true;  // Successfully compressed
#endif
}bool ResourceEmbedder::generateManifest(const std::vector<PEInfo>& exeFiles,
                                       std::vector<uint8_t>& manifest,
                                       bool waitForPrevious) {
    // Manifest structure:
    // - Magic number (4 bytes): "PACK"
    // - Version (4 bytes)
    // - Entry count (4 bytes)
    // - Wait for previous (1 byte)
    // - For each entry:
    //   - Resource ID (4 bytes)
    //   - Offset (4 bytes)
    //   - Size (4 bytes)
    //   - Original size (4 bytes)
    //   - Compressed flag (1 byte)
    //   - Execution order (4 bytes)
    //   - File extension (16 bytes - 8 wchar_t)
    
    const char magic[4] = {'P', 'A', 'C', 'K'};
    DWORD version = 2;  // Incremented for new format with extensions
    DWORD entryCount = static_cast<DWORD>(m_entries.size());
    
    // Write magic
    manifest.insert(manifest.end(), magic, magic + 4);
    
    // Write version
    const uint8_t* versionBytes = reinterpret_cast<const uint8_t*>(&version);
    manifest.insert(manifest.end(), versionBytes, versionBytes + sizeof(DWORD));
    
    // Write entry count
    const uint8_t* countBytes = reinterpret_cast<const uint8_t*>(&entryCount);
    manifest.insert(manifest.end(), countBytes, countBytes + sizeof(DWORD));
    
    // Write waitForPrevious flag
    uint8_t waitFlag = waitForPrevious ? 1 : 0;
    manifest.push_back(waitFlag);
    
    // Write entries
    for (const auto& entry : m_entries) {
        const uint8_t* idBytes = reinterpret_cast<const uint8_t*>(&entry.id);
        manifest.insert(manifest.end(), idBytes, idBytes + sizeof(DWORD));
        
        const uint8_t* offsetBytes = reinterpret_cast<const uint8_t*>(&entry.offset);
        manifest.insert(manifest.end(), offsetBytes, offsetBytes + sizeof(DWORD));
        
        const uint8_t* sizeBytes = reinterpret_cast<const uint8_t*>(&entry.size);
        manifest.insert(manifest.end(), sizeBytes, sizeBytes + sizeof(DWORD));
        
        const uint8_t* origSizeBytes = reinterpret_cast<const uint8_t*>(&entry.originalSize);
        manifest.insert(manifest.end(), origSizeBytes, origSizeBytes + sizeof(DWORD));
        
        uint8_t compressedFlag = entry.compressed ? 1 : 0;
        manifest.push_back(compressedFlag);
        
        const uint8_t* orderBytes = reinterpret_cast<const uint8_t*>(&entry.executionOrder);
        manifest.insert(manifest.end(), orderBytes, orderBytes + sizeof(DWORD));
        
        // Write file extension (16 bytes)
        const uint8_t* extBytes = reinterpret_cast<const uint8_t*>(&entry.extension);
        manifest.insert(manifest.end(), extBytes, extBytes + 16);
    }
    
    return true;
}

} // namespace Packer
