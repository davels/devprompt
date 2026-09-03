/**
 * @file ShellCommand.hpp
 */

#pragma once

#include <array>
#include <memory>
#include <span>
#include <optional>
#include <string_view>
#include <cstddef>
#include "Expected.hpp"

namespace devprompt
{

class ShellCommand
{
  public:
    ShellCommand();
    ~ShellCommand();

    // delete copy
    ShellCommand(const ShellCommand&) = delete;
    ShellCommand& operator=(const ShellCommand&) = delete;
    
    // unique_ptr handles move - have to =default in cpp because of unique_ptr<Impl>
    ShellCommand(ShellCommand&&) noexcept;
    ShellCommand& operator=(ShellCommand&&) noexcept;

    // maximum number of argument allowed for start,run methods that take a span<>
    static constexpr std::size_t MaxArguments = 16;
    static constexpr int ERR_BUFFER_OVERFLOW = 99999;
    
    // start the command with the given args
    template <typename... Args>
    bool start(Args... args)
    {
        const char* command_line[] = {args..., nullptr};
        return start_impl(command_line);
    }

    // start the command with an arg list
    bool start(std::span<const char* const> args)
    {
        std::array<const char*, MaxArguments + 1> command_line{};
        if (args.size() > MaxArguments)
            return false;

        for (std::size_t i = 0; i < args.size(); ++i)
            command_line[i] = args[i];
        command_line[args.size()] = nullptr;
        return start_impl(command_line.data());
    }
    
    // read from stdout - returns # chars read, 0 when nothing left
    std::size_t read(std::span<char> buffer);

    // read one line from stdout. any part of the line longer than linebuf it ignored
    std::optional<std::span<char>> readline(std::span<char> buffer,
                                            std::span<char> linebuf);

    // read from stderr - returns # chars read, 0 when nothing left
    std::size_t stderr(std::span<char> buffer);
    
    // wait for the process to finish and return the exit code
    int wait();

    // run the command, read stdout (upto buffer size) and return it
    // return exit code on error
    template <typename... Args>
    Expected<std::string_view,int> run(std::span<char> buffer, Args... args)
    {
        const char* command_line[] = {args..., nullptr};
        return run_impl(buffer, command_line);
    }

    // run the command with an arg list, putting stdout into buffer
    // if buffer overflows it will return Unexpected(ERR_BUFFER_OVERFLOW)
    Expected<std::string_view,int> run(std::span<char> buffer, std::span<const char* const> args)
    {
        std::array<const char*, MaxArguments + 1> command_line{};
        if (args.size() > MaxArguments)
            return Unexpected(1);

        for (std::size_t i = 0; i < args.size(); ++i)
            command_line[i] = args[i];
        command_line[args.size()] = nullptr;
        
        return run_impl(buffer, command_line.data());
    }
    
  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    // _impl command_line arg requires a final NULL in the array
    bool start_impl(const char* const command_line[]);
    Expected<std::string_view,int> run_impl(std::span<char> buffer, const char* const command_line[]);
};


template <std::size_t N>
  requires (N > 0 && N <= ShellCommand::MaxArguments)
class CommandArgs
{
public:
    constexpr bool push_back(const char* arg)
    {
        if (m_size == N)
            return false;

        m_args[m_size++] = arg;
        return true;
    }

    constexpr std::span<const char* const> span() const
    {
        return {m_args.data(), m_size};
    }

    constexpr std::size_t size() const
    {
        return m_size;
    }

private:
    std::array<const char*, N> m_args{};
    std::size_t m_size = 0;
};

} // namespace devprompt
