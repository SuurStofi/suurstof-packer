#ifndef FILETYPEDETECTOR_H
#define FILETYPEDETECTOR_H

#include "../core/common.h"
#include <algorithm>
#include <cctype>

namespace Packer {

class FileTypeDetector {
public:
    static FileType detectFileType(const std::wstring& filePath) {
        std::wstring extension = getExtension(filePath);
        
        // Convert to lowercase for comparison
        std::transform(extension.begin(), extension.end(), extension.begin(),
                      [](wchar_t c) { return std::tolower(c); });
        
        // Executables
        if (extension == L"exe" || extension == L"com") {
            return FileType::EXECUTABLE;
        }
        
        // Scripts
        if (extension == L"bat" || extension == L"cmd" || 
            extension == L"ps1" || extension == L"vbs" || 
            extension == L"js" || extension == L"py") {
            return FileType::SCRIPT;
        }
        
        // Documents
        if (extension == L"txt" || extension == L"pdf" || extension == L"doc" || 
            extension == L"docx" || extension == L"rtf" || extension == L"odt") {
            return FileType::DOCUMENT;
        }
        
        // Archives
        if (extension == L"zip" || extension == L"rar" || extension == L"7z" || 
            extension == L"tar" || extension == L"gz") {
            return FileType::ARCHIVE;
        }
        
        // Images
        if (extension == L"jpg" || extension == L"jpeg" || extension == L"png" || 
            extension == L"bmp" || extension == L"gif" || extension == L"ico") {
            return FileType::IMAGE;
        }
        
        return FileType::OTHER;
    }
    
    static std::wstring getExtension(const std::wstring& filePath) {
        size_t dotPos = filePath.find_last_of(L'.');
        if (dotPos == std::wstring::npos) {
            return L"";
        }
        return filePath.substr(dotPos + 1);
    }
    
    static std::wstring getFileTypeString(FileType type) {
        switch (type) {
            case FileType::EXECUTABLE: return L"Executable";
            case FileType::SCRIPT:     return L"Script";
            case FileType::DOCUMENT:   return L"Document";
            case FileType::ARCHIVE:    return L"Archive";
            case FileType::IMAGE:      return L"Image";
            case FileType::OTHER:      return L"Other";
            default:                   return L"Unknown";
        }
    }
};

} // namespace Packer

#endif // FILETYPEDETECTOR_H
