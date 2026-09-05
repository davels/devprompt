/**
 * @file runner.hpp
 */

#pragma once

#include <string>
#include "Expected.hpp"

namespace devprompt
{

struct Options;

Expected<void,std::string> run(Options& options);

} // namespace devprompt

