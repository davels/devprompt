/**
 * @file ArrayStack.hpp
 *
 * Basic stack api built on std::array
 */

#pragma once

#include <array>
#include <cstddef>
#include <utility>
#include <cassert>

namespace devprompt
{

template <typename T, std::size_t N>
class ArrayStack
{
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = typename std::array<T, N>::iterator;
    using const_iterator = typename std::array<T, N>::const_iterator;

    constexpr bool empty() const noexcept
    {
        return m_size == 0;
    }

    constexpr bool full() const noexcept
    {
        return m_size == N;
    }

    constexpr size_type size() const noexcept
    {
        return m_size;
    }

    constexpr void push(const T& value)
    {
        assert(!full());
        if (full()) return;
        m_data[m_size++] = value;
    }

    constexpr void push(T&& value)
    {
        assert(!full());
        if (full()) return;
        m_data[m_size++] = std::move(value);
    }

    template <typename... Args>
    constexpr T& emplace(Args&&... args)
    {
        assert(!full());
        return m_data[m_size++] = T(std::forward<Args>(args)...);
    }
    
    constexpr void pop()
    {
        assert(!empty());
        --m_size;
    }

    constexpr T& top() noexcept
    {
        assert(!empty());
        return m_data[m_size - 1];
    }

    constexpr const T& top() const noexcept
    {
        assert(!empty());        
        return m_data[m_size - 1];
    }

    constexpr iterator begin() noexcept
    {
        return m_data.begin();
    }

    constexpr const_iterator begin() const noexcept
    {
        return m_data.begin();
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return m_data.cbegin();
    }

    constexpr iterator end() noexcept
    {
        return m_data.begin() + m_size;
    }

    constexpr const_iterator end() const noexcept
    {
        return m_data.begin() + m_size;
    }

    constexpr const_iterator cend() const noexcept
    {
        return m_data.cbegin() + m_size;
    }

private:
    std::array<T, N> m_data{};
    size_type m_size = 0;
};

} // namespace devprompt
