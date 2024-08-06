#pragma once

#include <vector>
#include <functional>

#include "../../math_types.hpp"

namespace jvl::gfx::cpu {

template <typename T>
struct Framebuffer {
	size_t width;
	size_t height;

	std::vector <T> data;

	T *operator[](size_t i) {
		return &data[i * width];
	}

	const T *const operator[](size_t i) const {
		return &data[i * width];
	}

	template <typename U>
	Framebuffer <U> transform() const {
		Framebuffer <U> fb;
		fb.width = width;
		fb.height = height;
		fb.data.resize(width * height);

		for (size_t i = 0; i < data.size(); i++)
			fb.data[i] = static_cast <U> (data[i]);

		return fb;
	}

	template <typename U, typename F>
	requires std::is_same_v <std::invoke_result_t <F, T>, U>
	Framebuffer <U> transform(const F &kernel) const {
		Framebuffer <U> fb;
		fb.width = width;
		fb.height = height;
		fb.data.resize(width * height);

		for (size_t i = 0; i < data.size(); i++)
			fb.data[i] = kernel(data[i]);

		return fb;
	}

	static Framebuffer from(size_t width, size_t height) {
		Framebuffer fb;
		fb.width = width;
		fb.height = height;
		fb.data.resize(width * height);
		return fb;
	}
};

} // namespace jvl::gfx::cpu
