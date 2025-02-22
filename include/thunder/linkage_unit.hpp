#pragma once

#include <map>
#include <set>

#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "qualified_type.hpp"
#include "tracked_buffer.hpp"
#include "c_like_generator.hpp"

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

using TypeMap = std::map <Index, Index>;

struct local_layout_type {
	size_t function;
	Index index;
	QualifierKind kind;
	std::set <QualifierKind> extra;
};

struct special_type {
	size_t function;
	Index index;
};

using special_type_set = std::map <Index, special_type>;

struct push_constant_info : local_layout_type {
	size_t offset;
	Index underlying;

	static push_constant_info from() {
		return { { 0, -1, thunder::push_constant }, 0, -1 };
	}
};

using generator_list = std::vector <detail::c_like_generator_t>;

struct LinkageUnit {
	// TODO: dirty flag for caching using hash?
	std::optional <glm::uvec3> local_size;
	std::optional <glm::uvec2> mesh_shader_size;

	std::set <Index> loaded;

	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <local_layout_type> references;

	std::vector <TypeMap> maps;
	
	std::map <Index, Index> cids;
	std::map <Index, std::set <Index>> dependencies;

	std::set <std::string> extensions;

	struct {
		push_constant_info push_constant = push_constant_info::from();
		std::map <Index, local_layout_type> outputs;
		std::map <Index, local_layout_type> inputs;
		std::map <Index, local_layout_type> uniforms;
		std::map <Index, local_layout_type> buffers;
		std::map <Index, local_layout_type> shared;
		std::map <Index, local_layout_type> samplers;
		std::map <Index, local_layout_type> images;
		std::map <QualifierKind, special_type_set> special;
	} globals;

	Index new_aggregate(size_t, const std::string &, const std::vector <Field> &);

	void process_function_qualifier(Function &, size_t, Index, const Qualifier &);
	void process_function_intrinsic(Function &, size_t, Index, const Intrinsic &);
	void process_function_aggregate(TypeMap &, const Function &, size_t, Index, QualifiedType);

	std::set <Index> process_function(const Function &);

	void add(const TrackedBuffer &);

	generator_list configure_generators() const;

	// Generating code
	std::string generate_glsl() const;
	std::string generate_cpp() const;
	std::string generate_cuda() const;

	static void generate_function(std::string &, detail::c_like_generator_t &, const Function &);

	// TODO: conditional guard for SPIRV support
	std::vector <uint32_t> generate_spirv(const vk::ShaderStageFlagBits &) const;

	// TODO: cuda JIT as well
	void *jit() const;
};

} // namespace jvl::thunder
