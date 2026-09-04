/**
 * @file FormatString.hpp
 */

#pragma once

#include <string>
#include <vector>
#include "Expected.hpp"
#include "Variable.hpp"
#include "Style.hpp"
#include "Palette.hpp"

namespace devprompt
{

struct PendingString;
class Palette;

class FormatString
{
  public:
    FormatString(std::string format):
        m_format(std::move(format))
    {}
        
    void assign(std::string format)
    {
        m_format = std::move(format);
        m_tokens.clear();
    }
    
    Expected<void,std::string> compile();
    
    void expand(const Palette& palette, ansi::Output output, std::string& out) const
    { expand_impl(palette.empty()? nullptr: &palette, output, out); }
    
    void expand(ansi::Output output, std::string& out) const
    { expand_impl(nullptr, output, out); }
    
  private:
    enum class TokenType
    { Literal, Newline, Variable, CondOpen, QueryCondOpen, CondClose, Style };
    struct Token
    {
        TokenType type;
        union
        {
            std::string_view text;
            VariableFunction vfunc;
        };

        constexpr Token(TokenType type_, std::string_view text_): type{type_}, text{text_}
        {}

        constexpr Token(TokenType type_, VariableFunction vfunc_): type{type_}, vfunc{vfunc_}
        {}        
    };
    
    std::string m_format;
    std::vector<Token> m_tokens;

    void add_text_token(std::string_view text)
    { m_tokens.emplace_back(TokenType::Literal, text); }

    void add_newline_token()
    { m_tokens.emplace_back(TokenType::Newline, std::string_view{}); }
    
    void add_pending_text_token(PendingString& text);
    
    void add_variable_token(VariableFunction vfunc)
    { m_tokens.emplace_back(TokenType::Variable, vfunc); }
    
    void add_cond_open_token()
    { m_tokens.emplace_back(TokenType::CondOpen, std::string_view{}); }

    void add_query_cond_open_token(VariableFunction vfunc)
    { m_tokens.emplace_back(TokenType::QueryCondOpen, vfunc); }
    
    void add_cond_close_token()
    { m_tokens.emplace_back(TokenType::CondClose, std::string_view{}); }

    void add_style_token(std::string_view style)
    { m_tokens.emplace_back(TokenType::Style, style); }

    void expand_impl(const Palette* palette, ansi::Output output, std::string& out) const;
}; // FomatString

} // namespace devprompt



