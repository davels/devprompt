/**
 * @file VcsFormatter.cpp
 *
 * "\uE0A0" branch
 * "\u2261" identical branch
 * "\u2191" ahead
 * "\u2193" behind
 * "\u2262" branch gone
 * "\uF417" commit
 * "\uF412" tag
 * "\uE728" rebase
 * "\uE29B" cherry-pick
 * "\uF0E2" revert
 * "\U000F0095" no commits
 */

#include "VcsFormatter.hpp"
#include "RepoStatus.hpp"
#include "Options.hpp"
#include "AnsiColor.hpp"

namespace devprompt::VcsModule
{

// Basic, starship-style formatting: branch name followed by symbols.
std::string format_status(const RepoStatus& status, const Options& options)
{
    std::string out;
    out.reserve(64);

    //# vcs icon and 'remote' host icon
    ansi::append_style(options.Vcs.style, options.output, out);
    out += options.Vcs.icon;
    out += status.host_icon;
    
    //# Branch name
    if (options.Vcs.show_branch)
    {
        if (status.detached) out +=  '@';
        out += status.branch;
    }

    //# Worktree icon
    if (options.Vcs.show_worktree_icon && status.isworktree)
        out += " ";  // 󱘎   󰔱 󰐅 󰹩
    
    //# In Progress Operation
    if (options.Vcs.show_state && !status.op_in_progress.empty())
    {
        out += " (";
        out += status.op_in_progress;
        out += ")";
    }

    //# Flags
    if (options.Vcs.show_status)
    {
        std::string flags;
        auto collector = [&flags, showcount=options.Vcs.show_counts] (int count, char ch)
        {
            if (count > 0)
            {
                if (showcount && !flags.empty())
                    flags += ' ';
                flags += ch;
                if (showcount)
                    flags += std::to_string(count);
            }
        };
        collector(status.conflicts, '!');
        collector(status.unstaged,  '*');
        collector(status.staged,    '+');
        collector(status.untracked, '?');  
        collector(status.stashed,   '$');  // 
    
        if (!flags.empty())
        {
            out += ' ';
            out += flags;
        }
    }
    
    //# Ahead / Behind
    if (options.Vcs.show_diverged && (status.ahead > 0 || status.behind > 0))
    {
        out += ' ';
        if (status.ahead  > 0)
            out += "↑" + std::to_string(status.ahead);   // ↑ ⇡
        if (status.behind > 0)
            out += "↓" + std::to_string(status.behind);  // ↓ ⇣
    }

    return out;
}

} // namespace devprompt::VcsModule
