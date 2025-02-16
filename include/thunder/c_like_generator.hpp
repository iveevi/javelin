#pragma once

#include <map>

#include "atom.hpp"
#include "qualified_type.hpp"
#include "buffer.hpp"

namespace jvl::thunder::detail {

struct auxiliary_block_t : Buffer {
	std::map <Index, std::string> struct_names;

	auxiliary_block_t(const Buffer &buffer, const auto &names)
		: Buffer(buffer), struct_names(names) {}
};

struct c_like_generator_t : auxiliary_block_t {
	std::map <Index, std::string> local_variables;
	size_t indentation;
	std::string source;

	c_like_generator_t(const auxiliary_block_t &);

	void finish(const std::string &, bool = true);

	void declare(Index);
	void define(Index, const std::string &);
	void assign(int, const std::string &);
	
	std::string reference(Index) const;
	std::string inlined(Index) const;

	std::vector <std::string> arguments(Index) const;

	struct type_string {
		std::string pre;
		std::string post;
	};

	type_string type_to_string(const QualifiedType &) const;

	// Per-atom generator
	void generate(Index);

	template <atom_instruction T>
	void generate(const T &atom, Index i) {
                MODULE(c-like-generator-generate);

		JVL_ABORT("failed to generate C-like source for: {} (@{})", atom, i);
	}

	// General generator
	std::string generate();
};

} // namespace jvl::thunder