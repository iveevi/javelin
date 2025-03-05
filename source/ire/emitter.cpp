#include <cassert>
#include <unordered_map>
#include <unordered_set>

#include <fmt/core.h>

#include "common/logging.hpp"
#include "common/io.hpp"

#include "ire/emitter.hpp"
#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::ire {

MODULE(emitter);

thunder::Buffer &Emitter::buffer()
{
	JVL_ASSERT(scopes.size(), "no active buffer scope in emitter");
	return scopes.top().get();
}

// Scope management
void Emitter::push(thunder::Buffer &scratch, bool enable_classify)
{
	scopes.push(std::ref(scratch));
	classify.push(enable_classify);
	// JVL_INFO("pushed new scratch buffer to global emitter ({} scopes)", scopes.size());
}

void Emitter::pop()
{
	scopes.pop();
	classify.pop();
	// JVL_INFO("popped scratch buffer from global emitter ({} scopes)", scopes.size());
}

// Emitting instructions during function invocation
Emitter::Index Emitter::emit(const thunder::Atom &atom)
{
	JVL_ASSERT(scopes.size(), "in emit: no active scope");
	return scopes.top().get().emit(atom, classify.top());
}

// Qualifiers
Emitter::Index Emitter::emit_qualifier(Index underlying, Index numerical, thunder::QualifierKind kind)
{
	return emit(thunder::Qualifier(underlying, numerical, kind));
}

// Type information
Emitter::Index Emitter::emit_type_information(Index down, Index next, thunder::PrimitiveType item)
{
	return emit(thunder::TypeInformation(down, next, item));
}

// Primitives
Emitter::Index Emitter::emit_primitive(bool value)
{
	thunder::Primitive primitive;
	primitive.bdata = value;
	primitive.type = thunder::boolean;
	return emit(primitive);
}

Emitter::Index Emitter::emit_primitive(int32_t value)
{
	thunder::Primitive primitive;
	primitive.idata = value;
	primitive.type = thunder::i32;
	return emit(primitive);
}

Emitter::Index Emitter::emit_primitive(uint32_t value)
{
	thunder::Primitive primitive;
	primitive.udata = value;
	primitive.type = thunder::u32;
	return emit(primitive);
}

Emitter::Index Emitter::emit_primitive(float value)
{
	thunder::Primitive primitive;
	primitive.fdata = value;
	primitive.type = thunder::f32;
	return emit(primitive);
}

// Swizzles
Emitter::Index Emitter::emit_swizzle(Index src, thunder::SwizzleCode code)
{
	return emit(thunder::Swizzle(src, code));
}

// Operations
Emitter::Index Emitter::emit_operation(Index a, Index b, thunder::OperationCode code)
{
	return emit(thunder::Operation(a, b, code));
}

// Intrinsics
Emitter::Index Emitter::emit_intrinsic(Index args, thunder::IntrinsicOperation opn)
{
	return emit(thunder::Intrinsic(args, opn));
}

// List chains
Emitter::Index Emitter::emit_list(Index item, Index next)
{
	return emit(thunder::List(item, next));
}

// Constructions
Emitter::Index Emitter::emit_construct(Index type, Index args, thunder::ConstructorMode mode)
{
	return emit(thunder::Construct(type, args, mode));
}

// Invocations
Emitter::Index Emitter::emit_call(Index cid, Index args, Index type)
{
	return emit(thunder::Call(cid, args, type));
}

// Stores
Emitter::Index Emitter::emit_store(Index dst, Index src)
{
	return emit(thunder::Store(dst, src));
}

// Loads
Emitter::Index Emitter::emit_load(Index src, Index idx)
{
	return emit(thunder::Load(src, idx));
}

// Indexing
Emitter::Index Emitter::emit_array_access(Index src, Index loc)
{
	return emit(thunder::ArrayAccess(src, loc));
}

// Branches
Emitter::Index Emitter::emit_branch(Index cond, Index failto, thunder::BranchKind kind, const precondition &pre)
{
	return emit(thunder::Branch(cond, failto, kind), pre);
}

Emitter::Index Emitter::emit(const thunder::Branch &branch, const precondition &pre)
{
	auto &buffer = scopes.top().get();

	Index i = -1;
	if (branch.kind != thunder::control_flow_end)
		i = emit((thunder::Atom) branch);

	switch (branch.kind) {

	case thunder::conditional_if:
		control_flow_ends.push(cf_await(i));
		return i;

	case thunder::conditional_else:
	case thunder::conditional_else_if:
	{
		auto await = control_flow_ends.top();
		control_flow_ends.pop();

		auto &prior = buffer[await.ref];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;

		if (branch.kind != thunder::conditional_else)
			control_flow_ends.push(cf_await(i));

		// else branch should not be waiting...
	} return i;

	case thunder::loop_while:
		control_flow_ends.push(cf_await(i, pre));
		return i;

	case thunder::control_flow_end:
	{
		auto await = control_flow_ends.top();
		control_flow_ends.pop();

		if (await.pre)
			await.pre.value()();

		i = emit((thunder::Atom) branch);

		auto &prior = buffer[await.ref];
		auto &branch = prior.as <thunder::Branch> ();
		branch.failto = i;
	} return i;

	// These two are simply keywords
	case thunder::control_flow_skip:
	case thunder::control_flow_stop:
		return i;

	default:
		break;
	}

	JVL_ABORT("unhandled case of control_flow_callback: {}", branch);
}
	
// Returns
Emitter::Index Emitter::emit_return(Index value)
{
	return emit(thunder::Return(value));
}

// List chain construction
Emitter::Index Emitter::emit_list_chain(const std::vector <Index> &atoms)
{
	thunder::List list;

	Index next = -1;
	for (auto it = std::rbegin(atoms); it != std::rend(atoms); it++) {
		list.item = *it;
		list.next = next;
		next = emit(list);
	}

	return next;
}

// Emitting type hints
void Emitter::add_type_hint(Index idx, uint64_t id, const std::string &name, const std::vector <std::string> &fields)
{
	auto &buf = buffer();
	// TODO: sanity check for type related?
	if (!buf.decorations.all.contains(id)) {
		auto th = thunder::Buffer::type_hint(name, fields);
		buf.decorations.all[id] = th;
	}

	buf.decorations.used[idx] = id;
}

// Emitting type phantoms hints
void Emitter::add_phantom_hint(Index idx)
{
	auto &buf = buffer();
	auto &target = buf[idx];

	JVL_BUFFER_DUMP_ON_ASSERT(target.is <thunder::TypeInformation> (),
		"phantom hints can only be applied "
		"to type information atoms:\n{}", target);

	buf.decorations.phantom.insert(idx);
}

// Printing the IR state
void Emitter::display_assembly()
{
	JVL_ASSERT(scopes.size(), "no active scope in {}", __FUNCTION__);

	auto &buf = buffer();
	
	fmt::println("__emitter.{}:", scopes.size());
	buf.display_pretty();
}

void Emitter::display_pretty()
{
	JVL_ASSERT(scopes.size(), "no active scope in {}", __FUNCTION__);

	auto &buf = buffer();
	
	jvl::io::header(fmt::format("BUFFER IN PROGRESS ({}/{})", buf.pointer, buf.atoms.size()), 50);
	buf.display_pretty();
}

thread_local Emitter Emitter::active;

} // namespace jvl::ire