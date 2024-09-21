// TODO: warnings for the unused sections
// TODO: autodiff on inputs, for callables and shaders
// TODO: generating code with correct names
// TODO: test on shader toy shaders, use this as a gfx test
// TODO: passing layout inputs/outputs (should ignore them)
// TODO: test nested structs again
// TODO: partial evaluation of callables through substitution methods
// TODO: out/inout parameter qualifiers
// TODO: external constant specialization

#include <queue>
#include <map>
#include <set>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <ire/core.hpp>
#include <profiles/targets.hpp>
#include <thunder/c_like_generator.hpp>
#include <thunder/opt.hpp>
#include <thunder/properties.hpp>

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
	layout_in <u32, flat> bias(0);
	u32 x = (s.a << s.b) & (s.b - s.a) + bias;
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
	sampler1D tex(1);
	layout_out <u32, flat> result(0);
	vec2 v = seed_to_vector(s);
	vec3 vone = v.x * a.one + tex.fetch(0, 0).x;
	vec3 vtwo = v.y * a.two;
	result = floatBitsToUint(vtwo).x;
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

struct local_layout_type {
	index_t function;
	index_t index;
	QualifierKind kind;
};

static std::string layout_interpolationstring(QualifierKind kind)
{
	switch (kind) {
	case layout_in_flat:
	case layout_out_flat:
		return "flat ";
	case layout_in_noperspective:
	case layout_out_noperspective:
		return "noperspective ";
	case layout_in_smooth:
	case layout_out_smooth:
		return "";
	default:
		break;
	}

	return "<interp:?>";
}

static std::string sampler_string(QualifierKind kind)
{
	PrimitiveType result = sampler_result(kind);
	int32_t dimensions = sampler_dimension(kind);
	switch (result) {
        case ivec4:
                return fmt::format("isampler{}D", dimensions);
        case uvec4:
                return fmt::format("usampler{}D", dimensions);
        case vec4:
                return fmt::format("sampler{}D", dimensions);
        default:
                break;
        }

        return fmt::format("<?>sampler{}D", dimensions);
}

struct LinkageUnit {
	std::set <index_t> loaded;
	std::vector <Function> functions;
	std::vector <Aggregate> aggregates;
	std::vector <TypeMap> maps;

	struct {
		index_t push_constant = -1;
		std::map <index_t, local_layout_type> outputs;
		std::map <index_t, local_layout_type> inputs;
		std::map <index_t, QualifierKind> samplers;
	} globals;

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

