#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/kernel.hpp"
#include "thunder/buffer.hpp"
#include "logging.hpp"
#include "wrapped_types.hpp"

namespace jvl::thunder {

MODULE(buffer);

Buffer::Buffer() : pointer(0), pool(4) {}

index_t Buffer::emit(const Atom &atom)
{
	if (pointer >= pool.size())
		pool.resize((pool.size() << 1));

	JVL_ASSERT(pointer < pool.size(),
		"scratch buffer pointer is out of bounds ({} >= {})",
		pointer, pool.size());

	pool[pointer] = atom;
	if (auto td = classify(atom))
		types[pointer] = td;
	
	return pointer++;
}
	
// TODO: binary operation specialization
// TODO: separate source file
struct overload {
	TypeDecl result;
	std::vector <TypeDecl> args;

	template <typename ... Args>
	static overload from(TypeDecl result, const Args &...args) {
		std::vector <TypeDecl> list { args... };
		return overload(result, list);
	}

	static std::string type_decls_to_string(const std::vector <TypeDecl> &args) {
		std::string result = "(";
		for (size_t i = 0; i < args.size(); i++) {
			auto &td = args[0];

			result += td.to_string();
			if (i + 1 < args.size())
				result += ", ";
		}

		return result + ")";
	}
};
	
using overload_list = std::vector <overload>;

template <typename T>
struct overload_table : public wrapped::hash_table <T, overload_list> {
	using wrapped::hash_table <T, overload_list> ::hash_table;

	TypeDecl lookup(const T &key, const std::vector <TypeDecl> &args) const {
		const char *const *name_table = nullptr;
		if constexpr (std::same_as <T, IntrinsicOperation>)
			name_table = tbl_intrinsic_operation;
		else if constexpr (std::same_as <T, OperationCode>)
			name_table = tbl_operation_code;

		JVL_ASSERT(this->contains(key),
			"overload table does not contain entry for ${}",
			name_table[key]);

		auto &overloads = this->at(key);
		for (auto &ovl : overloads) {
			if (ovl.args.size() != args.size())
				continue;

			bool success = true;
			for (size_t i = 0; i < args.size(); i++) {
				if (ovl.args[i] != args[i]) {
					success = false;
					break;
				}
			}

			if (success)
				return ovl.result;
		}

		JVL_WARNING("no matching overload {} found for ${}",
			overload::type_decls_to_string(args),
			name_table[key]);

		// TODO: note diagnostics
		JVL_INFO("available overloads for ${}:", name_table[key]);
		for (auto &ovl : overloads) {
			fmt::println("  {} -> {}",
				overload::type_decls_to_string(ovl.args),
				ovl.result.to_string());
		}

		return TypeDecl();
	}
};

TypeDecl Buffer::classify(const Atom &original) const
{
	static const overload_table <IntrinsicOperation> intrinsic_overloads {
		{ acos, {
			overload::from(f32, f32)
		} },

		{ dot, {
			overload::from(f32, vec2, vec2),
			overload::from(f32, vec3, vec3),
			overload::from(f32, vec4, vec4),
		} },
	};

	Atom atom = original;

	index_t index = -1;
	while (true) {
		switch (atom.index()) {

		case Atom::type_index <TypeField> ():
		{
			auto &type_field = atom.as <TypeField> ();
			if (type_field.item != bad)
				return TypeDecl(type_field.item);

			return TypeDecl();
		}

		case Atom::type_index <Primitive> ():
			return TypeDecl(atom.as <Primitive> ().type);

		case Atom::type_index <Global> ():
		{
			index = atom.as <Global> ().type;
			atom = pool[index];
		} break;

		case Atom::type_index <Intrinsic> ():
		{
			auto &intrinsic = atom.as <Intrinsic> ();
			auto args = expand_list_types(intrinsic.args);
			auto result = intrinsic_overloads.lookup(intrinsic.opn, args);
			return result;
		}

		default:
			return TypeDecl();

		}
	}

	return TypeDecl();
}

// Kernel transfer
Kernel Buffer::export_to_kernel() const
{
	// TODO: export the kernel, then optimize and dce, and then validate...
	// validate();

	// TODO: run through optimizations

	thunder::Kernel kernel(thunder::Kernel::eAll);
	kernel.atoms = pool;
	kernel.atoms.resize(pointer);

	return kernel;
}

// Validation for kernels
void Buffer::validate() const
{
	// Phase 1: Verify that layout IOs are consistent
	// TODO: recursive IR cmp

	wrapped::hash_table <index_t, index_t> inputs;
	wrapped::hash_table <index_t, index_t> outputs;

	for (int i = 0; i < pointer; i++) {
		thunder::Atom g = pool[i];
		if (!g.is <thunder::Global>())
			continue;

		thunder::Global global = g.as <thunder::Global> ();
		if (global.qualifier == thunder::layout_in) {
			int type = global.type;
			int binding = global.binding;
			if (inputs.count(binding) && inputs[binding] != type)
				fmt::println("JVL (error): layout in type conflict with binding #{}", binding);
			else
				inputs[binding] = type;
		} else if (global.qualifier == thunder::layout_out) {
			int type = global.type;
			int binding = global.binding;
			if (outputs.count(binding) && outputs[binding] != type)
				fmt::println("JVL (error): layout out type conflict with binding #{}", binding);
			else
				outputs[binding] = type;
		}
	}
}

void Buffer::clear()
{
	pointer = 0;
	pool.clear();
	pool.resize(4);
}

void Buffer::dump()
{
	for (size_t i = 0; i < pointer; i++) {
		std::string type_string = types.get(i).value_or(TypeDecl()).to_string();
		fmt::println("   [{:4d}]: {:30} :: {}", i, pool[i].to_string(), type_string);
	}
}

// Utility methods
Atom &Buffer::operator[](size_t i)
{
	return pool[i];
}

const Atom &Buffer::operator[](size_t i) const
{
	return pool[i];
}

std::vector <index_t> Buffer::expand_list(index_t i) const
{
	std::vector <index_t> args;
	while (i != -1) {
		auto &atom = pool[i];
		assert(atom.is <List> ());

		List list = atom.as <List> ();
		args.push_back(list.item);

		i = list.next;
	}

	return args;
}

std::vector <TypeDecl> Buffer::expand_list_types(index_t i) const
{
	std::vector <TypeDecl> args;
	while (i != -1) {
		auto &atom = pool[i];
		assert(atom.is <List> ());

		List list = atom.as <List> ();

		if (types.contains(list.item))
			args.push_back(types.at(list.item));
		else
			args.push_back(TypeDecl());

		i = list.next;
	}

	return args;
}

} // namespace jvl::thunder
