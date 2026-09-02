/**
 * @file IterCursor.hpp
 */

#pragma once

#include <memory>

template <typename Container>
class IterCursor
{
    using Iterator = typename Container::const_iterator;

    Iterator m_current;
    Iterator m_end;

public:
    explicit IterCursor(const Container& container):
        m_current(container.begin()), m_end(container.end())
    {}

    bool valid() const
    {
        return m_current != m_end;
    }

    const auto& operator*() const
    {
        return *m_current;
    }

    const auto* operator->() const
    {
        return std::addressof(*m_current);
    }

    IterCursor& operator++()
    {
        ++m_current;
        return *this;
    }
};
