#pragma once

// TODO: remove
#include <iostream>

#include <fmt/printf.h>

#include "wrapped_types.hpp"

namespace jvl::ire::op {

struct Global {
	int type = -1;
	int binding = -1;

	enum {
		layout_in,
		layout_out
	} qualifier;
};

enum PrimitiveType {
	bad,
	boolean,
	i32,
	f32,
	vec2,
	vec3,
	vec4,
	mat4,
};

struct TypeField {
	// If the current field is a
	// primitive, then item != BAD
	PrimitiveType item = bad;

	// Otherwise it is a nested struct
	int down = -1;

	// Next field
	int next = -1;
};

struct Primitive {
	PrimitiveType type = bad;
	union {
		bool  b;
		float fdata[4];
		int   idata[4] = { 0, 0, 0, 0 };
	};
};

struct Swizzle {
	enum Kind {
		x, y, z, w,
		xy
	} type;

	constexpr Swizzle(Kind t) : type(t) {}
};

struct Cmp {
	int a = -1;
	int b = -1;
	enum {
		eq, neq,
	} type;
};

struct List {
	int item = -1;
	int next = -1;
};

struct Construct {
	int type = -1;
	int args = -1;
};

struct Store {
	int dst = -1;
	int src = -1;
};

struct Load {
	int src = -1;
};

struct Cond {
	int cond = -1;
	int failto = -1;
};

struct Elif : Cond {};

struct End {};

using General = wrapped::variant <
	Global,
	TypeField,
	Primitive,
	Swizzle,
	Cmp,
	Construct,
	List,
	Store,
	Load,
	Cond,
	Elif,
	End
>;

// Type translation
static const char *type_table[] = {
	"<BAD>", "bool", "int", "float",
	"vec2", "vec3", "vec4",
	"mat4",
};

struct typeof_vd {
	op::General *pool;

	const wrapped::hash_table <int, std::string> &struct_names;

	// TODO: cache system here as well

	// TODO: needs to be persistent handle structs later
	const char *resolve(const PrimitiveType type) {
		return type_table[type];
	}

	std::string operator()(const TypeField &type) {
		if (type.item != bad)
			return resolve(type.item);
		return "<?>";
	}

	std::string operator()(const Global &global) {
		// NOTE: this is implicit conversion
		return defer(global.type);
	}

	template <typename T>
	std::string operator()(const T &) {
		fmt::println("unknown type: {}", typeid(T).name());
		return "<?>";
	}

	std::string defer(int index) {
		if (struct_names.contains(index))
			return struct_names.at(index);

		return std::visit(*this, pool[index]);
	}
};

struct dump_vd {
	const char* resolve(const PrimitiveType &t) {
		return type_table[t];
	}

	void operator()(const Global &global) {
		static const char *qualifier_table[] = {
			"layout input",
			"layout output"
		};

		printf("global: %%%d = (%s, %d)", global.type,
			qualifier_table[global.qualifier], global.binding);
	}

	void operator()(const TypeField &t) {
		printf("type: ");
		if (t.item != bad)
			printf("%s", resolve(t.item));
		else
			printf("%%%d", t.down);

		printf(" -> ");
		if (t.next >= 0)
			printf("%%%d", t.next);
		else
			printf("(nil)");
	}

	void operator()(const Primitive &p) {
		printf("primitive: %s = ", resolve(p.type));

		switch (p.type) {
		case i32:
			printf("%d", p.idata[0]);
			break;
		case f32:
			printf("%.2f", p.fdata[0]);
			break;
		case vec4:
			printf("(%.2f, %.2f, %.2f, %.2f)",
					p.fdata[0],
					p.fdata[1],
					p.fdata[2],
					p.fdata[3]);
			break;
		default:
			printf("?");
			break;
		}
	}

	void operator()(const List &list) {
		printf("list: %%%d -> ", list.item);
		if (list.next >= 0)
			printf("%%%d", list.next);
		else
			printf("(nil)");
	}

	void operator()(const Construct &ctor) {
		printf("construct: %%%d = ", ctor.type);
		if (ctor.args == -1)
			printf("(nil)");
		else
			printf("%%%d", ctor.args);
	}

	void operator()(const Store &store) {
		printf("store %%%d -> %%%d", store.src, store.dst);
	}

	void operator()(const Load &load) {
		printf("load %%%d", load.src);
	}

	void operator()(const Cmp &cmp) {
		const char *cmp_table[] = { "=", "!=" };
		printf("cmp %%%d %s %%%d", cmp.a, cmp_table[cmp.type], cmp.b);
	}

	void operator()(const Cond &cond) {
		printf("cond %%%d -> %%%d", cond.cond, cond.failto);
	}

	void operator()(const Elif &elif) {
		if (elif.cond >= 0)
			printf("elif %%%d -> %%%d", elif.cond, elif.failto);
		else
			printf("elif (nil) -> %%%d", elif.failto);
	}

	void operator()(const End &) {
		printf("end");
	}

