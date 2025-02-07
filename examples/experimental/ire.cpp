// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include "ire/control_flow.hpp"
#include "thunder/enumerations.hpp"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <thunder/opt.hpp>
#include <thunder/linkage_unit.hpp>
#include <constants.hpp>

using namespace jvl;
using namespace jvl::ire;

// Task shader payload information
// TODO: generalize this structure for native/builtin/aggregate...
template <generic T>
struct task_payload {};

template <native T>
struct task_payload <T> {
	task_payload &operator=(const native_t <T> &value) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <native_t <T>> ().id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t dst = em.emit_construct(qual, -1, thunder::transient);
		em.emit_store(dst, value.synthesize().id);
		return *this;
	}
};

template <builtin T>
struct task_payload <T> : T {
	template <typename ... Args>
	task_payload(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		thunder::index_t type = type_field_from_args <T> ().id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t value = em.emit_construct(qual, -1, thunder::transient);
		this->ref = value;
	}

	task_payload &operator=(const T &value) {
		T::operator=(value);
		return *this;
	}
};

template <aggregate T>
struct task_payload <T> : T {
	template <typename ... Args>
	task_payload(const Args &... args) : T(args...) {
		auto &em = Emitter::active;
		auto layout = this->layout().remove_const();
		thunder::index_t type = type_field_from_args(layout).id;
		thunder::index_t qual = em.emit_qualifier(type, -1, thunder::task_payload);
		thunder::index_t value = em.emit_construct(qual, -1, thunder::transient);
		layout.ref_with(cache_index_t::from(value));
	}
};

struct payload {
	u32 pindex;
	u32 resolution;

	auto layout() const {
		return uniform_layout("payload",
			named_field(pindex),
			named_field(resolution));
	}
};

auto task = procedure <void> ("task_shader") << []()
{
	// layout_out <i32> pld(0);
	// pld = 9;
	
	// task_payload <i32> pld;
	// pld = 9;

	// task_payload <int> pld;
	// pld = 9;
	
	// task_payload <uvec2> pld;
	// pld.y = 9;
	
	task_payload <payload> pld;
	pld.pindex = 14;
	pld.resolution = 16;
};

auto mesh = procedure <void> ("mesh_shader") << []()
{
	local_size(8, 8);

	mesh_shader_size(16, 8);
};

int main()
{
	thunder::opt_transform(task);
	task.dump();
	fmt::println("{}", link(task).generate_glsl());
	
	thunder::opt_transform(mesh);
	mesh.dump();
	fmt::println("{}", link(mesh).generate_glsl());
}
