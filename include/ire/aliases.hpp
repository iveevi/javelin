#pragma once

#include "native.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "sampler.hpp"
#include "image.hpp"

namespace jvl::ire {

////////////////////////////////////////
// Aliases for common primitive types //
////////////////////////////////////////

using i32     = native_t <int32_t>;
using u32     = native_t <uint32_t>;
using f32     = native_t <float>;
using boolean = native_t <bool>;

// Special primitives
using u64     = native_t <uint64_t>;

using ivec2 = vec <int32_t, 2>;
using ivec3 = vec <int32_t, 3>;
using ivec4 = vec <int32_t, 4>;

using uvec2 = vec <uint32_t, 2>;
using uvec3 = vec <uint32_t, 3>;
using uvec4 = vec <uint32_t, 4>;

using vec2 = vec <float, 2>;
using vec3 = vec <float, 3>;
using vec4 = vec <float, 4>;

using mat2 = mat <float, 2, 2>;
using mat3 = mat <float, 3, 3>;
using mat4 = mat <float, 4, 4>;

//////////////////////////
// Aliases for samplers //
//////////////////////////

using isampler1D = sampler <int32_t, 1>;
using isampler2D = sampler <int32_t, 2>;
using isampler3D = sampler <int32_t, 3>;

using usampler1D = sampler <uint32_t, 1>;
using usampler2D = sampler <uint32_t, 2>;
using usampler3D = sampler <uint32_t, 3>;

using sampler1D = sampler <float, 1>;
using sampler2D = sampler <float, 2>;
using sampler3D = sampler <float, 3>;

////////////////////////
// Aliases for images //
////////////////////////

using iimage1D = image <int32_t, 1>;
using iimage2D = image <int32_t, 2>;
using iimage3D = image <int32_t, 3>;

using uimage1D = image <uint32_t, 1>;
using uimage2D = image <uint32_t, 2>;
using uimage3D = image <uint32_t, 3>;

using image1D = image <float, 1>;
using image2D = image <float, 2>;
using image3D = image <float, 3>;

} // namespace jvl::ire
