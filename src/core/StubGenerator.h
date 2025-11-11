#ifndef STUBGENERATOR_H
#define STUBGENERATOR_H

#include "common.h"

namespace Packer {

class StubGenerator {
public:
    StubGenerator();
    ~StubGenerator();
    
    // Generate complete packed executable
    bool generatePackedExecutable(const std::vector<PEInfo>& exeFiles,
                                  const PackerOptions& options,
                                  std::vector<uint8_t>& output);
    
    // Load stub template
    bool loadStubTemplate(std::vector<uint8_t>& stubData);
    
    // Append resources to stub
    bool appendResources(std::vector<uint8_t>& stubData,
                        const std::vector<uint8_t>& resources);
    
    // Update PE headers
    bool updatePEHeaders(std::vector<uint8_t>& peData,
                        size_t resourceOffset,
                        size_t resourceSize);
    
    // Create minimal stub fallback
    bool createMinimalStub(std::vector<uint8_t>& stubData);
    
private:
    std::vector<uint8_t> m_stubTemplate;
};

} // namespace Packer

#endif // STUBGENERATOR_H
