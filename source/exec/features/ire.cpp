// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <map>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>

#include "ire/intrinsics/glsl.hpp"
#include "ire/type_synthesis.hpp"
#include "thunder/atom.hpp"
#include "thunder/linkage.hpp"
#include "thunder/opt.hpp"
#include "thunder/qualified_type.hpp"
#include "wrapped_types.hpp"

using namespace jvl;
using namespace jvl::ire;
	
MODULE(ire-features);

struct seed {
	u32 a;
	u32 b;

	auto layout() const {
		return uniform_layout("seed",
			named_field(a),
			named_field(b));
	}
};

auto seed_to_vector = callable_info() >> [](const seed &s)
{
	u32 x = (s.a << s.b) & (s.b - s.a);
	u32 y = (s.b + s.a * s.b)/s.a;
	uvec2 v = uvec2(x, y);
	return uintBitsToFloat(v);
};

struct aux {
	vec3 one;
	vec3 two;
	vec3 three;

	auto layout() const {
		return uniform_layout("aux",
			named_field(one),
			named_field(two),
			named_field(three));
	}
};

auto seed_to_angle = callable_info() >> [](const aux &a, const seed &s)
{
	vec2 v = seed_to_vector(s);
	vec3 vone = v.x * a.one;
	vec3 vtwo = v.y * a.two;
	return dot(vone, vtwo);
};

namespace jvl::thunder {

using normalized_aggregate = std::vector <QualifiedType>;
using normalized_aggregate_map = std::map <index_t, normalized_aggregate>;

normalized_aggregate_map collect_aggregates(const Buffer &buffer)
{
	normalized_aggregate_map collected;

	for (size_t i : buffer.synthesized) {
		auto qt = buffer.types[i];
		if (!qt.is <StructFieldType> ())
			continue;

		normalized_aggregate aggregate {
			qt.as <StructFieldType> ().base()
		};

		do {
			auto &sft = qt.as <StructFieldType> ();
			qt = buffer.types[sft.next];
			if (qt.is <StructFieldType> ()) {
				auto &sft = qt.as <StructFieldType> ();
				aggregate.push_back(sft.base());
			} else {
				// TODO: if its concrete, we need to reference it...
				JVL_ASSERT_PLAIN(qt.is_primitive());
				aggregate.push_back(qt);
			}
		} while (qt.is <StructFieldType> ());

		collected[i] = aggregate;
	}

	return collected;
}

struct aggregate_meta {
	std::string name;
	normalized_aggregate aggregate;
};

struct linked_aggregate_map {
	using mapping = std::map <index_t, index_t>;

	std::vector <mapping> maps;
	std::vector <aggregate_meta> aggregates;
};

bool compare(const normalized_aggregate &A, const normalized_aggregate &B)
{
	if (A.size() != B.size())
		return false;

	for (size_t i = 0; i < A.size(); i++) {
		if (A[i] != B[i])
			return false;
	}

	return true;
}

linked_aggregate_map collect_linked_aggregates(const std::vector <Buffer> &buffers)
{
	linked_aggregate_map linked_aggregates;

	auto insert = [&](const normalized_aggregate &aggregate) -> index_t {
		auto &list = linked_aggregates.aggregates;
		auto it = std::find_if(list.begin(), list.end(),
			[&](const auto &meta) {
				return meta.aggregate == aggregate;
			});

		if (it != list.end())
			return std::distance(list.begin(), it);

		index_t i = list.size();
		std::string name = fmt::format("s{}_t", i);
		aggregate_meta meta { name, aggregate };
		list.push_back(meta);

		return i;
	};

	for (auto &buffer : buffers) {
		linked_aggregate_map::mapping map;

		auto aggregates = collect_aggregates(buffer);
		for (auto &[k, v] : aggregates)
			map[k] = insert(v);

		linked_aggregates.maps.push_back(map);
	}

	fmt::println("linked aggregate:");
	fmt::println("  aggregates:");
	for (auto &meta : linked_aggregates.aggregates) {
		fmt::println("    name: {}", meta.name);
		for (auto &qt : meta.aggregate)
			fmt::println("      {}", qt);
	}

	fmt::println("  buffer maps:");
	for (size_t i = 0; i < linked_aggregates.maps.size(); i++) {
		auto &map = linked_aggregates.maps[i];
		fmt::println("  map for block #{}", i + 1);
		for (auto &[k, v] : map)
			fmt::println("    {} -> {}", k, v);
	}

	return {};
}

}

int main()
{
	thunder::opt_transform(seed_to_vector);
	seed_to_vector.dump();

	fmt::println("{}",
		seed_to_vector
		.export_to_kernel()
		.compile(profiles::glsl_450));
	
	thunder::opt_transform(seed_to_angle);
	seed_to_angle.dump();
	
	fmt::println("{}",
		seed_to_angle
		.export_to_kernel()
		.compile(profiles::glsl_450));

	thunder::collect_aggregates(seed_to_vector);
	thunder::collect_aggregates(seed_to_angle);
	thunder::collect_linked_aggregates({ seed_to_angle, seed_to_vector });
}