#pragma once

#include <chrono>

struct Timer {
	using clk = std::chrono::high_resolution_clock;

	clk clock;
	clk::time_point mark;

	void reset() {
		mark = clock.now();
	}

	// Result is milliseconds
	double click() const {
		auto end = clock.now();
		auto count = std::chrono::duration_cast <std::chrono::microseconds> (end - mark).count();
		return double(count) / 1000.0;
	}
};