/**
 * @file FileReader.hpp
 */

#pragma once

#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>
#include <span>

namespace devprompt
{

/// Lightweight FILE wrapper
class FileReader
{
  public:
    FileReader() noexcept = default;
    explicit FileReader(const char* fname) noexcept { open(fname); }
    ~FileReader() { close(); }

    FileReader(const FileReader&) = delete;
    FileReader& operator=(const FileReader&) = delete;

    FileReader(FileReader&& other) noexcept;
    FileReader& operator=(FileReader&& other) noexcept;

    bool open(const char* fname) noexcept;
    void close() noexcept;

    [[nodiscard]] bool isopen() const noexcept;
    explicit operator bool() const { return isopen(); }

    [[nodiscard]] bool eof() const noexcept;
    [[nodiscard]] bool error() const noexcept;
    
    /// read up to buf.size() bytes. return number of bytes read
    std::size_t read(std::span<std::byte> buf) noexcept;

    /// read overloads for spans of byte convertable types (like char, uint, etc.)
    /// returns *bytes* read, not elements
    template <class T, std::size_t N>
      requires (!std::is_const_v<T> && std::is_trivially_copyable_v<T> && N > 0)
    std::size_t read(std::span<T, N> buf) noexcept
    { return read(std::as_writable_bytes(buf)); }

    /// Read one line into the provided string
    bool readline(std::string& line);
    
  private:
    std::FILE* m_file = nullptr;
};


inline FileReader::FileReader(FileReader&& other) noexcept:
    m_file(std::exchange(other.m_file, nullptr))
{}

inline FileReader& FileReader::operator=(FileReader&& other) noexcept
{
    if (this != &other)
    {
        close();
        m_file = std::exchange(other.m_file, nullptr);
    }
    return *this;
}

inline bool FileReader::open(const char* fname) noexcept
{
    close();
    m_file = std::fopen(fname, "rb");
    return m_file != nullptr;
}

inline void FileReader::close() noexcept
{
    if (m_file)
    {
        std::fclose(m_file);
        m_file = nullptr;
    }
}

[[nodiscard]] inline bool FileReader::isopen() const noexcept 
{
    return m_file != nullptr;
}

[[nodiscard]] inline bool FileReader::eof() const noexcept
 { return m_file && std::feof(m_file); }

[[nodiscard]] inline bool FileReader::error() const noexcept
{ return m_file && std::ferror(m_file); }

inline std::size_t FileReader::read(std::span<std::byte> buf) noexcept
{
    return m_file? std::fread(buf.data(), 1, buf.size(), m_file) : 0;
}

inline bool FileReader::readline(std::string& line)
{
    line.clear();
    if (!m_file) return false;

    char buf[1024];
    bool gotdata = false;
    
    while (std::fgets(buf, static_cast<int>(sizeof buf), m_file))
    {
        gotdata = true;
        line += buf;
        if (!line.empty() && line.back() == '\n')
            break;
    }

    if (!gotdata)
        return false;    

    if (!line.empty() && line.back() == '\n')
        line.pop_back();
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    return true;
}

} // namespace devprompt
