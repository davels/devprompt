/**
 * @file CoreVariables.cpp
 */

#include "CoreVariables.hpp"

namespace devprompt
{

namespace
{
bool s_timeout_reached{false};
} // namespace


void Core::set_timeout_reached()
{
    s_timeout_reached = true;
}


void Core::timeout(std::string& out)
{
    if (s_timeout_reached)
        out += "";  // 󰀠 󱫌  󱑄 󰥕 󰗖   
}

} // namespace devprompt
