#include "thunder/mir/transformations.hpp"

namespace jvl::thunder::mir {

OptimizationPassResult optimize_dead_code_elimination_pass(const Block &block)
{
	Linear instructions;

	auto users = mole_users(block);

	for (auto &r : block.body) {
		bool exempt = false;
		exempt |= r->is <Store> ();
		exempt |= r->is <Return> ();

		bool empty = !users.contains(r.idx()) || users.at(r.idx()).empty();
		
		if (exempt || !empty)
			instructions.emplace_back(r);
	}

	return OptimizationPassResult {
		.block = Block(instructions),
		.changed = block.body.count != instructions.size()
	};
}

Block optimize_dead_code_elimination(const Block &block)
{
	OptimizationPassResult pass;
	pass.block = block;

	do {
		pass = optimize_dead_code_elimination_pass(pass.block);
		fmt::println("DEC pass... (to {} moles)", pass.block.body.count);
	} while (pass.changed);

	return pass.block;
}

} // namespace jvl::thunder::mir