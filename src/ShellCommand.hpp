/**
 * @file ShellCommand.hpp
 */

#pragma once

#include <array>
#include <memory>
#include <span>
#include <optional>
#include <string_view>
#include <concepts>
#include <cstddef>
#include "Expected.hpp"

namespace devprompt
{

template <typename T>
concept NullTerminatedString =
    std::is_convertible_v<T, const char*> ||
    std::same_as<std::remove_cvref_t<T>, std::string>;


class ShellCommandBase
{
  public:
    // note: constructor is protected
    ~ShellCommandBase();
    
    // no copy/move
    ShellCommandBase(const ShellCommandBase&) = delete;
    ShellCommandBase& operator=(const ShellCommandBase&) = delete;
    ShellCommandBase(ShellCommandBase&&) = delete;
    ShellCommandBase& operator=(ShellCommandBase&&) = delete;

    // maximum number of arguments allowed for start and run methods that take a span<>
    static constexpr std::size_t MaxArguments = 16;
    static constexpr int ERR_BUFFER_OVERFLOW = 99999;
    
    /// start a command with the given args. follow with read/readline
    template <typename... Args>
      requires (NullTerminatedString<Args>&& ...)
    bool start(Args... args)
    {
        const char* command_line[] = {to_c_str(args)..., nullptr};
        return start_impl(command_line);
    }

    /// start the command with an arg list
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
    
    /// read from stdout - returns empty span when nothing left
    std::span<char> read();

    /// read one line from stdout. any part of the line longer than linebuf is dropped
    std::optional<std::string_view> readline(std::span<char> linebuf);

    /// read from stderr into user buffer - returns # chars read, 0 when nothing left
    std::span<char> stderr();
    
    /// wait for the process to finish and return the exit code
    int wait();

    /// kill the process
    void kill();

    /// run the command, read stdout (upto buffer size) and return it as a string_view.
    /// return exit code on error
    template <typename... Args>
      requires (NullTerminatedString<Args>&& ...)
    Expected<std::string_view,int> run(Args... args)
    {
        const char* command_line[] = {to_c_str(args)..., nullptr};
        return run_impl(command_line);
    }

    /// run the command with an arg list. return string view of the results.
    /// if buffer overflows it will return Unexpected(ERR_BUFFER_OVERFLOW)
    Expected<std::string_view, int> run(std::span<const char* const> args)
    {
        std::array<const char*, MaxArguments + 1> command_line;
        if (args.size() > MaxArguments)
            return Unexpected(1);

        for (std::size_t i = 0; i < args.size(); ++i)
            command_line[i] = args[i];
        command_line[args.size()] = nullptr;
        
        return run_impl(command_line.data());
    }
    
  protected:
    explicit ShellCommandBase(std::span<char> buf);
    
  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    template <typename T>
    static const char* to_c_str(T&& value)
    {
        if constexpr (std::same_as<std::remove_cvref_t<T>, std::string>)
            return value.c_str();
        else
            return value;
    }
    
    // x_impl command_line arg requires a final NULL in the array
    bool start_impl(const char* const command_line[]);
    std::size_t read_into(std::span<char> buffer);
    Expected<std::string_view,int> run_impl(const char* const command_line[]);
};


template <size_t N=4096>
  requires (N > 0)
class ShellCommand: public ShellCommandBase
{
  public:
    ShellCommand(): ShellCommandBase(std::span<char>(m_buffer_)) {}
                    
  private:
    std::array<char, N> m_buffer_;
};


template <std::size_t N>
  requires (N > 0 && N <= ShellCommandBase::MaxArguments)
class CommandArgs
{
public:
    constexpr bool push(const char* arg)
    {
        if (m_size == N)
            return false;

        m_args[m_size++] = arg;
        return true;
    }

    constexpr std::span<const char* const> args() const
    { return {m_args.data(), m_size}; }

    constexpr std::size_t size() const
    { return m_size; }

private:
    std::array<const char*, N> m_args{};
    std::size_t m_size = 0;
};

} // namespace devprompt
