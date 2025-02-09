#pragma once

#include "device_resource_collection.hpp"

#include <chrono>

struct Timer {
	using clk = std::chrono::high_resolution_clock;

	clk clock;
	clk::time_point mark;

	void reset() {
		mark = clock.now();
	}

	double click() const {
		auto end = clock.now();
		auto count = std::chrono::duration_cast <std::chrono::microseconds> (end - mark).count();
		return double(count) / 1000.0;
	}
};

template <typename F>
auto timed(const littlevk::Window &window, const F &f, double limit)
{
	Timer timer;
	timer.reset();

	return [&window, &f, timer, limit](const vk::CommandBuffer &cmd, uint32_t index) {
		if (limit >= 0 && timer.click() > limit)
			glfwSetWindowShouldClose(window.handle, true);

		f(cmd, index);
	};
}