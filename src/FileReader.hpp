/**
 * @file FileReader.hpp
 */

#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <array>


/// Lightweight FILE wrapper for reading lines from a file
class FileReader
{
  public:
    explicit FileReader(const char* fname);
    ~FileReader();
    
    /// Is the file open
    explicit operator bool() const;

    /// Read one line into the provided string
    bool readline(std::string& line);

  private:
    std::FILE* m_file = nullptr;
    std::array<char,2048> m_buffer;
};


inline FileReader::FileReader(const char* fname)
    : m_file(std::fopen(fname, "rb"))
{}

inline FileReader::~FileReader()
{
    if (m_file)
        std::fclose(m_file);
}

inline FileReader::operator bool() const
{
    return m_file != nullptr;
}

inline bool FileReader::readline(std::string& line)
{
    line.clear();

    if (!std::fgets(m_buffer.data(), m_buffer.size(), m_file))
        return false;

    do
    {
        line += m_buffer.data();
        if (!line.empty() && line.back() == '\n')
            break;

    } while (std::fgets(m_buffer.data(), m_buffer.size(), m_file));

    if (!line.empty() && line.back() == '\n')
        line.pop_back();
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    return true;
}
