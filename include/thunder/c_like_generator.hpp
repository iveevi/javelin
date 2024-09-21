#pragma once

#include <map>

#include "atom.hpp"
#include "qualified_type.hpp"
#include "buffer.hpp"

namespace jvl::thunder::detail {

struct auxiliary_block_t : Buffer {
	std::map <index_t, std::string> struct_names;

	auxiliary_block_t(const Buffer &buffer, const auto &names)
		: Buffer(buffer), struct_names(names) {}
};

struct c_like_generator_t : auxiliary_block_t {
	std::map <index_t, std::string> local_variables;
	size_t indentation;
	std::string source;

	c_like_generator_t(const auxiliary_block_t &);

	void finish(const std::string &, bool = true);

	void declare(index_t);
	void define(index_t, const std::string &);
	void assign(int, const std::string &);
	
	std::string reference(index_t) const;
	std::string inlined(index_t) const;

	std::vector <std::string> arguments(index_t) const;

	struct type_string {
		std::string pre;
		std::string post;
	};

	type_string type_to_string(const QualifiedType &) const;

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