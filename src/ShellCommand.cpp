/**
 * @file ShellCommand.cpp
 */

#include "ShellCommand.hpp"
#include <cstdio>
#include <cstddef>
#include "subprocess.h"

namespace devprompt
{

struct ShellCommand::Impl
{
    struct subprocess_s subprocess{};
    std::span<const char> pending;  // partial data in buffer between readline calls
};

ShellCommand::ShellCommand():
    m_impl(std::make_unique<Impl>())
{}

ShellCommand::~ShellCommand()
{
    if (m_impl)
        subprocess_destroy(&m_impl->subprocess);
}

// Move operations defined here where Impl is a complete type
ShellCommand::ShellCommand(ShellCommand&&) noexcept = default;
ShellCommand& ShellCommand::operator=(ShellCommand&&) noexcept = default;

std::size_t ShellCommand::read(std::span<char> buffer)
{
    auto* pout = subprocess_stdout(&m_impl->subprocess);
    return std::fread(buffer.data(), 1, buffer.size(), pout);
}

std::size_t ShellCommand::stderr(std::span<char> buffer)
{
    auto* pout = subprocess_stderr(&m_impl->subprocess);
    return std::fread(buffer.data(), 1, buffer.size(), pout);
}

std::optional<std::span<char>> ShellCommand::readline(std::span<char> buffer,
                                                      std::span<char> linebuf,
                                                      bool ignore_overflow/*=false*/)
{
    std::size_t line_size = 0;

    for (;;)
    {
        if (m_impl->pending.empty())
        {
            const auto len = read(buffer);

            if (len == 0)  //EOF
            {
                if (line_size == 0)
                    return std::nullopt;  // no pending data
                // EOF reached mid-line (no trailing \n) - return what remains
                if (linebuf[line_size - 1] == '\r')
                    --line_size;
                return linebuf.first(line_size);                
            }
            m_impl->pending = {buffer.data(), len};
        }

        while (!m_impl->pending.empty())
        {
            const char ch = m_impl->pending.front();
            m_impl->pending = m_impl->pending.subspan(1);

            if (ch == '\n')
            {
                if (line_size > 0 && linebuf[line_size - 1] == '\r')
                    --line_size;
                return linebuf.first(line_size);
            }

            if (line_size < linebuf.size())
                linebuf[line_size++] = ch;
        }
    }
}

int ShellCommand::wait()
{
    int exitcode = -1;
    subprocess_join(&m_impl->subprocess, &exitcode);
    return exitcode;
}

bool ShellCommand::start_impl(const char* const command_line[])
{
    subprocess_destroy(&m_impl->subprocess);

    int res = subprocess_create(command_line,
                                subprocess_option_inherit_environment
                                | subprocess_option_search_user_path
                                | subprocess_option_no_window,
                                &m_impl->subprocess);
    return res == 0;
}

Expected<std::string_view,int> ShellCommand::run_impl(std::span<char> buffer, const char* const command_line[])
{
    if (!start_impl(command_line))
        return Unexpected(1);

    // keep reading until buffer fills or read run out of data
    std::size_t fillsize = 0;  // amount written to buffer
    while (fillsize < buffer.size())
    {
        const auto count = read(std::span(buffer).subspan(fillsize));
        if (count == 0)
            break;  // read has nothing left
        fillsize += count;
    }
    
    // Buffer filled -- peek for one more byte to check if there would be more
    if (fillsize == buffer.size())
    {
        char probe[1];
        if (read(probe) != 0)
        {
            std::ignore = wait();
            return Unexpected(ERR_BUFFER_OVERFLOW);
        }        
    }

    // return view of the filled portion of buffer
    std::string_view result{buffer.data(), fillsize};
    auto exitcode = wait();
    if (exitcode != 0)
        return Unexpected(exitcode);
    return result;
}

} // namespace devprompt
