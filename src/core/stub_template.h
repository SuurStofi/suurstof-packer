#ifndef STUB_TEMPLATE_H
#define STUB_TEMPLATE_H

#include <cstdint>
#include <vector>

namespace Packer {

// Stub template removed - will load from disk instead
inline std::vector<uint8_t> getStubTemplate() {
    return std::vector<uint8_t>(); // Return empty - force loading from file
}

} // namespace Packer

#endif // STUB_TEMPLATE_H
