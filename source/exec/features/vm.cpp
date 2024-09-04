#include <concepts>
#include <functional>

#include "thunder/atom.hpp"
#include "math_types.hpp"
#include "thunder/enumerations.hpp"
#include "wrapped_types.hpp"

using namespace jvl;
using namespace jvl::thunder;

struct unsupported_overload {
	OperationCode code;
	size_t nargs;
};

struct Operator {
	using value_t = wrapped::variant <unsupported_overload, int, float, int2>;

	unsupported_overload ovl;

	Operator(const unsupported_overload &ovl_) : ovl(ovl_) {}
};

template <typename T, typename U>
concept operator_addable = requires(const T &t, const U &u) {
	{
		t + u
	};
};

struct Adder : Operator {
	Adder() : Operator({ addition, 2 }) {}
	
	template <typename T, typename U>
	requires operator_addable <T, U>
	value_t operator()(const T &t, const U &u) const {
		return t + u;
	}

	template <typename T, typename U>
	value_t operator()(const T &, const U &) const {
		return ovl;
	}
};

struct VM {
	struct Value {
		union {
			bool bdata;
			int idata;
			float fdata;
		};

		PrimitiveType type = bad;

		index_t next = -1;
		index_t down = -1;

		Value() = default;

		Value(const Primitive &p) : type(p.type) {
			switch (type) {
			case boolean: bdata = p.bdata; break;
			case i32: idata = p.idata; break;
			case f32: fdata = p.fdata; break;
			default: break;
			}
		}

		std::string to_string() const {
			std::string result = fmt::format("{:7s}", tbl_primitive_types[type]);

			switch (type) {
			case i32:
			case ivec2:
			case ivec3:
			case ivec4:
				result += fmt::format(" {:5d}", idata);
				break;
			case f32:
			case vec2:
			case vec3:
			case vec4:
				result += fmt::format(" {:4.3f}", fdata);
				break;
			default:
				result += " ?";
				break;
			}

			result += fmt::format("     (next: {}, down: {})",
				(next >= 0) ? fmt::format("{:5d}", next) : "(nil)",
				(down >= 0) ? fmt::format("{:5d}", down) : "(nil)");

			return result;
		}
	};

	// Main executation memory
	std::vector <Value> memory;

	index_t push(Value v) {
		index_t i = memory.size();
		memory.push_back(v);
		return i;
	}

	index_t write(int value) {
		Primitive primitive { .idata = value, .type = i32 };
		return push(primitive);
	}
	
	index_t write(float value) {
		Primitive primitive { .fdata = value, .type = f32 };
		return push(primitive);
	}

	index_t write(int2 value) {
		Value vx;
		vx.idata = value.x;
		vx.type = ivec2;

		Value vy;
		vy.idata = value.y;
		vy.type = i32;

		index_t iy = push(vy);
		vx.down = iy;
		
		return push(vx);
	}

	// Global input parameters
	wrapped::hash_table <index_t, index_t> parameters;

	template <typename T>
	void parameter(index_t loc, T value) {
		parameters[loc] = write(value);
	}

	// Result of instructions (instruction counter -> location in memory)
	wrapped::hash_table <index_t, index_t> values;
	wrapped::hash_table <index_t, Atom> history;

	void execute_global(index_t counter, const Global &global) {
		switch (global.qualifier) {
		case thunder::parameter:
			// TODO: check types...
			values[counter] = parameters[global.binding];
			break;
		default:
			fmt::println("VM does not support qualifier type {}", tbl_global_qualifier[global.qualifier]);
			abort();
		}
	}

	void execute_load(index_t counter, const Load &load) {
		if (load.idx == -1) {
			values[counter] = values[load.src];
			return;
		}

		// Walk to the correct field
		index_t i = values[load.src];

		index_t c = load.idx;
		while (c > 0) {
			Value &v = memory[i];
			assert(v.down != -1);
			i = v.down;
			c--;
		}

		// Downcast if needed (only for the original type)
		if (load.idx == 0) {
			switch (memory[i].type) {

			case ivec2:
			{
				Value nv = memory[i];
				nv.type = i32;
				nv.down = -1;
				values[counter] = push(nv);
			} break;

			default:
			{
				fmt::println("unsupported type for downcasting: {}",
					tbl_primitive_types[memory[i].type]);
				abort();
			}

			}
		} else {
			// Otherwise get it and unlink
			Value nv = memory[i];
			nv.down = -1;
			values[counter] = push(nv);
		}
	}

