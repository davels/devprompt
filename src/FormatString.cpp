/**
 * @file FormatString.cpp
 *
 * variable: %variable%
 *   output the variable
 * conditional: [ ... ]
 *   only ouptut the contents if at least one enclosed %variable% generates output
 * query conditional:  [?%variable% ... ]
 *   only output the contents if the specified variable has a value
 * style: {fg:red bool} | {/} for reset
 *   output control codes to set style
 */

#include "FormatString.hpp"
#include <string_view>
#include <span>
#include <algorithm>
#include <optional>
#include <cstddef>
#include <cassert>
#include "ArrayStack.hpp"
#include "IterCursor.hpp"
#include "styleparser.hpp"

namespace devprompt
{

namespace
{

// maximum number of nested [conditional] expressions
static constexpr int MAX_CONDITIONAL_DEPTH = 8;

// fast identification of special characters using lookup table
static constexpr std::array<bool, 256> SPECIAL_CHARS = [] {
    std::array<bool, 256> table{};
    for (unsigned char c : {'%', '[', ']', '{', '\\'}) table[c] = true;
    return table;
}();

/**
 * Simple cursor over a string_view
 * Call next() to return the first character and advance
 */
class Cursor
{
  public:
    Cursor(std::string_view str):
        m_view(str)
    {}

    static constexpr char EOS = 0;

    /// Return the current char and advance cursor to next
    inline char next()
    {
        if (m_view.empty())
            return EOS;
        auto ch = m_view.front();
        m_view.remove_prefix(1);
        return ch;
    }

    /// Pointer to the view buffer at the position of the last char return by next()
    /// UB if called before next()
    inline const char* prev_pos() const
    {
        return  m_view.data() - 1;
    }

    /// The char that next() would return
    inline char peek() const
    {
        return m_view.empty()? EOS: m_view.front();
    }

    inline bool atend() const
    {
        return m_view.empty();
    }
    
    std::optional<std::string_view> match(char ch)
    {
        if (m_view.empty()) return std::nullopt;

        std::size_t matchp = m_view.find(ch);
        if (matchp == std::string_view::npos)
        {
            // no match, advance to end
            m_view.remove_prefix(m_view.size());
            return std::nullopt;
        }
        
        std::string_view result = m_view.substr(0, matchp);
        m_view.remove_prefix(matchp+1); // advance to pos after match
        return result;
    }
    
  private:
    std::string_view m_view;
};  // Cursor

} // namespace


struct PendingString 
{
    const char* start = nullptr;
    std::size_t len = 0;

