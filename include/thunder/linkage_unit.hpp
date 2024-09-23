#pragma once

#include <map>
#include <set>

#include "buffer.hpp"
#include "tracked_buffer.hpp"

namespace jvl::thunder {

struct Aggregate {
	size_t function;
	std::string name;
	std::vector <QualifiedType> fields;

	bool operator==(const Aggregate &) const;
};

struct Function : Buffer {
	std::string name;
	QualifiedType returns;
	std::vector <QualifiedType> args;

	Function(const Buffer &, const std::string &);
};

using TypeMap = std::map <index_t, index_t>;

struct local_layout_type {
	size_t function;
	index_t index;
	QualifierKind kind;
};

struct LinkageUnit {
	std::set <index_t> loaded;
	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <TypeMap> maps;

	struct {
		local_layout_type push_constant = { 0, -1 };
		std::map <index_t, local_layout_type> outputs;
		std::map <index_t, local_layout_type> inputs;
		std::map <index_t, local_layout_type> uniforms;
		std::map <index_t, local_layout_type> buffers;
		std::map <index_t, QualifierKind> samplers;
	} globals;

	index_t new_aggregate(size_t, const std::vector <QualifiedType> &);

	void process_function_qualifier(Function &, index_t, index_t, const Qualifier &);
	void process_function_aggregate(TypeMap &, const Function &, index_t, QualifiedType);
	
	std::vector <index_t> process_function(const Function &);
	
	void add(const TrackedBuffer &);
	
	auto configure_generators() const;

	std::string generate_glsl() const;
	std::string generate_cpp() const;
	void *jit() const;
};

} // namespace jvl::thunder