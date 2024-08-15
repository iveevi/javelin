#include <littlevk/littlevk.hpp>

#include "math_types.hpp"

struct ViewportRegion {
	jvl::float2 min;
	jvl::float2 max;
	bool active;

	size_t width() const {
		return max.x - min.x;
	}

	size_t height() const {
		return max.y - min.y;
	}

	float aspect() const {
		float width = max.x - min.x;
		float height = max.y - min.y;
		return width/height;
	}

	bool contains(const jvl::float2 &v) const {
		if (!active)
			return false;

		return (min.x <= v.x) && (v.x <= max.x)
			&& (min.y <= v.y) && (v.y <= max.y);
	}
} extern viewport_region;

struct MouseInfo {
	bool left_drag = false;
	bool voided = true;
	float last_x = 0.0f;
	float last_y = 0.0f;
} extern mouse;

void button_callback(GLFWwindow *, int, int, int);
void cursor_callback(GLFWwindow *, double, double);
