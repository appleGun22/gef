#pragma once

#include <shared_mutex>
#include <type_traits>

#include "option.hpp"


namespace gef {

    template <class T>
    class mutex {
    public:

        template <typename ...Args>
            requires(std::constructible_from<T, Args...>)
        constexpr mutex(Args&&... args) noexcept :
            value(std::forward<Args>(args)...) {}

        mutex(T&& t) noexcept : value(std::move(t)) {}

        template <typename F>
            requires(requires(F&& f, T& v) { { f(v) }; })
        constexpr auto lock(F&& f) noexcept {
            std::scoped_lock lock{ m_mutex };

            return std::invoke(std::forward<F>(f), value);
        }

        template <typename F>
            requires(requires(F&& f, T& v) { { f(v) }; })
        constexpr auto shared_lock(F&& f) noexcept {
            std::shared_lock lock{ m_mutex };

            return std::invoke(std::forward<F>(f), value);
        }

        template <typename F_locked, typename F_failed>
            requires(
                requires(F_locked&& f, T& v) { { f(v) }; } &&
                requires(F_failed&& f) { { f() }; }
            )
        constexpr void try_lock(F_locked&& f_locked, F_failed&& f_failed) noexcept {
            std::unique_lock lock{ m_mutex, std::try_to_lock };

            if (lock.owns_lock()) {
                std::invoke(std::forward<F_locked>(f_locked), value);
            }
            else {
                std::invoke(std::forward<F_failed>(f_failed), value);
            }
        }

    private:
        // std::mutex is bloated with ABI compatability, so use shared_mutex instead
        std::shared_mutex m_mutex;
        T value;
    };
}