/**
 * @file CommandContext.hpp
 */

#pragma once

#include <atomic>
#include <mutex>
#include "ShellCommand.hpp"

namespace devprompt
{

class ShellCommandBase;

class CommandContext
{
  public:
    bool set(ShellCommandBase& cmd);
    void clear();
    void kill();
    bool killed() const noexcept;

    class ScopedSetter
    {
      public:
        ScopedSetter(CommandContext& cctx, ShellCommandBase& cmd):
            m_set{cctx.set(cmd)}, m_cctx{cctx} {}
        ~ScopedSetter() noexcept { m_cctx.clear(); }

        // delete copy/move
        ScopedSetter(const ScopedSetter&) = delete;
        ScopedSetter& operator=(const ScopedSetter&) = delete; 
        ScopedSetter(ScopedSetter&&) = delete;                 
        ScopedSetter& operator=(ScopedSetter&&) = delete;    

        explicit operator bool() const noexcept { return m_set; }
        void clear() { m_cctx.clear(); }

      private:
        bool m_set;
        CommandContext& m_cctx;
    };

    [[nodiscard]]  // copy elision, still works without copy or move
    ScopedSetter scoped_set(ShellCommandBase& cmd)
    { return ScopedSetter(*this, cmd); }

  private:
    std::mutex m_mtx;
    ShellCommandBase* m_cmd = nullptr;
    std::atomic<bool> m_killed{false};
}; // CommandContext

inline bool CommandContext::set(ShellCommandBase& cmd)
{
    std::lock_guard lock(m_mtx);

    if (m_killed.load(std::memory_order_relaxed))
        return false;
    
    m_cmd = &cmd;
    return true;
}

inline void CommandContext::clear()
{
    std::lock_guard lock(m_mtx);
    m_cmd = nullptr;
}

inline void CommandContext::kill()
{
    std::lock_guard lock(m_mtx);
    
    m_killed.store(true, std::memory_order_relaxed);
    if (m_cmd != nullptr)
    {
        // force kill the process
        m_cmd->kill();
        m_cmd = nullptr;
    }
}

inline bool CommandContext::killed() const noexcept
{
    return m_killed.load(std::memory_order_relaxed);
}

} // namespace devprompt
