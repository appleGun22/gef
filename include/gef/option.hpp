#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <concepts>
#include <variant>

#include "unique_ref.hpp"

namespace gef {

    struct nullopt_t {};
    static constexpr nullopt_t nullopt{};

    template <typename T>
    class option;

    template <typename T>
    class OptionMethods {
    public:
        constexpr bool is_null(this auto&& self) noexcept { return !self.has_value(); }

        // return value or `other`
        constexpr T& value_or(this auto&& self, T&& other) noexcept {
            if (self.has_value()) {
                return self.value_unchecked();
            }

            return std::forward<T>(other);
        }

        // use the value, if it exists
        template <typename F>
            requires(requires(F&& f, T& v) { { f(v) } -> std::same_as<void>; })
        constexpr option<T>& use(this auto&& self, F&& f) noexcept {
            if (self.has_value()) {
                std::invoke(std::forward<F>(f), self.value_unchecked());
            }

            return self;
        }

        // inspect the value without modifying it, if it exists
        template <typename F>
            requires(requires(F&& f, T const& v) { { f(v) } -> std::same_as<void>; })
        constexpr auto& inspect(this auto&& self, F&& f) noexcept {
            if (self.has_value()) {
                std::invoke(std::forward<F>(f), self.value_unchecked());
            }

            return self;
        }

        // map. if has value, return an option from the result of `f`, otherwise return nullopt
        template <typename F, typename U = std::invoke_result_t<F, T>>
            requires(requires(F&& f, T const& v) { { f(v) } -> std::convertible_to<U>; })
        constexpr option<U> transform(this auto&& self, F&& f) noexcept {
            if (self.has_value()) {
                return option<U>{ std::invoke(std::forward<F>(f), self.value_unchecked()) };
            }

            return option<U>{};
        }

        // flatmap. if has value, return the result of `f` (f should return an option), otherwise return nullopt
        template <typename U, typename F>
            requires(requires(F&& f, T const& v) { { f(v) } -> std::convertible_to<option<U>>; })
        constexpr option<U> and_then(this auto&& self, F&& f) noexcept {
            if (self.has_value()) {
                return std::invoke(std::forward<F>(f), self.value_unchecked());
            }

            return option<U>{};
        }

        // return self if has value, otherwise return an option from `f` (an option of the same type)
        template <typename F>
            requires(requires(F&& f) { { f() } -> std::convertible_to<option<T>>; })
        constexpr option<T> or_else(this auto&& self, F&& f) noexcept {
            if (self.has_value()) {
                return self;
            }

            return std::invoke(std::forward<F>(f));
        }

        // if self has value run `f`, otherwise return provided value
        template <typename F, typename R>
            requires(requires(F&& f, T& v) { { f(v) } -> std::convertible_to<R>; })
        constexpr R map_or(this auto&& self, F&& f, R or_val) noexcept {
            if (self.has_value()) {
                return std::invoke(std::forward<F>(f), self.value_unchecked());
            }
            
            return or_val;
        }

        // run `f_val` if self has value, otherwise run `f_null` (both functions return any, but same, type)
        template <typename F_val, typename F_null, typename R = std::invoke_result_t<F_null>>
            requires(
                requires(F_val&& f, T& v) { { f(v) } -> std::same_as<R>; } &&
                requires(F_null&& f)      { { f() } -> std::same_as<R>; }
            )
        constexpr R map_or_else(this auto&& self, F_val&& f_val, F_null&& f_null) noexcept {
            if (self.has_value()) {
                return std::invoke(std::forward<F_val>(f_val), self.value_unchecked());
            }
            else {
                return std::invoke(std::forward<F_null>(f_null));
            }
        }
    };

    template <typename T>
    class option : public OptionMethods<T> {
    public:
        static_assert(std::same_as<T, std::remove_reference_t<T>>);// T should be an lvalue

        constexpr option() noexcept : m_empty(), m_is_some(false) {}

        constexpr option(option&) noexcept = default;
        constexpr option(option&&) noexcept = default;

        constexpr option(option const& other) noexcept {
            m_is_some = other.m_is_some;

            if (m_is_some) {
                new (&m_value) T(other.m_value);
            }
        }

        constexpr ~option() noexcept { reset(); }

