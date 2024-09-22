#include "ire/callable.hpp"
#include "ire/core.hpp"

using namespace jvl::ire;

// Constants
const float PI = 3.1415926535897932384626433832795;

// Material properties
struct Material {
	vec3 diffuse;
	vec3 specular;
	vec3 emission;
	vec3 ambient;

	f32 shininess;
	f32 roughness;

	f32 has_albedo;
	f32 has_normal;

	auto layout() const {
		return uniform_layout(
			"Material",
			field <"diffuse"> (diffuse),
			field <"specular"> (specular),
			field <"emission"> (emission),
			field <"ambient"> (ambient),
			field <"shininess"> (shininess),
			field <"roughness"> (roughness),
			field <"has_albedo"> (has_albedo),
			field <"has_normal"> (has_normal)
		);
	}
};

// Random number generation
auto pcg3d = callable("pcg3d") << [](uvec3 v)
{
	v = v * 1664525u + 1013904223u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	v ^= v >> 16u;
	v.x += v.y * v.z;
	v.y += v.z * v.x;
	v.z += v.x * v.y;
	return v;
};

auto random3 = callable("random3") << [](/* inout */ vec3 seed)
{
	seed = uintBitsToFloat(
		(pcg3d(floatBitsToUint(seed)) & 0x007FFFFFu)
			| 0x3F800000u
	) - 1.0;

	return seed;
};

// Rotate a vector to orient it along a given direction
auto rotate = callable("rotate") << [](vec3 s, vec3 n)
{
	vec3 w = n;
	vec3 a = vec3(0.0f, 1.0f, 0.0f);

	cond(dot(w, a) > 0.999f);
		a = vec3(0.0f, 0.0f, 1.0f);
	end();

	vec3 u = normalize(cross(w, a));
	vec3 v = normalize(cross(w, u));

	return u * s.x + v * s.y + w * s.z;
};

// GGX microfacet distribution function
auto ggx_ndf = callable("ggx_ndf") << [](Material mat, vec3 n, vec3 h)
{
	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
	f32 ret = (alpha * alpha)
		/ (PI * pow(cos(theta), 4)
		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	return ret;
};

// Smith shadow-masking function (single)
auto G1 = callable <f32> ("G1") << [](Material mat, vec3 n, vec3 v)
{
	cond(dot(v, n) <= 0.0f);
		returns(0.0f);
	end();

	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, v), 0, 0.999f));

	f32 tan_theta = tan(theta);

	f32 denom = 1.0f + sqrt(1.0f + alpha * alpha * tan_theta * tan_theta);
	returns(2.0f/denom);
};

// Smith shadow-masking function (double)
auto G = callable("G") << [](Material mat, vec3 n, vec3 wi, vec3 wo)
{
	return G1(mat, n, wo) * G1(mat, n, wi);
};

// Shlicks approximation to the Fresnel reflectance
auto ggx_fresnel = callable("ggx_fresnel") << [](Material mat, vec3 wi, vec3 h)
{
	f32 k = pow(1.0f - dot(wi, h), 5);
	return mat.specular + (1 - mat.specular) * k;
};

// GGX specular brdf
auto ggx_brdf = callable <vec3> ("ggx_brdf") << [](Material mat, vec3 n, vec3 wi, vec3 wo)
{
	cond(dot(wi, n) <= 0.0f || dot(wo, n) <= 0.0f);
		returns(vec3(0.0f));
	end();

	vec3 h = normalize(wi + wo);

	vec3 f = ggx_fresnel(mat, wi, h);
	f32 g = G(mat, n, wi, wo);
	f32 d = ggx_ndf(mat, n, h);

	vec3 num = f * g * d;
	f32 denom = 4.0f * dot(wi, n) * dot(wo, n);

	returns(num / denom);
};

// GGX PDF
auto ggx_pdf = callable <f32> ("ggx_pdf") << [](Material mat, vec3 n, vec3 wi, vec3 wo)
{
	cond(dot(wi, n) <= 0.0f || dot(wo, n) < 0.0f);
		returns(0.0f);
	end();

	vec3 h = normalize(wi + wo);

	f32 avg_Kd = (mat.diffuse.x + mat.diffuse.y + mat.diffuse.z) / 3.0f;
	f32 avg_Ks = (mat.specular.x + mat.specular.y + mat.specular.z) / 3.0f;

	f32 t = 1.0f;
	cond(avg_Kd + avg_Ks > 0.0f);
		t = max(avg_Ks/(avg_Kd + avg_Ks), 0.25f);
	end();

	f32 term1 = dot(n, wi)/PI;
	f32 term2 = ggx_ndf(mat, n, h) * dot(n, h)/(4.0f * dot(wi, h));

	returns((1.0f - t) * term1 + t * term2);
};

// TODO: qualifiers
auto ggx_sample = callable <f32> ("ggx_sample") << [](Material mat, vec3 n, vec3 wo, /* inout */ vec3 seed)
{
	f32 avg_Kd = (mat.diffuse.x + mat.diffuse.y + mat.diffuse.z) / 3.0f;
	f32 avg_Ks = (mat.specular.x + mat.specular.y + mat.specular.z) / 3.0f;

	f32 t = 1.0f;
	cond(avg_Kd + avg_Ks > 0.0f);
		t = max(avg_Ks/(avg_Kd + avg_Ks), 0.25f);
	end();

	vec3 eta = fract(random3(seed));

	cond(eta.x < t);
	{
		// Specular sampling
		f32 k = sqrt(eta.y/(1.0f - eta.y));
		f32 theta = atan(k * mat.roughness);
		f32 phi = 2.0f * PI * eta.z;

		vec3 h = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
		h = rotate(h, n);

		returns(reflect(-wo, h));
	}
	end();

	// Diffuse sampling
	f32 theta = acos(sqrt(eta.y));
	f32 phi = 2.0f * PI * eta.z;

	vec3 s = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

	returns(rotate(s, n));
};