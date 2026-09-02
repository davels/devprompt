/**
 * @file Variable.hpp
 */

#pragma once

#include <string>
#include <string_view>

namespace devprompt
{

using VariableFunction = void (*)(std::string&);

enum class VariableId
{
#define VARIABLE(name, func) name,
#include "Variables.def"
#undef VARIABLE

    _Count
};

VariableFunction LookupVariable(std::string_view vname);

bool IsVariableInUse(VariableId var);

} // namespace devprompt
