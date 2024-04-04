#pragma once

#include <vector>
#include <shared_mutex>
#include <span>

#include "better_types.hpp"
#include "option.hpp"

namespace gef {

	template <typename T>
	class sparse_array {
	public:
		
		std::vector<gef::option<T>> data_vec;
		std::vector<size_t> alive_vec;

	public:

		sparse_array() noexcept {}

		sparse_array(const size_t size) noexcept {
			resize(size);
		}

	public:

		constexpr void resize(const size_t new_capacity) noexcept {
			data_vec.resize(new_capacity);
			alive_vec.reserve(new_capacity);
		}

		constexpr T& at(this auto& self, const size_t index) noexcept {
			return self.data_vec[index].value_unchecked();
		}

		constexpr T& operator[](this auto& self, const size_t index) noexcept {
			return self.at(index);
		}

		template <typename ...Args>
			requires(std::constructible_from<T, Args...>)
		constexpr T& emplace_at(const size_t index, Args&&... args) noexcept {

			alive_vec.emplace_back(index);

			return data_vec[index].set(std::forward<Args>(args)...);
		}

		constexpr gef::option<size_t> next_empty_index() noexcept {

			if (size() == capacity()) {
				return gef::nullopt;
			}

			for (auto& item : data_vec) {
				if (item.is_null()) {
					return &item - &data_vec[0];
				}
			}

			return gef::nullopt;
		}

		constexpr void erase_at(const size_t index) noexcept {

			for (size_t& s : alive_vec) {
				if (s == index) {
					alive_vec.erase(alive_vec.begin() + (&s - &alive_vec[0]));
				}
			}

			data_vec[index].reset();
		}

		template <typename F>
			requires(requires(F&& f, T& v) { { f(v) }; })
		constexpr void erase_if(F&& f) noexcept {

			std::erase_if(alive_vec,
				[&](size_t& index) {
					if (std::invoke(std::forward<F>(f), at(index))) {
						data_vec[index].reset();

						return true;
					}

					return false;
				});
		}

		template <typename F>
			requires(requires(F&& f, T& v, size_t& i) { { f(v, i) } -> std::same_as<void>; })
		constexpr void for_each(F&& f) noexcept {

			for (size_t& index : alive_vec) {
				std::invoke(std::forward<F>(f), at(index), index);
			}
		}

		template <typename F>
			requires(requires(F&& f, T& v) { { f(v) } -> std::same_as<bool>; })
		constexpr gef::option<T&> first_if(F&& f) noexcept {

			for (size_t& index : alive_vec) {
				if (std::invoke(std::forward<F>(f), at(index))) {
					return at(index);
				}
			}

			return gef::nullopt;
		}

		// Invalidates all data
		constexpr void clear() noexcept {

			alive_vec.clear();

			for (auto& opt : data_vec) {
				opt.reset();
			}
		}

		constexpr size_t capacity() const noexcept {
			return alive_vec.capacity();
		}

		constexpr size_t size() const noexcept {
			return alive_vec.size();
		}
	};

}