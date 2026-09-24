/**
 * @file Variable.cpp
 */

#include "Variable.hpp"
#include <array>
#include <cassert>
#include <string_view>
#include "ModuleInc.hpp"
#include "CoreVariables.hpp"

namespace devprompt
{

namespace
{

struct Variable
{
    std::string_view name;
    VariableFunction func = nullptr;
};

// Create an array of Variable objects for every definition in Variables.def
constexpr auto s_variables = std::to_array<Variable>({
#define VARIABLE(module, name, func) { #module "." #name, func },
#include "Variables.def"
#undef VARIABLE
});

// track which variables are referenced by LookupVariable (zero-initialized)
std::array<bool, s_variables.size()> s_inuse{};

} // namespace


VariableFunction LookupVariable(std::string_view vname)
{
    for (std::size_t i=0; const auto& var: s_variables)
    {
        if (var.name == vname)
        {
            s_inuse[i] = true;
            return var.func;
        }
        ++i;
    }
    return nullptr;
}

bool IsVariableInUse(VariableId var)
{
    auto i = static_cast<std::size_t>(var);
    assert(i < s_inuse.size());
    return s_inuse[i];
}

} // namespace devprompt
