/**
 * @file Module.hpp
 */

#pragma once

#include <string>
#include <mutex>
#include <condition_variable>

namespace devprompt
{

class FileScanner;

enum class ModuleUsage
{
    Unused,
    Used,
    UsedExpensive
};

class ModuleContext
{
  public:
    ModuleContext(std::string path, const FileScanner& fscanner):
        m_path{std::move(path)}, m_filescanner{fscanner}
    {}

    const std::string& path() const
    { return m_path; }

    /// Get the FileScanner - this will block until the results are ready
    const FileScanner& filescanner() const
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait(lock, [this] { return m_ready; });
        return m_filescanner;
    }

    /// Signal that the FileScanner results are ready
    void set_filescanner_ready()
    {
        {
            std::lock_guard lock(m_mutex);
            m_ready = true;
        }
        m_cv.notify_all();
    }
    
  private:
    const std::string m_path;
    const FileScanner& m_filescanner;
    // filescanner can't be used until it's ready
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
    bool m_ready = false;
};

} // namespace devprompt
