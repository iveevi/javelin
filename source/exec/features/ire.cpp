// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include "thunder/atom.hpp"
#include "thunder/qualified_type.hpp"
#include <queue>
#include <map>
#include <set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/c_like_generator.hpp>
#include <thunder/opt.hpp>

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

struct Aggregate {
	std::string name;
	std::vector <QualifiedType> fields;

	bool operator==(const Aggregate &other) {
		if (fields.size() != other.fields.size())
			return false;
	
		for (size_t i = 0; i < fields.size(); i++) {
			if (fields[i] != other.fields[i])
				return false;
		}

		return true;
	}
};

struct Function : Buffer {
	std::string name;
	QualifiedType returns;
	std::vector <QualifiedType> args;

	Function(const Buffer &buffer, const std::string &name_)
		: Buffer(buffer), name(name_) {}
};

using TypeMap = std::map <index_t, index_t>;

struct LinkageUnit {
	std::set <index_t> loaded;
	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <TypeMap> maps;

	index_t new_aggregate(const std::vector <QualifiedType> &fields) {
		size_t index = aggregates.size();
	
		Aggregate aggr {
			.name = fmt::format("s{}_t", index),
			.fields = fields
		};

		auto it = std::find(aggregates.begin(), aggregates.end(), aggr);
		if (it == aggregates.end()) {
			aggregates.push_back(aggr);
			return index;
		}

		fmt::println("duplicate aggregate\n");

		return std::distance(aggregates.begin(), it);
	}

	void process_function(Function function) {
		TypeMap map;

		for (size_t i : function.synthesized) {
			auto &atom = function[i];

			// Get the return type of the function
			if (atom.is <Returns> ())
				function.returns = function.types[i];

			// Get the parameter types
			if (atom.is <Qualifier> ()) {
				auto &qualifier = atom.as <Qualifier> ();
				size_t binding = qualifier.numerical;
				size_t size = std::max(function.args.size(), binding + 1);
				function.args.resize(size);

				function.args[binding] = function.types[i];
			}

			// Checking for structs used by the function
			auto qt = function.types[i];
			if (qt.is <StructFieldType> ()) {
				std::vector <QualifiedType> fields {
					qt.as <StructFieldType> ().base()
				};

				do {
					auto &sft = qt.as <StructFieldType> ();
					qt = function.types[sft.next];
					if (qt.is <StructFieldType> ()) {
						auto &sft = qt.as <StructFieldType> ();
						fields.push_back(sft.base());
					} else {
						// TODO: if its concrete, we need to reference it...
						JVL_ASSERT_PLAIN(qt.is_primitive());
						fields.push_back(qt);
					}
				} while (qt.is <StructFieldType> ());

				map[i] = new_aggregate(fields);
			}
		}

		functions.emplace_back(function);
		maps.emplace_back(map);
	}

	void add(const Callable &callable) {
		if (loaded.contains(callable.cid)) {
			fmt::println("callable already loaded into linkage unit");
			return;
		}

		fmt::println("adding callable to linkage unit:");
		callable.dump();

		process_function(Function(callable, callable.name));
	
		std::queue <index_t> referenced;
		for (size_t i = 0; i < callable.pointer; i++) {
			auto &atom = callable[i];
			if (atom.is <Call> ()) {
				auto &call = atom.as <Call> ();
				referenced.push(call.cid);
			}
		}

		loaded.insert(callable.cid);
		while (referenced.size()) {
			index_t i = referenced.front();
			referenced.pop();
		
			add(*Callable::search_tracked(i));
		}
	}

	std::string generate_glsl() {
		std::string result;

		result += "#version 450\n";
		result += "\n";

		for (size_t i = 0; i < functions.size(); i++) {
			auto &function = functions[i];
			auto &map = maps[i];

			std::map <index_t, std::string> structs;
			for (auto &[k, v] : map)
				structs[k] = aggregates[v].name;

			detail::auxiliary_block_t block(function, structs);
			detail::c_like_generator_t generator(block);
				
			auto ts = generator.type_to_string(function.returns);

			// TODO: sort

			std::string args;
			for (size_t i = 0; i < function.args.size(); i++) {
				auto ts = generator.type_to_string(function.args[i]);
				args += fmt::format("{} _arg{}{}", ts.pre, i, ts.post);
				if (i + 1 < function.args.size())
					args += ", ";
			}

			result += fmt::format("{} {}({})\n",
				ts.pre + ts.post,
				function.name, args);

			result += "{\n";
			result += generator.generate();
			result += "}\n";
		}

		return result;
	}
};

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

	thunder::LinkageUnit unit;
	unit.add(seed_to_angle);
	unit.add(seed_to_vector);

	fmt::println("# of loaded: {}", unit.loaded.size());
	fmt::println("# of functions: {}", unit.functions.size());
	fmt::println("# of aggregates: {}", unit.aggregates.size());
	fmt::println("# of type maps: {}", unit.maps.size());

	fmt::println("{}", unit.generate_glsl());
}