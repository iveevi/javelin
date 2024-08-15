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

	int type;
	f32 has_albedo;
	f32 has_normal;
};

// GGX microfacet distribution function
void __ggx_ndf(Material mat, vec3 n, vec3 h)
{
	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, h), 0.0f, 0.999f));
	f32 ret = (alpha * alpha)
		/ (PI * pow(cos(theta), 4)
		* pow(alpha * alpha + tan(theta) * tan(theta), 2.0f));
	returns(ret);
}

// Smith shadow-masking function (single)
void __G1(Material mat, vec3 n, vec3 v)
{
	cond(dot(v, n) <= 0.0f);
		returns(0.0f);
	end();

	f32 alpha = mat.roughness;
	f32 theta = acos(clamp(dot(n, v), 0, 0.999f));

	f32 tan_theta = tan(theta);

	f32 denom = 1 + sqrt(1 + alpha * alpha * tan_theta * tan_theta);
	returns(2.0f/denom);
}

// // Smith shadow-masking function (double)
// f32 G(Material mat, vec3 n, vec3 wi, vec3 wo)
// {
// 	return G1(mat, n, wo) * G1(mat, n, wi);
// }

// Shlicks approximation to the Fresnel reflectance
void ggx_f(Material mat, vec3 wi, vec3 h)
{
	f32 k = pow(1 - dot(wi, h), 5);
	returns(mat.specular + (1 - mat.specular) * k);
}

// // GGX specular brdf
// vec3 ggx_brdf(Material mat, vec3 n, vec3 wi, vec3 wo)
// {
// 	if (dot(wi, n) <= 0.0f || dot(wo, n) <= 0.0f)
// 		return vec3(0.0f);
//
// 	vec3 h = normalize(wi + wo);
//
// 	vec3 f = ggx_f(mat, wi, h);
// 	f32 g = G(mat, n, wi, wo);
// 	f32 d = ggx_d(mat, n, h);
//
// 	vec3 num = f * g * d;
// 	f32 denom = 4 * dot(wi, n) * dot(wo, n);
//
// 	return num / denom;
// }
//
// // GGX pdf
// f32 ggx_pdf(Material mat, vec3 n, vec3 wi, vec3 wo)
// {
// 	if (dot(wi, n) <= 0.0f || dot(wo, n) < 0.0f)
// 		return 0.0f;
//
// 	vec3 h = normalize(wi + wo);
//
// 	f32 avg_Kd = (mat.diffuse.x + mat.diffuse.y + mat.diffuse.z) / 3.0f;
// 	f32 avg_Ks = (mat.specular.x + mat.specular.y + mat.specular.z) / 3.0f;
//
// 	f32 t = 1.0f;
// 	if (avg_Kd + avg_Ks > 0.0f)
// 		t = max(avg_Ks/(avg_Kd + avg_Ks), 0.25f);
//
// 	f32 term1 = dot(n, wi)/PI;
// 	f32 term2 = ggx_d(mat, n, h) * dot(n, h)/(4.0f * dot(wi, h));
//
// 	return (1 - t) * term1 + t * term2;
// }
//
// vec3 ggx_sample(Material mat, vec3 n, vec3 wo, inout vec3 seed)
// {
// 	f32 avg_Kd = (mat.diffuse.x + mat.diffuse.y + mat.diffuse.z) / 3.0f;
// 	f32 avg_Ks = (mat.specular.x + mat.specular.y + mat.specular.z) / 3.0f;
//
// 	f32 t = 1.0f;
// 	if (avg_Kd + avg_Ks > 0.0f)
// 		t = max(avg_Ks/(avg_Kd + avg_Ks), 0.25f);
//
// 	vec3 eta = fract(random3(seed));
//
// 	if (eta.x < t) {
// 		// Specular sampling
// 		f32 k = sqrt(eta.y/(1 - eta.y));
// 		f32 theta = atan(k * mat.roughness);
// 		f32 phi = 2.0f * PI * eta.z;
//
// 		vec3 h = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
// 		h = rotate(h, n);
//
// 		return reflect(-wo, h);
// 	}
//
// 	// Diffuse sampling
// 	f32 theta = acos(sqrt(eta.y));
// 	f32 phi = 2.0f * PI * eta.z;
//
// 	vec3 s = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
// 	return rotate(s, n);
// }