    constexpr bool empty() const { return len == 0; }
    constexpr void reset(const char* s, std::size_t l)
    {
        start = s;
        len = l;
    }
    constexpr void reset() 
    {
        start = nullptr;
        len = 0;
    }
};


inline void FormatString::add_pending_text_token(PendingString& textnode)
{
    if (!textnode.empty())
    {
        add_text_token({textnode.start, textnode.len});
        textnode.reset();
    }
}

Expected<void,std::string> FormatString::compile()
{
    int cond_depth = 0;
    PendingString textnode;  // in progress text node
    Cursor cur{m_format};
    m_tokens.reserve(32);  // avoid multiple allocations for typical format strings
    
    while (char ch = cur.next())
    {
        // plain text
        if (!SPECIAL_CHARS[static_cast<unsigned char>(ch)])
        {
            if (textnode.empty())
                textnode = {cur.prev_pos(), 1};
            else
                ++textnode.len;
            continue;
        }
        // special char
        switch (ch)
        {
        case '\\': {                            // escape char
            char next = cur.next();
            if (next == Cursor::EOS)
            {
                // no following char, treat the esc char as a literal
                if (textnode.empty())
                    textnode.reset(cur.prev_pos(), 1);
                else
                    ++textnode.len;
            }
            else
            {
                // otherwise add the escaped character as a new text token
                add_pending_text_token(textnode);
                if (next == 'n')    // \n is a special case: newline
                    add_newline_token();
                else                
                    textnode.reset(cur.prev_pos(), 1);
            }
            break;
        }
        case '%': {                               // variable
            add_pending_text_token(textnode);
            auto var = cur.match('%');
            if (!var)
                return Unexpected("missing close tag on variable");
            // lookup the variable function and tag it's use
            auto vfunc = LookupVariable(*var);
            if (!vfunc)
                return Unexpected("unknown variable '" + std::string{*var} + "'");
            assert(vfunc != nullptr);
            add_variable_token(vfunc);
            break;
        }
        case '[': {                               // conditional open
            add_pending_text_token(textnode);
            if (cond_depth == MAX_CONDITIONAL_DEPTH)
                return Unexpected("maximum conditional depth exceeded");
            ++cond_depth;
            if (cur.peek() == '?')                // query conditional variant
            {
                std::ignore = cur.next();  // skip over the '?'
                if (cur.next() != '%')
                    return Unexpected("query conditional must start with a %variable%");
                auto var = cur.match('%');
                if (!var)
                    return Unexpected("missing close tag on condition variable");
                auto vfunc = LookupVariable(*var);
                if (!vfunc)
                    return Unexpected("unknown variable '" + std::string{*var} + "'");
                assert(vfunc != nullptr);
                add_query_cond_open_token(vfunc);
            }
            else
            {
                add_cond_open_token();
            }
            break;
        }
        case ']': {                               // conditional close
            add_pending_text_token(textnode);
            if (cond_depth == 0)
                return Unexpected("mismatched conditional close");
            --cond_depth;
            add_cond_close_token();
            break;
        }
        case '{': {                               // style tag
            add_pending_text_token(textnode);
            auto stylestr = cur.match('}');
            if (!stylestr)
                return Unexpected("missing style close");
            // style strings have to be lower case - do in-place downcase
            // need a mutable span of the characters that stylestr points to
            std::span<char> chars {m_format.data() + (stylestr->data() - m_format.data()),
                                   stylestr->size()};
            for (auto& c: chars)
            {
                if (c >= 'A' && c <= 'Z')
                    c = static_cast<char>(ch + ('a' - 'A'));
            }                
            add_style_token(*stylestr);
            break;
        }
        default:
            assert(false);
        } //swtich
    }
    add_pending_text_token(textnode);
    return {};
}

void FormatString::expand_impl(const Palette* palette, ansi::Output output, std::string& out) const
{
    struct CondFrame
    {
        std::size_t start = 0;
        bool dirty = false;
    };
    ArrayStack<CondFrame, MAX_CONDITIONAL_DEPTH> cond_stack;

    // estimate required output size
    out.reserve(m_format.size() + 32);

    IterCursor cursor{m_tokens};
    while (cursor.valid())
    {
        const auto& token = *cursor;
        ++cursor;
        switch (token.type)
        {
        case TokenType::Literal:
            out += token.text;
            break;
        case TokenType::Newline:
            out += '\n';
            break;
        case TokenType::Variable: {
            assert(token.vfunc != nullptr);
            auto start = out.size();
            token.vfunc(out);
            if (out.size() > start)  // variable created output
            {
                std::ranges::for_each(cond_stack, [](auto& state) {
                    state.dirty = true;
                });
            }
            break;
        }
        case TokenType::CondOpen:
            assert(!cond_stack.full());
            cond_stack.emplace(out.size(), false);
            break;
        case TokenType::QueryCondOpen: {
            assert(!cond_stack.full());
            auto var = token.text;
            bool dirty = true;  // TODO get the var value T/F
            if (dirty)
            {
                cond_stack.emplace(out.size(), true);
            }
            else
            {
                // immediately skip everything to the close token
                // make sure to skip over other open/close pairs
                int depth = 1;
                while (cursor.valid() && depth > 0)
                {
                    const auto ttype = cursor->type;
                    ++cursor;
                    if (ttype == TokenType::CondOpen || ttype == TokenType::QueryCondOpen)
                    {
                        ++depth;
                    }
                    else if (ttype == TokenType::CondClose && --depth == 0)
                    {
                        break; // found close tag for the query conditional
                    }
                }
            }
            break;
            }
        case TokenType::CondClose: {
            assert(!cond_stack.empty());
            auto& cond = cond_stack.top();
            if (!cond.dirty)
            {
                // no variables written out, roll back the output
                out.resize(cond.start);
            }
            cond_stack.pop();
            break;
        }
        case TokenType::Style: {
            auto style = parse_style(palette, token.text);
            if (!style)
            {
                out += "{ERROR:";
                out += style.error();
                out += "}";
            }
            else
                style->write(output, out);
            break;
        }
        } // switch
    }
}

} // namespace devprompt

