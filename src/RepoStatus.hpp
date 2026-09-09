/**
 * @file RepoStatus.hpp
 */

#pragma once

#include <string>
#include "Expected.hpp"

namespace devprompt::vcs
{

struct RepoStatus
{
    std::string branch;       // branch name, short OID if detached, revision#, etc
    bool detached = false;

    int unstaged    = 0;      // changes in the working tree, tracked files
    int staged      = 0;      // changes in the index (git status "index")
    int untracked   = 0;      // untracked files
    int conflicts   = 0;      // files with conflicts
    int stashed     = 0;      // stashed files
    bool isworktree = false;  // is this a worktree checkout
    
    int ahead       = 0;      // local is ahead of upstream
    int behind      = 0;      // local is behind upstream

    
    std::string state;        // MERGE / REBASE / CHERRY_PICK / etc, empty if normal
    std::string remote_icon;  // icon for the 'remote' host
};

struct RepoQueryVars
{
    bool remote_icon = false;
    bool branch      = false;
    bool state       = false;
    bool wt_icon     = false;
    bool status      = false;
    bool divergence  = false;
};

enum class RepoError
{
    NotRepository,
    CorruptRepository,
    AccessDenied,
    Error  // generic catch-all error
};

using RepoQueryResult = Expected<RepoStatus, RepoError>;

} //namespace devprompt::vcs