	// TODO: return referenced callables
	void process_function(const Function &ftn) {
		// TODO: run validation here as well
		index_t index = functions.size();

		functions.emplace_back(ftn);
		maps.emplace_back();

		auto &function = functions.back();
		auto &map = maps.back();

		function.dump();

		for (size_t i : function.synthesized) {
			auto &atom = function[i];

			// Get the return type of the function
			if (atom.is <Returns> ())
				function.returns = function.types[i];

			// Parameters and global variables
			if (atom.is <Qualifier> ()) {
				auto &qualifier = atom.as <Qualifier> ();

				if (sampler_kind(qualifier.kind)) {
					size_t binding = qualifier.numerical;
					globals.samplers[binding] = qualifier.kind;
				}

				switch (qualifier.kind) {
				case parameter:
				{
					size_t binding = qualifier.numerical;
					size_t size = std::max(function.args.size(), binding + 1);
					function.args.resize(size);
					function.args[binding] = function.types[i];
				} break;

				case layout_in_flat:
				case layout_in_noperspective:
				case layout_in_smooth:
				{
					size_t binding = qualifier.numerical;
					local_layout_type lin(index, qualifier.underlying, qualifier.kind);
					globals.inputs[binding] = lin;
				} break;

				case layout_out_flat:
				case layout_out_noperspective:
				case layout_out_smooth:
				{
					size_t binding = qualifier.numerical;
					local_layout_type lout(index, qualifier.underlying, qualifier.kind);
					globals.outputs[binding] = lout;
				} break;

				default:
					break;
				}
			}

			// Checking for structs used by the function
			auto qt = function.types[i];
			if (qt.is <StructFieldType> ()) {
				fmt::println("attempting to create aggregate for {} (@{})", qt, i);
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
	}

	void add(const Callable &callable) {
		if (loaded.contains(callable.cid))
			return;

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

		// Create the generators
		std::vector <detail::c_like_generator_t> generators;

		for (size_t i = 0; i < functions.size(); i++) {
			auto &function = functions[i];
			auto &map = maps[i];

			std::map <index_t, std::string> structs;
			for (auto &[k, v] : map)
				structs[k] = aggregates[v].name;

			fmt::println("struct names for function #{}", i);
			for (auto &[k, v] : structs)
				fmt::println("  %{} -> {}", k, v);

			generators.emplace_back(detail::auxiliary_block_t(function, structs));
		}

		// Globals: layout inputs
		for (auto &[b, llt] : globals.inputs) {
			auto &types = functions[llt.function].types;
			auto &generator = generators[llt.function];


			fmt::println("layout input for: {}", types[llt.index]);
			auto ts = generator.type_to_string(types[llt.index]);
			auto interpolation = layout_interpolationstring(llt.kind);

			result += fmt::format("layout (location = {}) in {} {}_lin{};\n",
				b, ts.pre + ts.post, interpolation, b);
		}

		if (globals.inputs.size())
			result += "\n";
		
		// Globals: layout outputs
		for (auto &[b, llt] : globals.outputs) {
			auto &types = functions[llt.function].types;
			auto &generator = generators[llt.function];


			fmt::println("layout input for: {}", types[llt.index]);
			auto ts = generator.type_to_string(types[llt.index]);
			auto interpolation = layout_interpolationstring(llt.kind);

			result += fmt::format("layout (location = {}) out {} {}_lout{};\n",
				b, ts.pre + ts.post, interpolation, b);
		}

		if (globals.outputs.size())
			result += "\n";

		// Globals: samplers
		for (const auto &[binding, kind] : globals.samplers) {
			result += fmt::format("layout (binding = {}) uniform {} _sampler{};\n",
				binding, sampler_string(kind), binding);
		}

		if (globals.samplers.size())
			result += "\n";

		for (size_t i = 0; i < functions.size(); i++) {
			auto &function = functions[i];
			auto &generator = generators[i];
				
			auto ts = generator.type_to_string(function.returns);

			// TODO: topologically sort the functions before synthesis

			std::string args;
			for (size_t j = 0; j < function.args.size(); j++) {
				auto ts = generator.type_to_string(function.args[j]);
				args += fmt::format("{} _arg{}{}", ts.pre, j, ts.post);
				if (j + 1 < function.args.size())
					args += ", ";
			}

			result += fmt::format("{} {}({})\n",
				ts.pre + ts.post,
				function.name, args);

			result += "{\n";
			result += generator.generate();
			result += "}\n";

			if (i + 1 < functions.size())
				result += "\n";
		}

		return result;
	}
};

}

int main()
{
	thunder::opt_transform(seed_to_vector);
	thunder::opt_transform(seed_to_angle);

	// seed_to_vector.dump();

	// fmt::println("{}",
	// 	seed_to_vector
	// 	.export_to_kernel()
	// 	.compile(profiles::glsl_450));
	
	// seed_to_angle.dump();
	
	// fmt::println("{}",
	// 	seed_to_angle
	// 	.export_to_kernel()
	// 	.compile(profiles::glsl_450));

	thunder::LinkageUnit unit;
	unit.add(seed_to_angle);
	unit.add(seed_to_vector);

	fmt::println("# of loaded: {}", unit.loaded.size());
	fmt::println("# of functions: {}", unit.functions.size());
	fmt::println("# of aggregates: {}", unit.aggregates.size());
	fmt::println("# of type maps: {}", unit.maps.size());

	fmt::println("{}", unit.generate_glsl());
}