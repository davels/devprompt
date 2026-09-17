/**
 * @file filesystem.cpp
 */

#include "filesystem.hpp"

#if defined(_WIN32) || defined (_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#else
    #include <cstdlib>
    #include <sys/stat.h>
    #include <climits>   // PATH_MAX
    #include <pwd.h>
    #include <unistd.h>
    #include <sys/types.h>
#endif


namespace devprompt::filesystem
{

//
// Windows
//

#ifdef _WIN32

namespace
{
// Windows needs utf8 <=> utf16 conversion functions for api calls

inline std::wstring Utf8ToWide(const std::string& s)
{
    if (s.empty()) return std::wstring();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    if (len <= 0) return {};  // conversion failed
    std::wstring ws(static_cast<size_t>(len), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), ws.data(), len);
    return ws;
}

inline std::string WideToUtf8(const std::wstring& ws)
{
    if (ws.empty()) return std::string();
    int len = ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()),
                                    nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};  // conversion failed
    std::string s(static_cast<size_t>(len), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), static_cast<int>(ws.size()),
                          s.data(), len, nullptr, nullptr);
    return s;
}

std::string get_home_directory()
{
    wchar_t buf[MAX_PATH];
    DWORD len = ::GetEnvironmentVariableW(L"USERPROFILE", buf, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        return WideToUtf8(std::wstring(buf, len));
    }
    wchar_t drive[MAX_PATH];
    wchar_t hpath[MAX_PATH];
    DWORD dlen = ::GetEnvironmentVariableW(L"HOMEDRIVE", drive, MAX_PATH);
    DWORD plen = ::GetEnvironmentVariableW(L"HOMEPATH", hpath, MAX_PATH);
    if (dlen > 0 && plen > 0)
    {
        return WideToUtf8(std::wstring(drive, dlen) + std::wstring(hpath, plen));
    }
    return {};  // home not found
}

std::string expand_home(const std::string& path)
{
    if (path.empty() || path[0] != '~') return path;

    if (path.size() == 1)
    {
        return get_home_directory();
    }
    if (path[1] == '/' || path[1] == '\\')
    {
        return get_home_directory() + path.substr(1);
    }    
}
} // namespace

bool path_exists(const std::string& path)
{
    DWORD dwAttrib = ::GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
}

bool is_dir(const std::string& path)
{
    DWORD dwAttrib = ::GetFileAttributesA(path.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

// the windows api has no function to resolve a relative path with symlinks
// we need have to open the file first to get a handle (from CreateFileW) then use that to
// get the full path.
std::string expand_path(const std::string& path, bool resolve_links=false)
{
    if (path.empty())
        return {};

    std::wstring wpath = Utf8ToWide(path);

    DWORD needed = ::GetFullPathNameW(wpath.c_str(), 0, nullptr, nullptr);
    if (needed == 0)
        return {};  // failed
      
    std::wstring fullPath(needed, L'\0');
    DWORD written = GetFullPathNameW(wpath.c_str(), needed, fullPath.data(), nullptr);
    if (written == 0 || written >= needed)
        return {};  // failed
    fullPath.resize(written);

    if (!resolve_links)
        return WideToUtf8(fullPath);
    
    HANDLE hFile = ::CreateFileW(
        fullPath.c_str(),
        0, // metadata only, no read/write access needed
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS, // required to open directories
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE)
        return {};  // path is not valid

    DWORD finalLen = GetFinalPathNameByHandleW(hFile, nullptr, 0, FILE_NAME_NORMALIZED);
    if (finalLen == 0)
    {
        CloseHaneld(hFile);
        return {};  // failed
    }
     
    std::wstring finalPath(finalLen, L'\0');
    DWORD finalWritten = GetFinalPathNameByHandleW(hFile, finalPath.data(), finalLen, FILE_NAME_NORMALIZED);
    CloseHandle(hFile);
    if (finalWritten == 0 || finalWritten >= finalLen)
        return {};  // failed
    finalPath.resize(finalWritten);

    // GetFinalPathNameByHandleW returns extended-length paths prefixed
    // with "\\?\" (or "\\?\UNC\" for network shares). Strip the prefix
    // for a "normal" looking path; drop this if you need >MAX_PATH
    // support and are fine with callers seeing the raw prefixed form.
    const std::wstring uncPrefix = L"\\\\?\\UNC\\";
    const std::wstring prefix    = L"\\\\?\\";
    if (finalPath.rfind(uncPrefix, 0) == 0)
    {
        finalPath = L"\\\\" + finalPath.substr(uncPrefix.size());
    }
    else if (finalPath.rfind(prefix, 0) == 0)
    {
        finalPath = finalPath.substr(prefix.size());
    }

    return WideToUtf8(finalPath);    
}

std::string get_current_directory()
{
    wchar_t buffer[MAX_PATH];
    DWORD len = ::GetCurrentDirectoryW(MAX_PATH, buffer);
    if (len <= 0)
        return {};  // failed
    return WideToUtf8(buffer);
}

//
// Linux/macOS
//

#else

namespace
{
std::string get_home_directory()
{
    if (const char* home = std::getenv("HOME"); home && *home)
    {
        return std::string(home);
    }
    if (struct passwd* pw = getpwuid(getuid()); pw && pw->pw_dir)
    {
        return std::string(pw->pw_dir);
    }
    // can't determine home directory
    return {};
}

// expand ~ and ~user
std::string expand_home(const std::string& path)
{
    if (path.empty() || path[0] != '~') return path;

    // ~
    if (path.size() == 1)
    {
        return get_home_directory();
    }
    if (path[1] == '/')
    {
        return get_home_directory() + path.substr(1);
    }

    // ~user
    size_t pos = path.find('/', 1);
    std::string username = path.substr(1, pos == std::string::npos
                                       ? std::string::npos
                                       : pos - 1);
    if (struct passwd* pw = ::getpwnam(username.c_str()); pw && pw->pw_dir)
    {
        std::string rest = (pos == std::string::npos) ? "" : path.substr(pos);
        return std::string(pw->pw_dir) + rest;
    }

    return path;
}
} // namespace


bool path_exists(const std::string& path)
{
    struct stat buffer;
    return (::stat(path.c_str(), &buffer) == 0);
}

bool is_dir(const std::string& path)
{
    struct stat buffer;
    if (::stat(path.c_str(), &buffer) == 0)
    {
        return (buffer.st_mode & S_IFDIR) != 0;
    }
    return false;
}

std::string expand_path(const std::string& path)
{
    if (path.empty())
        return {};

    char resolved[PATH_MAX];
    if (::realpath(expand_home(path).c_str(), resolved) == nullptr)
    {
        return {};  // path was not valid
    }
    return std::string(resolved);
}

std::string get_current_directory()
{
    const char* pwd = std::getenv("PWD");
    if (!pwd || *pwd == '\0')
        return {};
    return pwd;
}

#endif

} // namespace devprompt::filesystem

