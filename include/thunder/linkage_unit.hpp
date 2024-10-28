#pragma once

#include <map>
#include <set>

#include <vulkan/vulkan.hpp>

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

struct push_constant_info : local_layout_type {
	size_t offset;
	index_t underlying;

	static push_constant_info from() {
		return { { 0, -1, thunder::push_constant }, 0, -1 };
	}
};

struct LinkageUnit {
	// TODO: dirty flag for caching
	std::set <index_t> loaded;
	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <TypeMap> maps;
	std::map <index_t, std::set <index_t>> dependencies;

	struct {
		push_constant_info push_constant = push_constant_info::from();
		std::map <index_t, local_layout_type> outputs;
		std::map <index_t, local_layout_type> inputs;
		std::map <index_t, local_layout_type> uniforms;
		std::map <index_t, local_layout_type> buffers;
		std::map <index_t, local_layout_type> shared;
		std::map <index_t, QualifierKind> samplers;
	} globals;

	index_t new_aggregate(size_t, const std::vector <QualifiedType> &);

	void process_function_qualifier(Function &, size_t, index_t, const Qualifier &);
	void process_function_aggregate(TypeMap &, const Function &, size_t, index_t, QualifiedType);
	
	std::set <index_t> process_function(const Function &);
	
	void add(const TrackedBuffer &);
	
	auto configure_generators() const;

	// Generating code
	std::string generate_glsl() const;
	std::string generate_cpp() const;

	// TODO: conditional guard for SPIRV support
	std::vector <uint32_t> generate_spirv(const vk::ShaderStageFlagBits &) const;

	void *jit() const;
};

} // namespace jvl::thunder