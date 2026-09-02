/**
 * @file filesystem.cpp
 */

#include "filesystem.hpp"

namespace devprompt
{

//
// Windows
//

#if defined(_WIN32) || defined (_WIN64)

#include <windows.h>

bool path_exists(const std::string& path)
{
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

bool is_dir(const std::string& path)
{
    DWORD dwAttrib = GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

//
// Linux/macOS
//

#else

#include <sys/stat.h>

bool path_exists(const std::string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool is_dir(const std::string& path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0)
    {
        return (buffer.st_mode & S_IFDIR) != 0;
    }
    return false;
}

#endif
} // devprompt
