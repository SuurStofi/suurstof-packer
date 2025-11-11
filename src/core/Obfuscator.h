#ifndef OBFUSCATOR_H
#define OBFUSCATOR_H

#include "common.h"

namespace Packer {

class Obfuscator {
public:
    Obfuscator();
    ~Obfuscator();
    
    // Obfuscate PE file
    bool obfuscate(PEInfo& peInfo, const ObfuscationOptions& options);
    
    // String encryption
    bool encryptStrings(std::vector<uint8_t>& data);
    
    // Control flow obfuscation
    bool obfuscateControlFlow(std::vector<uint8_t>& data);
    
    // Import table obfuscation
    bool obfuscateImports(std::vector<uint8_t>& data);
    
    // Add anti-debugging techniques
    bool addAntiDebug(std::vector<uint8_t>& data);
    
    // Add junk code
    bool addJunkCode(std::vector<uint8_t>& data);
    
private:
    // XOR encryption for strings
    void xorEncrypt(uint8_t* data, size_t size, uint8_t key);
    
    // Generate random bytes
    void generateRandomBytes(uint8_t* buffer, size_t size);
    
    // Find code sections
    bool findCodeSections(const std::vector<uint8_t>& data, 
                         std::vector<std::pair<size_t, size_t>>& sections);
};

} // namespace Packer

#endif // OBFUSCATOR_H
