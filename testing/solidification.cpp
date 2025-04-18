#include <gtest/gtest.h>

#include <ire.hpp>

using namespace jvl;
using namespace jvl::ire;

template <typename T, size_t F>
auto solid_offset()
{
	T A;
	return ((std::intptr_t) &A.template get <F> () - (std::intptr_t) &A);
}

TEST(solidification, proxy_A)
{
	struct proxy_A {
		vec3 x;
		vec3 y;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y));
		}
	};

	static_assert(std::same_as <solid_t <proxy_A>, aggregate_storage <aligned_vec3, glm::vec3>>);

	using solid_A = solid_t <proxy_A>;

	fmt::println("offset of f0: {}", solid_offset <solid_A, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_A, 1> ());

	ASSERT_EQ((solid_offset <solid_A, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_A, 1> ()), 16);
}

TEST(solidification, proxy_B)
{
	struct proxy_B {
		u32 x;
		vec3 y;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y));
		}
	};

	static_assert(std::same_as <solid_t <proxy_B>, aggregate_storage <aligned_uint32_t <16>, glm::vec3>>);

	using solid_B = solid_t <proxy_B>;

	fmt::println("offset of f0: {}", solid_offset <solid_B, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_B, 1> ());

	ASSERT_EQ((solid_offset <solid_B, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_B, 1> ()), 16);
}

TEST(solidification, proxy_C)
{
	struct proxy_C {
		vec3 x;
		u32 y;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y));
		}
	};

	static_assert(std::same_as <solid_t <proxy_C>, aggregate_storage <glm::vec3, uint32_t>>);

	using solid_C = solid_t <proxy_C>;

	fmt::println("offset of f0: {}", solid_offset <solid_C, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_C, 1> ());

	ASSERT_EQ((solid_offset <solid_C, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_C, 1> ()), 12);
}

TEST(solidification, proxy_D)
{
	struct proxy_D {
		vec3 x;
		vec3 y;
		u32 z;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y),
				verbatim_field(z));
		}
	};

	static_assert(std::same_as <solid_t <proxy_D>, aggregate_storage <aligned_vec3, glm::vec3, uint32_t>>);

	using solid_D = solid_t <proxy_D>;

	fmt::println("offset of f0: {}", solid_offset <solid_D, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_D, 1> ());
	fmt::println("offset of f2: {}", solid_offset <solid_D, 2> ());

	ASSERT_EQ((solid_offset <solid_D, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_D, 1> ()), 16);
	ASSERT_EQ((solid_offset <solid_D, 2> ()), 28);
}

TEST(solidification, proxy_E)
{
	struct proxy_E {
		vec3 x;
		u32 y;
		vec3 z;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y),
				verbatim_field(z));
		}
	};

	static_assert(std::same_as <solid_t <proxy_E>, aggregate_storage <glm::vec3, uint32_t, glm::vec3>>);

	using solid_E = solid_t <proxy_E>;

	fmt::println("offset of f0: {}", solid_offset <solid_E, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_E, 1> ());
	fmt::println("offset of f2: {}", solid_offset <solid_E, 2> ());

	ASSERT_EQ((solid_offset <solid_E, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_E, 1> ()), 12);
	ASSERT_EQ((solid_offset <solid_E, 2> ()), 16);
}

TEST(solidification, proxy_F)
{
	struct proxy_F {
		u32 x;
		vec3 y;
		vec3 z;

		auto layout() {
			return layout_from("A",
				verbatim_field(x),
				verbatim_field(y),
				verbatim_field(z));
		}
	};

	static_assert(std::same_as <solid_t <proxy_F>, aggregate_storage <aligned_uint32_t <16>, aligned_vec3, glm::vec3>>);

	using solid_F = solid_t <proxy_F>;

	fmt::println("offset of f0: {}", solid_offset <solid_F, 0> ());
	fmt::println("offset of f1: {}", solid_offset <solid_F, 1> ());
	fmt::println("offset of f2: {}", solid_offset <solid_F, 2> ());

	ASSERT_EQ((solid_offset <solid_F, 0> ()), 0);
	ASSERT_EQ((solid_offset <solid_F, 1> ()), 16);
	ASSERT_EQ((solid_offset <solid_F, 2> ()), 32);
}