        // explicit nullopt
        constexpr option(nullopt_t) noexcept : m_empty(), m_is_some(false) {}

        template <typename ...Args>
             requires(std::constructible_from<T, Args...>)
        constexpr option(Args&&... args) noexcept :
             m_value(std::forward<Args>(args)...), m_is_some(true) {}

        // ====

        template <typename ...Args>
            requires(std::constructible_from<T, Args...>)
        constexpr T& set(Args&&... args) noexcept {
            reset();

            m_is_some = true;

            new (&m_value) T(std::forward<Args>(args)...);

            return value_unchecked();
        }

        constexpr option& replace(option&& rhs) noexcept {
            reset();

            m_is_some = rhs.has_value();

            if (m_is_some) {
                new (&m_value) T(std::move(rhs.m_value));
            }

            return *this;
        }

        constexpr void reset() noexcept {
            if (has_value()) {
                m_value.~T();
                m_is_some = false;
            }
        }

        constexpr bool has_value() const noexcept { return m_is_some; }

        constexpr auto& value_unchecked(this auto& self) noexcept { return self.m_value; }

    private:
        union {
            struct empty {} m_empty;
            T m_value;
        };

        bool m_is_some;
    };

    // Optional reference - (A safe pointer)
    // Doesn't take ownership
    template <typename Ty>
    requires(std::is_lvalue_reference_v<Ty>)
    class option<Ty> : public OptionMethods<Ty> {
    public:
        using T = std::remove_reference_t<Ty>;

        constexpr option() noexcept : m_value(nullptr) {}

        constexpr option(option&) noexcept       = default;
        constexpr option(option const&) noexcept = default;
        constexpr option(option&&) noexcept      = default;

        constexpr ~option() noexcept = default;

        // explicit nullopt
        constexpr option(nullopt_t) noexcept : m_value(nullptr) {}

        constexpr option(T* t_ptr) noexcept : m_value(t_ptr) {}
        constexpr option(T& t) noexcept : m_value(std::addressof(t)) {}
        constexpr option(T&& t) noexcept : m_value(std::addressof(t)) {}

        // ====

        explicit constexpr operator T*& () noexcept { return m_value; }

        constexpr auto& set(T& t) noexcept {
            m_value = std::addressof(t);

            return value_unchecked();
        }

        constexpr void reset() noexcept { m_value = nullptr; }

        constexpr bool has_value() const noexcept { return m_value != nullptr; }

        constexpr auto& value_unchecked(this auto& self) noexcept { return *(self.m_value); }

    private:
        T* m_value;
    };

    // Optional unique_ref - (A safe std::unique_ptr)
    // Takes ownership
    template <typename Ty>
    class option<unique_ref<Ty>> : public OptionMethods<unique_ref<Ty>> {
    public:
        using T = unique_ref<Ty>;

        constexpr option() noexcept : m_value(nullptr) {}

        constexpr option(option&)           = delete;
        constexpr option(option const&)     = delete;
        constexpr option(option&&) noexcept = default;

        constexpr ~option() noexcept = default;

        constexpr option(nullopt_t) noexcept : m_value(nullptr) {}

        constexpr option(std::unique_ptr<Ty>&& uqptr) noexcept :
            m_value(maybe_nullref, std::move(uqptr))
        {}

        template <typename ...Args>
            requires(std::constructible_from<T, Args...>)
        constexpr option(Args&&... args) noexcept :
            m_value(std::forward<Args>(args)...) {}

        // ====

        explicit constexpr operator std::unique_ptr<Ty>& () noexcept { return m_value._Ptr; }

        template <typename ...Args>
            requires(std::constructible_from<T, Args...>)
        constexpr auto& set(Args&&... args) noexcept {
            reset();
            new (&m_value) T(std::forward<Args>(args)...);

            return value_unchecked();
        }

        constexpr option& replace(option&& rhs) noexcept {
            reset();
            m_value = std::move(rhs.m_value);

            return *this;
        }

        constexpr void reset() noexcept { m_value._Ptr.reset(); }

        constexpr bool has_value() const noexcept { return m_value._Ptr != nullptr; }

        constexpr auto& value_unchecked(this auto& self) noexcept { return self.m_value; }

    private:
        T m_value;
    };
}