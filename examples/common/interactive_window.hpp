#pragma once

#include <littlevk/littlevk.hpp>

struct InteractiveWindow : littlevk::Window {
	InteractiveWindow() = default;
	InteractiveWindow(const littlevk::Window &);

	bool should_close() const;
	bool key_pressed(int) const;
	void poll() const;
};