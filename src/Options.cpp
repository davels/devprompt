/**
 * @file Options.cpp
 */

#include "Options.hpp"
#include <cstdlib>
#include <string>
#include <string_view>
#include <optional>

namespace devprompt
{

namespace
{

class ArgParser
{
  public:
    ArgParser(int argc, char** argv)
        : m_argc(argc), m_argv(argv), m_inext(1) // skip argv[0] (program name)
    {}

    std::optional<std::string_view> next()
    {
        if (m_inext >= m_argc)
        {
            return std::nullopt;
        }
        return std::string_view(m_argv[m_inext++]);
    }

    bool more() { return m_inext < m_argc; }
    void reset() { m_inext = 1; }

  private:
    int m_argc;
    char** m_argv;
    int m_inext;
};

// style parser requires that all style strings have lower case property names
std::string lowerstyle(std::string_view style)
{
    std::string result(style);
    for (char& c : result) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c + ('a' - 'A'));
    }

    return result;
}

} // namespace


Expected<Options, std::string> parse_options(int argc,  char** argv)
{
    Options options;
    bool have_format = false;
    
    ArgParser parser(argc, argv);
    while (auto parg = parser.next())
    {
        auto arg = *parg;
        // Everything after the first non-option argument is the path.
        if (!arg.starts_with("-"))
        {
            options.path = arg;
            break;
        }

        if (arg == "--format")
        {
            if (!parser.more())
                return Unexpected("Missing format string");
            options.format = *parser.next();
            have_format = true;
        }
        else if (arg == "--bash")
        {
            options.output = ansi::Output::Bash;
        }
        else if (arg == "--zsh")
        {
            options.output = ansi::Output::Zsh;
        }        
        else if (arg == "--git")
        {
            options.Vcs.git = true;;
        }
        else if (arg == "--svn")
        {
            options.Vcs.svn = true;;
        }
        else if (arg == "--vcs-counts")
        {
            options.Vcs.show_counts = true;
        }
        else if (arg == "--vcs-no-counts")
        {
            options.Vcs.show_counts = false;
        }
        else if (arg == "--vcs-untracked")
        {
            options.Vcs.show_untracked = true;
        }
        else if (arg == "--vcs-no-untracked")
        {
            options.Vcs.show_untracked = false;
        }
        else if (arg == "--vcs-stashed")
        {
            options.Vcs.show_stashed = true;
        }
        else if (arg == "--vcs-no-stashed")
        {
            options.Vcs.show_stashed = false;
        }
        else if (arg == "--vcs-commit-as-branch")
        {
            options.Vcs.commit_as_branch = true;
        }
        else if (arg == "--vcs-no-commit-as-branch")
        {
            options.Vcs.commit_as_branch = false;
        }
        else if (arg == "--vcs-remote-pattern")
        {
            if (!parser.more())
                return Unexpected("Missing value for --vcs-remote-pattern");
            auto iconinfo = *parser.next();
            auto pos = iconinfo.find(';');
            if (pos == std::string_view::npos)
                return Unexpected("Invalid mapping for --vcs-remote-pattern. Use 'pattern;icon'");
            options.Vcs.remoteicon_mapping.emplace_back(std::string{iconinfo.substr(0, pos)},
                                                        std::string{iconinfo.substr(pos + 1)});            
        }
        else if (arg == "--os-wsl-is-distro")
        {
            options.Os.wsl_is_distro = true;
        }
        else if (arg == "--os-no-wsl-is-distro")
        {
            options.Os.wsl_is_distro = false;
        }
        else if (arg == "--shell-error")
        {
            if (!parser.more())
                return Unexpected("Missing error value for --shell-error");
            options.Shell.error = *parser.next();
        }        
        else if (arg == "--timing")
        {
            options.show_timing = true;
        }
        else if (arg == "--version")
        {
            options.show_version = true;
        }
        else if (arg == "-h" || arg == "--help")
        {
            options.show_help = true;
        }
        else
        {
            std::string error {"unrecognized argument '"};
            error += arg;
            error += '\'';
            return Unexpected(std::move(error));
        }
    }

    // if no repo type restrictions are specified, enable all
    if (options.Vcs.git == false && options.Vcs.svn == false)
    {
        options.Vcs.git = options.Vcs.svn = true;
    }

    // if fotmat is not specified in args, check ENV
    if (!have_format)
    {
        auto format_env = std::getenv("DEVPROMPT_FORMAT");
        if (format_env)
            options.format = format_env;
    }
    return options;
}

} // namespace devprompt
