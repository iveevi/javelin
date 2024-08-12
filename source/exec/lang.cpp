#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>
#include <fstream>
#include <array>
#include <stack>
#include <list>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>

#include "wrapped_types.hpp"
#include "lang/token.hpp"
#include "lang/memory.hpp"
#include "lang/parsing.hpp"
#include "lang/ast.hpp"

inline const std::string readfile(const std::filesystem::path &path)
{
	std::ifstream f(path);
	if (!f.good()) {
		fmt::println("could find file: {}", path);
		return "";
	}

	std::stringstream s;
	s << f.rdbuf();
	return s.str();
}

int main()
{
	std::filesystem::path path = "include/lang/proto/triangle.jvl";
	std::string source = readfile(path);
	fmt::println("{}", source);

	jvl::lang::parse_state ps(jvl::lang::lexer(source), 0);

	auto module = jvl::lang::ast_module::attempt(ps);

	module->dump();

	jvl::lang::grand_bank_t::one().dump();

	module.free();

	jvl::lang::grand_bank_t::one().dump();
}
