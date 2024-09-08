#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include "ire/core.hpp"

using namespace jvl;
using namespace jvl::ire;

void check_cpluslpus_source(const std::string &cpp)
{
	std::filesystem::path file = "tmp.cpp";

	std::ofstream fout(file);
	fout << cpp;
	fout.close();

	int ret = system("g++ -c -fPIC tmp.cpp");
	if (ret)
		fmt::println("\n{}", cpp);

	// Delete temporary resources
	system("rm -f tmp.cpp tmp.o");

	EXPECT_EQ(ret, 0);
}

template <typename T>
void simple_io()
{
	Emitter::active.dump();
	auto shader = []() {
		layout_in <T> lin(0);
		layout_out <T> lout(0);
		lout = lin;
	};

	auto kernel = kernel_from_args(shader);
	auto cpp = kernel.compile(profiles::cplusplus_11);
	check_cpluslpus_source(cpp);
}

TEST(ire_synthesize_cpp, simple_int)
{
	simple_io <int> ();
}

TEST(ire_synthesize_cpp, simple_float)
{
	simple_io <float> ();
}

template <typename T, size_t N>
void simple_vector_io()
{
	auto shader = []() {
		layout_in <vec <T, N>> lin(0);
		layout_out <vec <T, N>> lout(0);
		lout = lin;
	};

	auto kernel = kernel_from_args(shader);
	auto cpp = kernel.compile(profiles::cplusplus_11);
	check_cpluslpus_source(cpp);
}

TEST(ire_synthesize_cpp, simple_int_2)
{
	simple_vector_io <int, 2> ();
}

TEST(ire_synthesize_cpp, simple_float_2)
{
	simple_vector_io <float, 2> ();
}

TEST(ire_synthesize_cpp, simple_int_3)
{
	simple_vector_io <int, 3> ();
}

TEST(ire_synthesize_cpp, simple_float_3)
{
	simple_vector_io <float, 3> ();
}

TEST(ire_synthesize_cpp, simple_int_4)
{
	simple_vector_io <int, 4> ();
}

TEST(ire_synthesize_cpp, simple_float_4)
{
	simple_vector_io <float, 4> ();
}
