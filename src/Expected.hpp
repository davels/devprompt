/**
 * @file Expected.hpp
 * 
 * use this until we move to c++23 for std::expected
 */

#pragma once

#include <variant>
#include <utility>
#include <type_traits>

namespace devprompt
{

template <typename E>
struct Unexpected
{
    E error;
    constexpr explicit Unexpected(E err) : error(std::move(err)) {}
};

template <typename T, typename E>
class Expected
{
public:
    using value_type = T;
    using error_type = E;

    // Construct with a successful value
    constexpr Expected(const T& value) : m_storage(std::in_place_index<0>, value) {}
    constexpr Expected(T&& value) : m_storage(std::in_place_index<0>, std::move(value)) {}
    
    // Construct with an unexpected error state
    template <typename G>
    requires std::is_constructible_v<E, G>
    constexpr Expected(Unexpected<G> unexpected) : m_storage(std::in_place_index<1>, std::move(unexpected.error)) {}

    // Observers
    constexpr bool has_value() const noexcept { return m_storage.index() == 0; }
    constexpr explicit operator bool() const noexcept { return has_value(); }

    // Value access
    constexpr const T& value() const&
    {
        if (!has_value()) throw std::bad_variant_access();
        return std::get<0>(m_storage);
    }

    constexpr T& value() &
    {
        if (!has_value()) throw std::bad_variant_access();
        return std::get<0>(m_storage);
    }

    constexpr T&& value() &&
    {
        if (!has_value()) throw std::bad_variant_access();
        return std::get<0>(std::move(m_storage));
    }

    template <typename U>
    constexpr T value_or(U&& default_value) const&
    {
        return has_value() ? std::get<0>(m_storage) : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    constexpr T value_or(U&& default_value) &&
    {
        return has_value() ? std::get<0>(std::move(m_storage)) : static_cast<T>(std::forward<U>(default_value));
    }
    
    // Direct pointer-like access operators
    constexpr const T* operator->() const noexcept   { return &std::get<0>(m_storage); }
    constexpr T* operator->() noexcept               { return &std::get<0>(m_storage); }
    constexpr const T& operator*() const& noexcept   { return std::get<0>(m_storage); }
    constexpr T& operator*() & noexcept              { return std::get<0>(m_storage); }
    constexpr const T&& operator*() const&& noexcept { return std::get<0>(std::move(m_storage)); }
    constexpr T&& operator*() && noexcept            { return std::get<0>(std::move(m_storage)); }
    
    // Error access
    constexpr const E& error() const&
    {
        if (has_value()) throw std::bad_variant_access();
        return std::get<1>(m_storage);
    }

    constexpr E& error() &
    {
        if (has_value()) throw std::bad_variant_access();
        return std::get<1>(m_storage);
    }

    constexpr E&& error() &&
    {
        if (has_value()) throw std::bad_variant_access();
        return std::get<1>(std::move(m_storage));
    } 

private:
    std::variant<T, E> m_storage;
};


// Expected specialization for void T
template <typename E>
class Expected<void, E>
{
public:
    using value_type = void;
    using error_type = E;

    // Construct with a successful state
    constexpr Expected() noexcept
        : m_storage(std::monostate{})
    {
    }

    // Construct with an unexpected error state
    template <typename G>
    requires std::is_constructible_v<E, G>
    constexpr Expected(Unexpected<G> unexpected)
        : m_storage(std::move(unexpected.error))
    {
    }

    // Observers
    constexpr bool has_value() const noexcept
    {
        return std::holds_alternative<std::monostate>(m_storage);
    }

    constexpr explicit operator bool() const noexcept
    {
        return has_value();
    }

    // Value access
    constexpr void value() const
    {
        if (!has_value())
            throw std::bad_variant_access();
    }

    // Error access
    constexpr const E& error() const&
    {
        if (has_value())
            throw std::bad_variant_access();

        return std::get<E>(m_storage);
    }

    constexpr E& error() &
    {
        if (has_value())
            throw std::bad_variant_access();

        return std::get<E>(m_storage);
    }

    constexpr E&& error() &&
    {
        if (has_value())
            throw std::bad_variant_access();

        return std::get<E>(std::move(m_storage));
    }

private:
    std::variant<std::monostate, E> m_storage;
};


} // namespace devprompt
