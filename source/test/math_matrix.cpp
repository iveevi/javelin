#include <gtest/gtest.h>

#include "math_types.hpp"

using namespace jvl;

TEST(math_matrix, multiply_identity)
{
	float4x4 A = float4x4::identity();
	float4x4 B = float4x4::identity();
	float4x4 C = A * B;

	ASSERT_EQ(C, float4x4::identity());
}
