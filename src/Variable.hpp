/**
 * @file Variable.hpp
 */

#pragma once

#include <string>
#include <string_view>

namespace devprompt
{

using VariableFunction = void (*)(std::string&);

enum class VariableId: int
{
#define VARIABLE(module, name, func) module##_##name,
#include "Variables.def"
#undef VARIABLE
};

VariableFunction LookupVariable(std::string_view vname);

bool IsVariableInUse(VariableId var);

} // namespace devprompt
