/**
 * @file CoreVariables.hpp
 */

#pragma once

#include <string>

namespace devprompt
{


struct Core
{
    // Operations
    static void set_timeout_reached();

    // Variables
    static void timeout(std::string& out);
};

} // namespace devprompt
