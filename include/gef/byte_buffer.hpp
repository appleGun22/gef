#pragma once

#include "better_types.hpp"

class byte_buffer {
public:
	byte_buffer() :
		buffer(nullptr),
		_buffer_size(0)
	{}

	byte_buffer(const size_t init_bytes) :
		buffer((byte*)std::malloc(init_bytes)),
		_buffer_size(init_bytes)
	{}

	~byte_buffer() {
		if (buffer != nullptr) {
			std::free(buffer);
		}
	}

	void reserve(size_t init_bytes) {
		if (buffer == nullptr) {
			buffer = (byte*)std::malloc(init_bytes);
		}
		else {
			buffer = (byte*)std::realloc(buffer, init_bytes);
		}

		_buffer_size = init_bytes;
	}

	void copy_back(const void* from, const size_t size) {
		std::memcpy(buffer + rw_size, from, size);
		rw_size += size;
	}

	template <typename T, typename ...Args>
	void construct_back(Args ...args) {
		new (buffer + rw_size) T{ std::forward<Args>(args)... };
		rw_size += sizeof(T);
	}

	void load_front(void* to, const size_t size) {
		std::memcpy(to, buffer + rw_size, size);
		rw_size += size;
	}

	template <typename T>
	T make_front() {
		T t;

		std::memcpy(&t, buffer + rw_size, sizeof(T));

		rw_size += sizeof(T);

		return t;
	}

	constexpr size_t buffer_size() const {
		return _buffer_size;
	}

public:
	byte* buffer;

private:
	size_t rw_size{ 0 };
	size_t _buffer_size;
};