/**
 * @file FileScanner.cpp
 */

#include "FileScanner.hpp"
#include <cassert>
#include "utils.hpp"

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dirent.h>
#endif

namespace devprompt
{

unsigned int FileScanner::add_filename(std::string fname)
{
    assert(!fname.empty());
    std::size_t i = 0;
    for (const auto& pat: m_patterns)
    {
        if (pat.ptype == PatternType::Name && pat.pattern == fname)
            return i;
        ++i;
    }
    m_patterns.emplace_back(std::move(fname), PatternType::Name);
    return i;
}

unsigned int FileScanner::add_extension(std::string ext)
{
    assert(!ext.empty());
    assert(ext.front() != '.');
    std::size_t i = 0;
    for (const auto& pat: m_patterns)
    {
        if (pat.ptype == PatternType::Extension && pat.pattern == ext)
            return i;
        ++i;
    }
    m_patterns.emplace_back(std::move(ext), PatternType::Extension);
    return i;
}

void FileScanner::scan_directory(const std::string& path, std::stop_token stoken, bool timing/*=false*/)
{
    auto start = timing?
        std::chrono::steady_clock::now():
        std::chrono::steady_clock::time_point{};
    
#ifdef _WIN32
    std::wstring wpath = utils::Utf8ToWide(path);
    if (wpath.back() != L'/' && wpath.back() != L'\\')
        wpath += L'/';
    wpath += L'*';

    WIN32_FIND_DATAW fdata;
    HANDLE h = ::FindFirstFileW(wpath,
                                FineExInfoBasic,
                                &fdata,
                                FindExSearchNameMatch,
                                nullptr,
                                FIND_FIRST_EX_LARGE_FETCH);
    if (h != INVALID_HANDLE_VALUE)
    {
        std::size_t i = 0;
        do
        {
            // periodically check for a stop request
            if (++i % 16 == 0 && stoken.stop_requested())
                break;
            if (data.cFileName[0] == L'.' &&
                (data.cFileName[1] == L'\0' || (data.cFileName[1] == L'.' && data.cFileName[2] == L'\0')))
                continue;
            std::string fname = utils::WideToUtf8(data.cFileName);
            track_filename(fname);
        } while (::FindNextFileW(h, &fdata));
        
        ::FindClose(h);
    }
#else
    DIR* dir = ::opendir(path.c_str());
    if (!dir) return;

    std::size_t i = 0;
    while (auto* entry = ::readdir(dir))
    {
        // periodically check for a stop request
        if (++i % 16 == 0 && stoken.stop_requested())
            break;
        std::string_view name{entry->d_name};
        if (name == "." || name == "..") continue;
        track_filename(name);
    }
    ::closedir(dir);
#endif

    if (timing)
        m_elapsed = std::chrono::steady_clock::now() - start;
}

void FileScanner::track_filename(std::string_view fname)
{
    std::string_view ext;
    auto pos = fname.rfind('.');
    if (pos != std::string_view::npos)
    {
        // file extension, excluding the '.'
        ext = fname.substr(pos+1);
    }

    // fname could match both a filename and extension pattern so we need to check them all
    for (auto& pat: m_patterns)
    {
        if (pat.ptype == PatternType::Name && pat.pattern == fname)
            pat.found = true;
        else if(pat.ptype == PatternType::Extension && pat.pattern == ext)
            pat.found = true;
    }
}

} // namespace devprompt