	void execute_list(index_t counter, const List &list) {
		index_t i = values[list.item];

		Value nv = memory[i];
		if (list.next != -1) {
			nv.next = values[list.next];
			values[counter] = push(nv);
		} else {
			values[counter] = i;
		}
	}

	void execute_operation(index_t counter, const Operation &opn) {
		using value_t = Operator::value_t;

		auto translate_value = [&](const Value &v) -> value_t {
			switch (v.type) {
			
			case i32:
				return v.idata;

			case f32:
				return v.fdata;

			case ivec2:
			{
				assert(v.down != -1);
				const Value &n = memory[v.down];
				fmt::println("translated ivec2: ({}, {})", v.idata, n.idata);
				return int2(v.idata, n.idata);
			}

			default:
			{
				fmt::println("VM does not support translation of type {}",
					tbl_primitive_types[v.type]);
				abort();
			}

			}
		};

		auto write_value = [&](const value_t &v) {
			if (auto uns = v.get <unsupported_overload> ()) {
				fmt::println("unsupported overload for operation {}",
					tbl_operation_code[uns->code]);
				abort();
			} else if (auto i2 = v.get <int2> ()) {
				values[counter] = write(*i2);
			} else if (auto i = v.get <int> ()) {
				values[counter] = write(*i);
			} else {
				fmt::println("VM does not supported writing back value of type index", v.index());
				abort();
			}
		};

		std::vector <value_t> args;
		std::vector <Value> original;

		index_t i = values[opn.args];
		while (i != -1) {
			Value &v = memory[i];
			original.push_back(v);
			args.push_back(translate_value(v));
			i = v.next;
		}

		auto calculate = [&](const auto &opr) {
			if (args.size() != opr.ovl.nargs) {
				fmt::println("operation {} expected {} arguments, got {}",
					tbl_operation_code[opr.ovl.code],
					opr.ovl.nargs,
					args.size());

				abort();
			}

			auto v = std::visit(opr, args[0], args[1]);
			if (v.template is <unsupported_overload> ()) {
				fmt::println("unsupported overload ({}, {}) for operation {}",
					tbl_primitive_types[original[0].type],
					tbl_primitive_types[original[1].type],
					tbl_operation_code[opr.ovl.code]);

				abort();
			}

			write_value(v);
		};

		switch (opn.code) {

		case addition:
			return calculate(Adder());
		
		default:
		{
			fmt::println("VM does not support operation: {}",
				tbl_operation_code[opn.code]);
			break;
		}

		}
	}

	index_t execute(const Atom &atom) {
		index_t counter = values.size();

		switch (atom.index()) {
	
		case Atom::type_index <Global> ():
			execute_global(counter, atom.as <Global> ());
			break;

		case Atom::type_index <Load> ():
			execute_load(counter, atom.as <Load> ());
			break;

		case Atom::type_index <List> ():
			execute_list(counter, atom.as <List> ());
			break;

		case Atom::type_index <Operation> ():
			execute_operation(counter, atom.as <Operation> ());
			break;

		default:
			fmt::println("VM does not support instruction type: {}", atom);
			abort();
		}

		history[counter] = atom;

		return counter;
	}

	// Display the current machine state
	void dump() const {
		fmt::println("------------------------------");
		fmt::println("VM:");
		fmt::println("  {} primitives in memory", memory.size());
		fmt::println("  {} parameters", parameters.size());
		fmt::println("  {} instructions in history", history.size());
		fmt::println("------------------------------");
		
		fmt::println("MEMORY");
		fmt::println("------------------------------");
		for (int i = 0; i < memory.size(); i++)
			fmt::println("[{:4d}]  {}", i, memory[i].to_string());

		fmt::println("------------------------------");
		fmt::println("HISTORY");
		fmt::println("------------------------------");
		for (int i = 0; i < history.size(); i++)
			fmt::println("[{:4d}| {:4d}] {}", i, values.at(i), history.at(i).to_string());
	}
};

int main()
{
	VM vm;

	vm.parameter(0, int2(1, 5));
	vm.parameter(1, 2.0f);

	index_t v = vm.execute(Global(f32, 0, parameter));
	index_t l = vm.execute(Load(v, 1));
	index_t a = vm.execute(List(l, -1));
	index_t b = vm.execute(List(l, a));
	index_t c = vm.execute(Operation(b, i32, addition));

	vm.dump();
}