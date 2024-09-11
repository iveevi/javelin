#pragma once

#include "atom.hpp"
#include "qualified_type.hpp"
#include "buffer.hpp"

namespace jvl::thunder::detail {

// TODO: type alias for synthesis list
// Determine the set of instructions to concretely synthesized
std::unordered_set <index_t> synthesize_list(const std::vector <Atom> &);

struct body_t : Buffer {
	const wrapped::hash_table <int, std::string> &struct_names;
	std::unordered_set <index_t> synthesized;

	body_t(const Buffer &buffer, const auto &names)
			: Buffer(buffer), struct_names(names) {
		synthesized = synthesize_list(pool);
	}
};

struct c_like_generator_t : body_t {
	wrapped::hash_table <index_t, std::string> local_variables;
	size_t indentation;
	std::string source;

	c_like_generator_t(const body_t &);

	void finish(const std::string &, bool = true);

	void declare(const std::string &, index_t, index_t = -1);
	void define(const std::string &, const std::string &, index_t);
	void assign(int, const std::string &);
	
	std::string reference(index_t) const;
	std::string inlined(index_t) const;

	std::vector <std::string> arguments(index_t) const;

	std::string generate_type_string(index_t, index_t) const;

	// Per-atom generator
	void generate(index_t);

	template <atom_instruction T>
	void generate(const T &atom, index_t i) {
                MODULE(c-like-generator-generate);

		JVL_ABORT("failed to generate C-like source for: {} (@{})", atom, i);
	}

	// General generator
	std::string generate();
};

} // namespace jvl::thunder