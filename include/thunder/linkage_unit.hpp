#pragma once

#include <map>
#include <set>

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "thunder/qualified_type.hpp"
#include "tracked_buffer.hpp"

namespace jvl::thunder {

struct Field : QualifiedType {
	std::string name;

	Field() = default;
	Field(const QualifiedType &q, const std::string &s) : QualifiedType(q), name(s) {}
};

struct Aggregate {
	size_t function;
	std::string name;
	std::vector <Field> fields;

	bool operator==(const Aggregate &) const;
};

struct Function : Buffer {
	size_t cid;
	std::string name;
	QualifiedType returns;
	std::vector <QualifiedType> args;

	Function(const Buffer &, const std::string &, size_t);
};

using TypeMap = std::map <index_t, index_t>;

struct local_layout_type {
	size_t function;
	index_t index;
	QualifierKind kind;
};

struct special_type {
	size_t function;
	index_t index;
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
	std::optional <glm::uvec3> local_size;
	std::optional <glm::uvec2> mesh_shader_size;

	std::set <index_t> loaded;
	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <TypeMap> maps;
	std::map <index_t, index_t> cids;
	std::map <index_t, std::set <index_t>> dependencies;
	std::set <std::string> extensions;

	struct {
		push_constant_info push_constant = push_constant_info::from();
		std::map <index_t, local_layout_type> outputs;
		std::map <index_t, local_layout_type> inputs;
		std::map <index_t, local_layout_type> uniforms;
		std::map <index_t, local_layout_type> buffers;
		std::map <index_t, local_layout_type> shared;
		std::map <index_t, QualifierKind> samplers;
		std::map <QualifierKind, special_type> special;
	} globals;

	index_t new_aggregate(size_t, const std::string &, const std::vector <Field> &);

	void process_function_qualifier(Function &, size_t, index_t, const Qualifier &);
	void process_function_intrinsic(Function &, size_t, index_t, const Intrinsic &);
	void process_function_aggregate(TypeMap &, const Function &, size_t, index_t, QualifiedType);

	std::set <index_t> process_function(const Function &);

	void add(const TrackedBuffer &);

	auto configure_generators() const;

	// Generating code
	std::string generate_glsl() const;
	std::string generate_cpp() const;
	std::string generate_cuda() const;

	// TODO: conditional guard for SPIRV support
	std::vector <uint32_t> generate_spirv(const vk::ShaderStageFlagBits &) const;

	// TODO: cuda JIT as well
	void *jit() const;
};

} // namespace jvl::thunder
