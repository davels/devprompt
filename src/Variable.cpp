/**
 * @file Variable.cpp
 */

#include "Variable.hpp"
#include <array>
#include <cassert>
#include "Modules.hpp"

namespace devprompt
{

namespace
{

struct Variable
{
    std::string_view name;
    VariableFunction func = nullptr;
};

constexpr auto s_variables = std::to_array<Variable>({
#define VARIABLE(name, func) { #name, func },
#include "Variables.def"
#undef VARIABLE
});

// gets zero-initialized
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
