#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <profiles/targets.hpp>

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
void simple_parameters()
{
	auto shader = [](i32 x, i32 y) {
		return x * y - y;
	};
	
	auto F = procedure("shader") << shader;
	auto cpp = link(F).generate_cpp();

	check_cpluslpus_source(cpp);
}

TEST(ire_synthesize_cpp, simple_int)
{
	simple_parameters <int> ();
}

TEST(ire_synthesize_cpp, simple_float)
{
	simple_parameters <float> ();
}

template <typename T, size_t N>
void simple_vector_parameters()
{
	auto shader = [](vec <T, N> &x, vec <T, N> &y) {
		return x * y - x + y;
	};

	auto F = procedure("shader") << shader;
	thunder::legalize_for_cc(F);
	auto cpp = link(F).generate_cpp();

	check_cpluslpus_source(cpp);
}

TEST(ire_synthesize_cpp, simple_int_2)
{
	simple_vector_parameters <int, 2> ();
}

TEST(ire_synthesize_cpp, simple_float_2)
{
	simple_vector_parameters <float, 2> ();
}

TEST(ire_synthesize_cpp, simple_int_3)
{
	simple_vector_parameters <int, 3> ();
}

TEST(ire_synthesize_cpp, simple_float_3)
{
	simple_vector_parameters <float, 3> ();
}

TEST(ire_synthesize_cpp, simple_int_4)
{
	simple_vector_parameters <int, 4> ();
}

TEST(ire_synthesize_cpp, simple_float_4)
{
	simple_vector_parameters <float, 4> ();
}
