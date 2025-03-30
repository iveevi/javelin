#pragma once

#include <ire.hpp>

using namespace jvl::ire;

using ColorMap = vec3 (*)(f32);

inline vec3 viridis(f32 t)
{
	array <vec3> colors = std::array <vec3, 17> {
		vec3(0.267004, 0.004874, 0.329415),
		vec3(0.268510, 0.009605, 0.335427),
		vec3(0.269944, 0.014625, 0.341379),
		vec3(0.273809, 0.031497, 0.358853),
		vec3(0.282327, 0.095856, 0.417331),
		vec3(0.298300, 0.140245, 0.423156),
		vec3(0.344074, 0.173832, 0.446230),
		vec3(0.404001, 0.228802, 0.506102),
		vec3(0.478113, 0.280894, 0.539470),
		vec3(0.558309, 0.341448, 0.553437),
		vec3(0.643914, 0.425895, 0.556217),
		vec3(0.725254, 0.492206, 0.558263),
		vec3(0.808868, 0.561568, 0.552989),
		vec3(0.890001, 0.644231, 0.525776),
		vec3(0.963624, 0.740373, 0.500228),
		vec3(0.991014, 0.859422, 0.404156),
		vec3(0.983868, 0.983868, 0.106764),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = fract(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 coolwarm(f32 t)
{
	const vec3 blue = vec3(0.230, 0.299, 0.754);
	const vec3 red = vec3(0.706, 0.016, 0.150);
	return mix(blue, red, t);
}

inline vec3 plasma(f32 t)
{
	array <vec3> colors = std::array <vec3, 5> {
		vec3(0.050383, 0.029803, 0.527975),
		vec3(0.243113, 0.157851, 0.580158),
		vec3(0.604854, 0.298763, 0.539024),
		vec3(0.901807, 0.455796, 0.313098),
		vec3(0.986893, 0.998364, 0.644924),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = fract(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 magma(f32 t)
{
	array <vec3> colors = std::array <vec3, 6> {
		vec3(0.001462, 0.000466, 0.013866),
		vec3(0.189952, 0.072838, 0.364543),
		vec3(0.488034, 0.117846, 0.520837),
		vec3(0.798216, 0.280197, 0.469538),
		vec3(0.986888, 0.518867, 0.319809),
		vec3(0.987053, 0.998364, 0.644924),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = fract(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 inferno(f32 t)
{
	array <vec3> colors = std::array <vec3, 6> {
		vec3(0.001462, 0.000466, 0.013866),
		vec3(0.134692, 0.038125, 0.286266),
		vec3(0.432299, 0.088325, 0.544931),
		vec3(0.779289, 0.198144, 0.579307),
		vec3(0.995236, 0.418828, 0.407278),
		vec3(0.988362, 0.998364, 0.644924),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = fract(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 cividis(f32 t)
{
	array <vec3> colors = std::array <vec3, 6> {
		vec3(0.0000, 0.1351, 0.3042),
		vec3(0.2028, 0.3734, 0.5536),
		vec3(0.4287, 0.5289, 0.5534),
		vec3(0.6712, 0.6800, 0.4627),
		vec3(0.8686, 0.8313, 0.3660),
		vec3(0.9932, 0.9786, 0.2165),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = f32(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 turbo(f32 t)
{
	array <vec3> colors = std::array <vec3, 9> {
		vec3(0.18995, 0.07176, 0.23217),
		vec3(0.25107, 0.25237, 0.63315),
		vec3(0.27628, 0.44031, 0.76149),
		vec3(0.27801, 0.61115, 0.75516),
		vec3(0.23306, 0.82041, 0.69230),
		vec3(0.15751, 0.92852, 0.55229),
		vec3(0.48650, 0.98462, 0.31395),
		vec3(0.79253, 0.95628, 0.12851),
		vec3(0.97689, 0.98392, 0.08051),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = f32(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 jet(f32 t)
{
	array <vec3> colors = std::array <vec3, 5> {
		vec3(0.0, 0.0, 0.5),
		vec3(0.0, 0.5, 1.0),
		vec3(0.5, 1.0, 0.5),
		vec3(1.0, 1.0, 0.0),
		vec3(0.5, 0.0, 0.0)
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = f32(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 parula(f32 t)
{
	array <vec3> colors = std::array <vec3, 9> {
		vec3(0.18995, 0.07176, 0.23217),
		vec3(0.25107, 0.25237, 0.63315),
		vec3(0.27628, 0.44031, 0.76149),
		vec3(0.27801, 0.61115, 0.75516),
		vec3(0.23306, 0.82041, 0.69230),
		vec3(0.15751, 0.92852, 0.55229),
		vec3(0.48650, 0.98462, 0.31395),
		vec3(0.79253, 0.95628, 0.12851),
		vec3(0.97689, 0.98392, 0.08051),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = f32(x);

	return mix(colors[idx], colors[idx + 1], f);
}

inline vec3 spectral(f32 t)
{
	array <vec3> colors = std::array <vec3, 11> {
		vec3(0.3686, 0.3098, 0.6353),
		vec3(0.1961, 0.5333, 0.7412),
		vec3(0.4000, 0.7608, 0.6471),
		vec3(0.6706, 0.8667, 0.6431),
		vec3(0.9020, 0.9608, 0.5961),
		vec3(1.0000, 1.0000, 0.7490),
		vec3(0.9961, 0.8784, 0.5451),
		vec3(0.9922, 0.6824, 0.3804),
		vec3(0.9569, 0.4275, 0.2627),
		vec3(0.8353, 0.2431, 0.3098),
		vec3(0.6196, 0.0039, 0.2588),
	};

	t = clamp(t, 0.0f, 1.0f);
	f32 x = t * (colors.length - 2);
	i32 idx = i32(x);
	f32 f = f32(x);

	return mix(colors[idx], colors[idx + 1], f);
}