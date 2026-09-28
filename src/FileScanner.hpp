/**
 * @file FileScanner.hpp
 */

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <stop_token>
#include <chrono>
#include <cstddef>
#include <cassert>

namespace devprompt
{

class FileScanner
{
  public:
    FileScanner() { m_patterns.reserve(20); }

    //// Add a filename to the pattern list and return its Id
    unsigned int add_filename(std::string fname);

    /// Add a file extension to the pattern list and return its Id
    /// (do not include the leading '.')
    unsigned int add_extension(std::string ext);

    /// Count of added patterns
    std::size_t count() const;

    /// Given a pattern Id, return true if it was found in the last directory scan
    bool check_pattern(unsigned int index) const;

    /// Scan a directory for matches to registered patterns
    void scan_directory(const std::string& path, std::stop_token stoken, bool timing=false);

    /// Elapsed time of previous directory scan
    auto elapsed() const { return m_elapsed; }
    
  private:
    enum class PatternType
    { Name, Extension };
    
    struct Pattern
    {
        std::string pattern;
        PatternType ptype;
        bool found = false;
    };
    std::vector<Pattern> m_patterns;
    std::chrono::steady_clock::duration m_elapsed{};
    
    void track_filename(std::string_view fname);
}; // FileScanner


inline std::size_t FileScanner::count() const
{
    return m_patterns.size();
}

inline bool FileScanner::check_pattern(unsigned int index) const
{
    assert(index < m_patterns.size());
    return m_patterns[index].found;
}

} // namespace devprompt
