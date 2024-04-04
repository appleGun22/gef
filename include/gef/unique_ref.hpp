#pragma once

#include <memory>
#include <type_traits>

namespace gef {

	struct maybe_nullref_t {};

	// set unique_ref to `maybe_nullref` when it is guaranteed that unique_ref won't be accessed
	// (Example: delayed initialization)
	static constexpr maybe_nullref_t maybe_nullref{};

	// Just like std::unique_ptr, but cannot be null (except after moved, but it is not a valid state)
	template <class T> class unique_ref {
	public:

		constexpr unique_ref(unique_ref&)           = delete;
		constexpr unique_ref(const unique_ref&)     = delete;
		constexpr unique_ref(unique_ref&&) noexcept = default;

		constexpr ~unique_ref() noexcept = default;

		constexpr unique_ref(std::nullptr_t) noexcept : _Ptr(nullptr) {}

		// Explicitly state that the unique_ptr may be null
		constexpr unique_ref(maybe_nullref_t, std::unique_ptr<T>&& uqptr) noexcept :
			_Ptr(std::move(uqptr))
		{}

		// Explicitly state that the pointer may be null
		// Takes ownership
		constexpr unique_ref(maybe_nullref_t, T* ptr) noexcept :
			_Ptr(ptr)
		{}

		template <typename ...Args>
			requires(std::constructible_from<T, Args...>)
		constexpr unique_ref(std::in_place_t, Args&&... args) noexcept :
			_Ptr(std::make_unique<T>(std::forward<Args>(args)...)) {}

		template <typename U>
			requires(std::derived_from<U, T>)
		constexpr unique_ref(unique_ref<U>&& u) noexcept :
			_Ptr(std::move(u._Ptr)) {}

		template <typename ...Args>
			requires(std::constructible_from<T, Args...>)
		static unique_ref make(Args&&... args) noexcept {
			return unique_ref{ std::in_place, std::forward<Args>(args)... };
		}

		constexpr unique_ref& operator=(unique_ref&)           = delete;
		constexpr unique_ref& operator=(const unique_ref&)     = delete;
		constexpr unique_ref& operator=(unique_ref&&) noexcept = default;
		constexpr unique_ref& operator=(const unique_ref&&)    = delete;

		constexpr auto& get(this auto& self) noexcept {
			return *(self._Ptr.get());
		}

		constexpr void swap(unique_ref& rhs) noexcept {
			_Ptr.swap(rhs.storage());
		}

		constexpr auto operator->(this auto& self) noexcept {
			return self._Ptr.operator->();
		}

	public:
		std::unique_ptr<T> _Ptr;
	};
}