#include "thunder/c_like_generator.hpp"
#include "thunder/linkage_unit.hpp"

namespace jvl::thunder {

MODULE(generate);

void LinkageUnit::generate_function(std::string &result,
				    detail::c_like_generator_t &generator,
				    const Function &function)
{
	JVL_INFO("generating function '{}.{}'", function.name, function.cid);

	auto ts = generator.type_to_string(function.returns);

	std::string args;
	for (size_t j = 0; j < function.args.size(); j++) {
		auto ts = generator.type_to_string(function.args[j]);
		args += fmt::format("{} _arg{}{}", ts.pre, j, ts.post);
		if (j + 1 < function.args.size())
			args += ", ";
	}

	result += fmt::format("{} {}({})\n",
		ts.pre + ts.post,
		function.name, args);

	result += "{\n";
	result += generator.generate();
	result += "}\n";
}

} // namespace jvl::thunder
