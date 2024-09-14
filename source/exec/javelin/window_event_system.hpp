#pragma once

#include <imgui.h>

#include <functional>
#include <optional>

#include <littlevk/littlevk.hpp>

#include "wrapped_types.hpp"
#include "math_types.hpp"

class WindowEventSystem {
	double last_x = 0.0;
	double last_y = 0.0;
	int64_t uuid_counter = 0;
public:
	struct button_event {
		double x;
		double y;
		int button;
		int action;
		int mods;
	};

	struct cursor_event {
		double x;
		double y;
		double dx;
		double dy;
		bool drag;
	};

	using cursor_event_callback = std::function <void (const cursor_event &)>;

	class event_region {
		jvl::float2 min;
		jvl::float2 max;
		bool drag = false;

		std::optional <cursor_event_callback> cursor_callback;
	public:
		void set_cursor_callback(const cursor_event_callback &cbl) {
			cursor_callback = cbl;
		}

		void set_bounds(const ImVec2 &min_, const ImVec2 &max_) {
			min = { min_.x, min_.y };
			max = { max_.x, max_.y };
		}

		bool contains(double x, double y) const {
			return (x > min.x && x < max.x)
				&& (y > min.y && y < max.y);
		}

		friend class WindowEventSystem;
	};

	jvl::wrapped::hash_table <int64_t, event_region> regions;

	// TODO: put in the editor
	int64_t new_uuid() {
		int64_t ret = uuid_counter++;
		regions[ret] = event_region();
		return ret;
	}

	void button_callback(GLFWwindow *, int, int, int);
	void cursor_callback(GLFWwindow *, double, double);
};

void glfw_button_callback(GLFWwindow *, int, int, int);
void glfw_cursor_callback(GLFWwindow *, double, double);