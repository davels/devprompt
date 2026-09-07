/**
 * @file ShellCommand.cpp
 */

#include "ShellCommand.hpp"
#include <cstdio>
#include "subprocess.h"

namespace devprompt
{

struct ShellCommandBase::Impl
{
    Impl(std::span<char> buf): buffer(buf) {}
    struct subprocess_s subprocess{};
    std::span<char> buffer;
    std::span<const char> pending;  // partial data in buffer between readline calls
};

ShellCommandBase::ShellCommandBase(std::span<char> buf):
    m_impl(std::make_unique<Impl>(buf))
{}

ShellCommandBase::~ShellCommandBase()
{
    if (m_impl)
        subprocess_destroy(&m_impl->subprocess);
}

std::span<char> ShellCommandBase::read()
{
    auto* pout = subprocess_stdout(&m_impl->subprocess);
    auto len = std::fread(m_impl->buffer.data(), sizeof(char), m_impl->buffer.size(), pout);
    return m_impl->buffer.first(len);
}

std::span<char> ShellCommandBase::stderr()
{
    auto* pout = subprocess_stderr(&m_impl->subprocess);
    auto len = std::fread(m_impl->buffer.data(), sizeof(char), m_impl->buffer.size(), pout);
    return m_impl->buffer.first(len);
}

std::optional<std::string_view> ShellCommandBase::readline(std::span<char> linebuf)
{
    std::size_t line_size = 0;

    for (;;)
    {
        if (m_impl->pending.empty())
        {
            const auto len = read_into(m_impl->buffer);

            if (len == 0)  //EOF
            {
                if (line_size == 0)
                    return std::nullopt;  // no pending data
                // EOF reached mid-line (no trailing \n) - return what remains
                if (linebuf[line_size - 1] == '\r')
                    --line_size;
                return std::string_view{linebuf.data(), line_size};
            }
            m_impl->pending = m_impl->buffer.first(len);
        }

        while (!m_impl->pending.empty())
        {
            const char ch = m_impl->pending.front();
            m_impl->pending = m_impl->pending.subspan(1);

            if (ch == '\n')
            {
                if (line_size > 0 && linebuf[line_size - 1] == '\r')
                    --line_size;
                return std::string_view{linebuf.data(), line_size};
            }

            if (line_size < linebuf.size())
                linebuf[line_size++] = ch;
        }
    }
}

int ShellCommandBase::wait()
{
    int exitcode = -1;
    subprocess_join(&m_impl->subprocess, &exitcode);
    return exitcode;
}

bool ShellCommandBase::start_impl(const char* const command_line[])
{
    subprocess_destroy(&m_impl->subprocess);

    int res = subprocess_create(command_line,
                                subprocess_option_inherit_environment
                                | subprocess_option_search_user_path
                                | subprocess_option_no_window,
                                &m_impl->subprocess);
    return res == 0;
}

std::size_t ShellCommandBase::read_into(std::span<char> buffer)
{
    auto* pout = subprocess_stdout(&m_impl->subprocess);
    return std::fread(buffer.data(), sizeof(char), buffer.size(), pout);
}

Expected<std::string_view,int> ShellCommandBase::run_impl(const char* const command_line[])
{
    if (!start_impl(command_line))
        return Unexpected(1);

    // keep reading until buffer fills or read run out of data
    std::span<char> buffer = m_impl->buffer;
    while (!buffer.empty())
    {
        const auto count = read_into(buffer);
        if (count == 0)
            break;  // nothing left to read
        buffer = buffer.subspan(count); // remaing free space in buffer
    }

    // Buffer filled -- peek for one more byte to check if there would be more
    if (buffer.empty())
    {
        std::array<char, 1> probe;
        if (read_into(probe) != 0)
        {
            std::ignore = wait();
            return Unexpected(ERR_BUFFER_OVERFLOW);
        }        
    }

    // return view of the filled portion of buffer
    auto exitcode = wait();
    if (exitcode != 0)
        return Unexpected(exitcode);
    std::string_view result{m_impl->buffer.data(),
                            m_impl->buffer.size() - buffer.size()};
    return result;
}

} // namespace devprompt
