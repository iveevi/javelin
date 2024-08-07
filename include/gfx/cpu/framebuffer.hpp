#pragma once

#include <type_traits>
#include <vector>

#include "../../math_types.hpp"
#include "../../wrapped_types.hpp"

namespace jvl::gfx::cpu {

struct Tile {
	int2 min;
	int2 max;
};

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

	wrapped::thread_safe_queue <Tile> tiles(int2 size) const {
		wrapped::thread_safe_queue <Tile> tsq;

		for (int i = 0; i < (height + size.y - 1)/size.y; i++) {
			for (int j = 0; j < (width + size.x - 1)/size.x; j++) {
				// TODO: fix by cropping
				int2 ji = { j, i };
				int2 ji_n = { j + 1, i + 1 };

				Tile tile {
					.min = ji * size,
					.max = min(ji_n * size, int2(width, height))
				};

				tsq.push(tile);
			}
		}

		return tsq;
	}

	// TODO: int2 indexing

	// TODO: with input attachments as well
	template <typename F>
	requires std::is_same_v <std::invoke_result_t <F, int2, float2>, T>
	void process(const F &kernel) {
		// TODO: tile version
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				int2 ij { i, j };
				float2 uv = { j/float(width), i/float(height) };
				data[i * width + j] = kernel(ij, uv);
			}
		}
	}

	template <typename F>
	requires std::is_same_v <std::invoke_result_t <F, int2, float2>, T>
	void process_tile(const F &kernel, const Tile &tile) {
		// TODO: tile version
		for (int i = tile.min.y; i < tile.max.y; i++) {
			for (int j = tile.min.x; j < tile.max.x; j++) {
				int2 ij { i, j };
				float2 uv = { j/float(width), i/float(height) };
				data[i * width + j] = kernel(ij, uv);
			}
		}
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

	void clear() {
		memset(data.data(), 0, sizeof(T) * width * height);
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