	template <typename T>
	void operator()(const T &) {
		printf("<?>");
	}
};

struct translate_vd {
	op::General *pool;
	int generator = 0;
	int indentation = 0;
	int next_indentation = 0;
	bool inlining = false;

	wrapped::hash_table <int, std::string> symbols;
	wrapped::hash_table <int, std::string> struct_names;

	std::string operator()(const Cond &cond) {
		next_indentation++;
		std::string c = defer(cond.cond, true);
		return "if (" + c + ") {";
	}

	std::string operator()(const Elif &elif) {
		next_indentation = indentation--;
		if (elif.cond == -1)
			return "} else {";

		std::string c = defer(elif.cond, true);
		return "} else if (" + c + ") {";
	}

	std::string operator()(const End &) {
		// TODO: verify which end?
		next_indentation = indentation--;
		return "}";
	}

	std::string operator()(const Global &global) {
		switch (global.qualifier) {
		case Global::layout_out:
			return "_lout" + std::to_string(global.binding);
		case Global::layout_in:
			return "_lin" + std::to_string(global.binding);
		default:
			break;
		}

		return "<glo:?>";
	}

	std::string operator()(const Primitive &p) {
		// TODO: fmt format
		switch (p.type) {
		case i32:
			return std::to_string(p.idata[0]);
		case f32:
			return std::to_string(p.fdata[0]);
		case vec4:
			return "vec4("
				+ std::to_string(p.fdata[0]) + ", "
				+ std::to_string(p.fdata[1]) + ", "
				+ std::to_string(p.fdata[2]) + ", "
				+ std::to_string(p.fdata[3])
				+ ")";
		default:
			break;
		}

		return "<prim:?>";
	}

	std::string operator()(const Load &load) {
		// TODO: cache based on instruction address

		bool inl = inlining;
		fmt::println("load: inl={}", inl);
		std::string value = defer(load.src);
		if (inl)
			return value;

		typeof_vd typer(pool, struct_names);
		std::string type = typer.defer(load.src);
		std::string sym = "s" + std::to_string(generator++);
		return type + " " + sym + " = " + value + ";";
	}

	std::string operator()(const Store &store) {
		std::string lhs = defer(store.dst);
		std::string rhs = defer(store.src);
		return lhs + " = " + rhs + ";";
	}

	std::string operator()(const Cmp &cmp) {
		bool inl = inlining;
		std::string sa = defer(cmp.a, inl);
		std::string sb = defer(cmp.b, inl);
		switch (cmp.type) {
		case Cmp::eq:
			return sa + " == " + sb;
		default:
			break;
		}

		return "<cmp:?>";
	}

	std::string operator()(const Construct &ctor) {
		bool inl = inlining;

		typeof_vd typer(pool, struct_names);
		std::string type = typer.defer(ctor.type);

		int args = ctor.args;
		std::string value = type + "(";
		while (args != -1) {
			op::General g = pool[args];
			if (!g.is <op::List> ())
				abort();

			op::List l = g.as <op::List> ();
			value += defer(l.item, true);

			if (l.next != -1)
				value += ", ";

			args = l.next;
		}

		value += ")";

		if (inl)
			return value;

		std::string sym = "s" + std::to_string(generator++);
		return type + " " + sym + " = " + value + ";";
	}

	template <typename T>
	std::string operator()(const T &) {
		return "<?>";
	}

	std::string defer(int index, bool inl = false) {
		inlining = inl;
		if (symbols.contains(index))
			return symbols[index];

		return std::visit(*this, pool[index]);
	}

	std::string eval(int index) {
		std::string source = defer(index) + "\n";
		std::string space(indentation << 2, ' ');
		indentation = next_indentation;
		return space + source;
	}
};

struct reindex_vd {
	wrapped::hash_table <int, int> &reindex;

	// Non-reindexable
	void operator()(Primitive &) {}
	void operator()(Swizzle &) {}
	void operator()(End &) {}

	// Useful
	void operator()(Global &g) {
		g.type = reindex[g.type];
	}

	void operator()(TypeField &tf) {
		tf.down = reindex[tf.down];
		tf.next = reindex[tf.next];
	}

	void operator()(Cmp &cmp) {
		cmp.a = reindex[cmp.a];
		cmp.b = reindex[cmp.b];
	}

	void operator()(List &list) {
		list.item = reindex[list.item];
		list.next = reindex[list.next];
	}

	void operator()(Construct &ctor) {
		ctor.type = reindex[ctor.type];
		ctor.args = reindex[ctor.args];
	}

	void operator()(Store &store) {
		store.dst = reindex[store.dst];
		store.src = reindex[store.src];
	}

	void operator()(Load &load) {
		load.src = reindex[load.src];
	}

	void operator()(Cond &cond) {
		cond.cond = reindex[cond.cond];
		cond.failto = reindex[cond.failto];
	}

	void operator()(Elif &elif) {
		elif.cond = reindex[elif.cond];
		elif.failto = reindex[elif.failto];
	}

	// void operator()()
};

}

