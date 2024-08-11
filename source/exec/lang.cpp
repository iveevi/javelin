#include <cstdint>
#include <string>
#include <tuple>
#include <vector>
#include <filesystem>
#include <fstream>
#include <array>
#include <stack>
#include <list>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>

#include "wrapped_types.hpp"
#include "lang/token.hpp"

using namespace jvl::lang;

inline const std::string readfile(const std::filesystem::path &path)
{
	std::ifstream f(path);
	if (!f.good()) {
		fmt::println("could find file: {}", path);
		return "";
	}

	std::stringstream s;
	s << f.rdbuf();
	return s.str();
}

// AST structures

// Memory management
struct grand_bank_t {
	// Tracks memory usage from within the compiler
	jvl::wrapped::hash_table <void *, size_t> tracked;
	size_t bytes = 0;

	template <typename T>
	T *alloc(size_t elements) {
		size_t s = sizeof(T) * elements;
		bytes += s;
		T *ptr = (T *) std::malloc(s);
		// T *ptr = new T[elements];
		tracked[ptr] = s;
		return ptr;
	}

	template <typename T>
	void free(T *ptr) {
		if (!tracked.count(ptr)) {
			fmt::println("fatal error: {} not registered", (void *) ptr);
			abort();
		}

		if (tracked[ptr] > bytes) {
			fmt::println("fatal erorr: alloced pointer is registered for more than tracked");
			fmt::println("  grand bank currently holds {} bytes", bytes);
			fmt::println("  address to be freed ({}) tracked for {} bytes", (void *) ptr, tracked[ptr]);
			abort();
		}

		bytes -= tracked[ptr];
		tracked.erase(ptr);
		// delete[] ptr;
		std::free(ptr);
	}

	void dump() {
		fmt::println("Grand Bank:");
		fmt::println("  # of bytes remaining  {}", bytes);
		fmt::println("  # of items in track   {}", tracked.size());
	}

	static grand_bank_t &one() {
		static grand_bank_t gb;
		return gb;
	}
};

template <typename T>
constexpr size_t page_size_v = 16;

// Strings will be longer
template <>
constexpr size_t page_size_v <char> = 32;

template <typename T>
struct bank_t {
	static constexpr size_t page_size = page_size_v <T>;

	using page_mask_t = std::bitset <page_size>;

	struct page {
		T *addr = nullptr;
		page_mask_t used = 0;

		int find_slot(size_t elements) const {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++)
				packet.set(i);

			for (size_t i = 0; i < page_size - elements + 1; i++) {
				page_mask_t offset = packet << i;
				page_mask_t combined = offset & (~used);
				if (combined.count() == elements)
					return i;
			}

			return -1;
		}

		void free_slot(int slot, size_t elements) {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++) {
				addr[slot + i].~T();
				packet.set(i);
			}

			packet <<= slot;
			used &= (~packet);
		}

		void mark_used(int slot, size_t elements) {
			page_mask_t packet;
			for (size_t i = 0; i < elements; i++)
				packet.set(i);

			packet <<= slot;
			used |= packet;
		}

		size_t usage() const {
			return used.count();
		}

		void clear() {
			grand_bank_t::one().free(addr);
			addr = nullptr;
			used = 0;
		}

		static page alloc() {
			page p;
			p.addr = grand_bank_t::one().alloc <T> (page_size);
			p.used = 0;
			return p;
		}
	};

	using track_data_t = std::tuple <page *, int, size_t>;

	jvl::wrapped::hash_table <T *, track_data_t> tracked;

	std::list <page> page_list;

	std::tuple <page *, int> find_slot(size_t elements) {
		for (auto &p : page_list) {
			int slot = p.find_slot(elements);
			if (slot != -1)
				return { &p, slot };
		}

		return { nullptr, -1 };
	}

	T *alloc(size_t elements) {
		if (elements > page_size) {
			fmt::println("fatal error: too many elements requested");
			fmt::println("  requested {} elements", elements);
			fmt::println("  page size is {} elements", page_size);
			abort();
		}

		auto [p, slot] = find_slot(elements);
		bool insufficient = page_list.empty() || (slot == -1);

		if (insufficient)
			page_list.push_back(page::alloc());

		std::tie(p, slot) = find_slot(elements);
		if (slot == -1) {
			fmt::println("fatal error: failed to find slot");
			abort();
		}

		p->mark_used(slot, elements);

		T *ptr = &p->addr[slot];
		tracked[ptr] = std::make_tuple(p, slot, elements);

		return ptr;
	}

	void free(T *ptr) {
		if (!tracked.count(ptr)) {
			fmt::println("fatal error: {} not registered", (void *) ptr);
			abort();
		}

		auto [p, slot, elements] = tracked[ptr];

		// TODO: verify that p exists in debug mode

		p->free_slot(slot, elements);
		tracked.erase(ptr);

		// TODO: allow some dead time?
		// dispatch a thread which will do it if needed
		if (p->usage() == 0) {
			for (auto it = page_list.begin(); it != page_list.end(); it++) {
				if (&(*it) == p) {
					it->clear();
					page_list.erase(it);
					break;
				}
			}
		}
	}

	void clear() {
		for (auto page : page_list)
			page.clear();

		page_list.clear();
	}

	void dump() {
		fmt::println("Bank:");
		fmt::println("  # of pages: {}", page_list.size());

		size_t index = 0;
		for (auto page : page_list)
			fmt::println("  {:3d}: {}", index++, page.used);
	}

	static bank_t &one() {
		static bank_t b;
		return b;
	}
};

template <typename T>
concept weak_type = requires(T &t) {
	{ t.disown() } -> std::same_as <void>;
};

template <typename T>
struct ptr {
	T *address = nullptr;

	ptr(const std::nullptr_t &) : address(nullptr) {}
	ptr(T *address_) : address(address_) {}

	ptr &disowned() {
		address->disown();
		return *this;
	}

	operator bool() const {
		return address;
	}

	T &operator*() {
		return *address;
	}

	const T &operator*() const {
		return *address;
	}

	T *operator->() {
		return address;
	}

	const T *operator->() const {
		return address;
	}

	ptr &operator++(int) {
		address++;
		return *this;
	}

	void free() {
		if (address)
			bank_t <T> ::one().free(address);
		address = nullptr;
	}

	template <typename ... Args>
	static ptr from(const Args &... args) {
		T *p = bank_t <T> ::one().alloc(1);
		*p = T(args...);
		return p;
	}

	static ptr block_from(const std::vector <ptr <T>> &values) {
		T *p = bank_t <T> ::one().alloc(values.size());
		for (size_t i = 0; i < values.size(); i++)
			p[i] = *values[i];
		return p;
	}
};

// State management
struct parse_state {
	std::vector <token> tokens;
	size_t index;

	bool eof() const {
		return index >= tokens.size();
	}

	const token &gett(int lookahead = 0) const {
		return tokens[index + lookahead];
	}

	const token &gettn() {
		return tokens[index++];
	}

	jvl::wrapped::optional <token> gett_match(const token::kind type) {
		if (gett().type == type)
			return gett();

		return std::nullopt;
	}

	jvl::wrapped::optional <token> gettn_match(const token::kind type) {
		if (gett().type == type)
			return gettn();

		return std::nullopt;
	}

	struct save_state {
		parse_state &ref;
		std::string scope;
		size_t prior;
		bool success;

		template <typename S>
		struct free_state {
			ptr <S> tracked = nullptr;
			bool &success;

			~free_state() {
				if (!success)
					tracked.free();
			}
		};

		template <typename S>
		auto release_on_fail(const ptr <S> &t) {
			return free_state <S> { t, success };
		}

		template <typename T>
		const T &ok(const T &t) {
			// TODO: debug only
			// fmt::println("[S] succeeded $({})", scope);
			success = true;
			return t;
		}

		~save_state() {
			if (!success) {
				// TODO: debug only
				// fmt::println("[F] failed $({}), going back to #{}", scope, prior);
				ref.index = prior;
			}
		}
	};

	save_state save(const std::string &scope) {
		return { *this, scope, index, false };
	}
};

// Specification
template <typename T>
concept ast_node_type = requires(parse_state &state) {
	{
		T::attempt(state)
	} -> std::same_as <ptr <T>>;
};

// Simplifying sequences of tokens and ast types
template <typename T = void>
struct payload_ref {
	static constexpr bool valued = true;

	T &ref;
	token::kind type;
};

template <>
struct payload_ref <void> {
	static constexpr bool valued = false;

	token::kind type;
};

template <typename T, typename ... Args>
bool match(parse_state &state, payload_ref <T> &tpr, Args &... args)
{
	auto topt = state.gettn_match(tpr.type);
	if (!topt)
		return false;

	if constexpr (payload_ref <T> ::valued)
		tpr.ref = topt.value().payload.template as <T> ();

	if constexpr (sizeof...(Args)) {
		if (!match(state, args...))
			return false;
	}

	return true;
}

template <ast_node_type T, typename ... Args>
bool match(parse_state &state, ptr <T> &p, Args &... args)
{
	p = T::attempt(state);
	if (!p)
		return false;

	if constexpr (sizeof...(Args)) {
		if (!match(state, args...))
			return false;
	}

	return true;
}

// Printing nicely
#define pp_println(s, ...)						\
	do {								\
		fmt::print("{}", std::string(indent << 1, ' '));	\
		fmt::println(s, ##__VA_ARGS__);				\
	} while (0)

// Forward declarations

// Implementations

struct ast_string {
	size_t size;
	ptr <char> data = nullptr;

	ast_string &operator=(const ast_string &other) {
		if (this != &other) {
			size = other.size;
			data.address = bank_t <char> ::one().alloc(size + 1);
			std::memcpy(data.address, other.data.address, size + 1);
		}

		return *this;
	}

	~ast_string() {
		data.free();
		disown();
	}

	void disown() {
		data = nullptr;
	}

	ast_string(const std::string &s) {
		size = s.size();
		data.address = bank_t <char> ::one().alloc(size + 1);
		std::memcpy(data.address, s.data(), size + 1);
	}

	const char *c_str() const {
		return data.address;
	}
};

// General structure of a namespace:
//     <name #1>.<name #2>. ....
struct ast_namespace {
	size_t count;
	ptr <ast_string> names = nullptr;

	~ast_namespace() {
		names.free();
		disown();
	}

	void disown() {
		names = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("namespace {{", (void *) this);
		auto p = names;
		for (size_t i = 0; i < count; i++, p++)
			pp_println("  name: {}", p->c_str());
		pp_println("}}");
	}

	static ptr <ast_namespace> attempt(parse_state &state) {
		auto save = state.save("namespace");

		if (!state.gett().is(token::misc_identifier))
			return nullptr;

		std::vector <ptr <ast_string>> names;
		while (true) {
			const token &t = state.gettn();
			std::string pstr = t.payload.as <std::string> ();
			names.push_back(ptr <ast_string> ::from(pstr));
			if (!state.gettn_match(token::sign_dot) || !state.gett_match(token::misc_identifier))
				break;
		}

		auto node = ptr <ast_namespace> ::from();
		node->count = names.size();
		node->names = ptr <ast_string> ::block_from(names);

		for (auto name : names)
			name.free();

		return save.ok(node);
	}
};

// General structure of an import:
//     import <namespace>;
struct ast_import {
	ptr <ast_namespace> nspace = nullptr;
	// TODO: sub

	~ast_import() {
		nspace.free();
		disown();
	}

	void disown() {
		nspace = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("import {{", (void *) this);
		nspace->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_import> attempt(parse_state &state) {
		auto save = state.save("import");

		auto node = ptr <ast_import> ::from();
		auto free_node = save.release_on_fail(node);

		auto ref_kw = payload_ref <> (token::keyword_import);
		auto ref_end = payload_ref <> (token::sign_semicolon);
		if (!match(state, ref_kw, node->nspace, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_attribute {
	// TODO: arguments as well
	ptr <ast_namespace> nspace = nullptr;

	~ast_attribute() {
		nspace.free();
		disown();
	}

	void disown() {
		nspace = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("attribute {{");
		nspace->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_attribute> attempt(parse_state &state) {
		auto save = state.save("attribute");

		auto node = ptr <ast_attribute> ::from();
		auto ref = payload_ref <> (token::sign_attribute);
		if (!match(state, ref, node->nspace))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_type {
	// TODO: generics
	ptr <ast_namespace> nspace = nullptr;

	~ast_type() {
		nspace.free();
		disown();
	}

	void disown() {
		nspace = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("type:");
		nspace->dump(indent + 1);
	}

	static ptr <ast_type> attempt(parse_state &state) {
		auto save = state.save("type");

		auto node = ptr <ast_type> ::from();
		auto free_node = save.release_on_fail(node);
		if (!match(state, node->nspace))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_decl_using {
	ptr <ast_namespace> nspace = nullptr;

	~ast_decl_using() {
		nspace.free();
		disown();
	}

	void disown() {
		nspace = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("decl using:");
		nspace->dump(indent + 1);
	}

	static ptr <ast_decl_using> attempt(parse_state &state) {
		auto save = state.save("decl: using");

		auto node = ptr <ast_decl_using> ::from();
		auto ref_kw = payload_ref <> (token::keyword_using);
		auto ref_end = payload_ref <> (token::sign_semicolon);

		auto free_node = save.release_on_fail(node);

		if (!match(state, ref_kw, node->nspace, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_factor;

struct ast_expression {
	// TODO: extend
	ptr <ast_factor> factor = nullptr;

	~ast_expression() {
		factor.free();
		disown();
	}

	void disown() {
		factor = nullptr;
	}

	void dump(int indent = 0);

	static ptr <ast_expression> attempt(parse_state &);
};

struct ast_pseudo_expression_list {
	size_t count;
	ptr <ast_expression> list = nullptr;

	static ptr <ast_pseudo_expression_list> attempt(parse_state &state) {
		auto save = state.save("pseudo: expressions");

		std::vector <ptr <ast_expression>> exprs;
		while (true) {
			auto expr = ast_expression::attempt(state);
			if (!expr)
				break;

			exprs.push_back(expr);

			if (!state.gett_match(token::sign_comma))
				break;

			state.gettn();
		}

		auto node = ptr <ast_pseudo_expression_list> ::from();
		node->count = exprs.size();
		node->list = ptr <ast_expression> ::block_from(exprs);

		for (auto expr : exprs)
			expr.disowned().free();

		return save.ok(node);
	}
};

struct ast_argument_list {
	size_t count;
	ptr <ast_expression> list = nullptr;

	~ast_argument_list() {
		list.free();
		disown();
	}

	void disown() {
		list = nullptr;
	}

	static ptr <ast_argument_list> attempt(parse_state &state) {
		auto save = state.save("pseudo: arguments");

		auto begin = payload_ref <> (token::sign_parenthesis_left);
		auto end = payload_ref <> (token::sign_parenthesis_right);
		ptr <ast_pseudo_expression_list> stmts = nullptr;

		auto free_stmts = save.release_on_fail(stmts);

		if (!match(state, begin, stmts, end))
			return nullptr;

		auto node = ptr <ast_argument_list> ::from();
		node->count = stmts->count;
		node->list = stmts->list;

		stmts.free();

		return save.ok(node);
	}
};

// TODO: refactor to call... <factor>(args...)
struct ast_factor_construct {
	ptr <ast_type> type = nullptr;
	ptr <ast_argument_list> args = nullptr;

	~ast_factor_construct() {
		type.free();
		args.free();
		disown();
	}

	void disown() {
		type = nullptr;
		args = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("construct {{");
		type->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_factor_construct> attempt(parse_state &state) {
		auto save = state.save("factor: construct");

		auto node = ptr <ast_factor_construct> ::from();
		auto free_node = save.release_on_fail(node);
		if (!match(state, node->type, node->args))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_factor_literal {
	token_payload_t value;

	void dump(int indent = 0) {
		if (auto i = value.get <int> ())
			pp_println("int: {}", *i);
		else if (auto i = value.get <float> ())
			pp_println("float: {}", *i);
		else if (auto i = value.get <std::string> ())
			pp_println("string: {}", *i);
		else
			pp_println("literal: ??");
	}

	static ptr <ast_factor_literal> attempt(parse_state &state) {
		auto save = state.save("factor: literal");

		auto node = ptr <ast_factor_literal> ::from();
		if (auto val = state.gettn_match(token::literal_int))
			node->value = val->payload.as <int> ();
		else if (auto val = state.gettn_match(token::literal_float))
			node->value = val->payload.as <float> ();
		else if (auto val = state.gettn_match(token::literal_string))
			node->value = val->payload.as <std::string> ();
		else
			return nullptr;

		return save.ok(node);
	}
};

struct ast_factor {
	ptr <ast_factor_construct> construct = nullptr;
	// TODO: should be semantically different
	ptr <ast_namespace> nspace = nullptr;
	ptr <ast_factor_literal> literal = nullptr;

	~ast_factor() {
		construct.free();
		nspace.free();
		literal.free();
		disown();
	}

	void disown() {
		construct = nullptr;
		nspace = nullptr;
		literal = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("factor:");
		if (construct)
			construct->dump(indent + 1);
		if (nspace)
			nspace->dump(indent + 1);
		if (literal)
			literal->dump(indent + 1);
	}

	static ptr <ast_factor> attempt(parse_state &state) {
		auto save = state.save("factor");

		auto node = ptr <ast_factor> ::from();
		if (match(state, node->construct))
			return save.ok(node);

		if (match(state, node->nspace))
			return save.ok(node);

		if (match(state, node->literal))
			return save.ok(node);

		return nullptr;
	}
};

void ast_expression::dump(int indent)
{
	pp_println("expression:");
	factor->dump(indent + 1);
}

ptr <ast_expression> ast_expression::attempt(parse_state &state)
{
	auto save = state.save("expression");

	auto node = ptr <ast_expression> ::from();
	if (match(state, node->factor))
		return save.ok(node);

	fmt::println("unable to parse expression: {}", state.gett());

	return nullptr;
}

struct ast_decl_assign {
	ptr <ast_namespace> dst = nullptr;
	ptr <ast_expression> value = nullptr;

	~ast_decl_assign() {
		dst.free();
		value.free();
		disown();
	}

	void disown() {
		dst = nullptr;
		value = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("decl assign {{");
		dst->dump(indent + 1);
		value->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_decl_assign> attempt(parse_state &state) {
		auto save = state.save("decl: assign");

		auto node = ptr <ast_decl_assign> ::from();
		auto ref_eq = payload_ref <> (token::sign_equals);
		auto ref_end = payload_ref <> (token::sign_semicolon);

		auto free_node = save.release_on_fail(node);

		if (!match(state, node->dst, ref_eq, node->value, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_decl_return {
	ptr <ast_expression> value = nullptr;

	~ast_decl_return() {
		value.free();
		disown();
	}

	void disown() {
		value = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("decl return {{", (void *) this);
		value->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_decl_return> attempt(parse_state &state) {
		auto save = state.save("decl: return");

		auto node = ptr <ast_decl_return> ::from();
		auto ref_kw = payload_ref <> (token::keyword_return);
		auto ref_end = payload_ref <> (token::sign_semicolon);

		auto free_node = save.release_on_fail(node);

		if (!match(state, ref_kw, node->value, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_statement {
	// TODO: variant_ptr <...>
	ptr <ast_decl_using> decl_using = nullptr;
	ptr <ast_decl_assign> decl_assign = nullptr;
	ptr <ast_decl_return> decl_return = nullptr;

	~ast_statement() {
		decl_using.free();
		decl_assign.free();
		decl_return.free();
		disown();
	}

	void disown() {
		decl_using = nullptr;
		decl_assign = nullptr;
		decl_return = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("statement:");

		if (decl_using)
			decl_using->dump(indent + 1);

		if (decl_assign)
			decl_assign->dump(indent + 1);

		if (decl_return)
			decl_return->dump(indent + 1);
	}

	static ptr <ast_statement> attempt(parse_state &state) {
		auto save = state.save("statement");

		auto node = ptr <ast_statement> ::from();
		auto free_statement = save.release_on_fail(node);

		if (match(state, node->decl_using))
			return save.ok(node);

		if (match(state, node->decl_assign))
			return save.ok(node);

		if (match(state, node->decl_return))
			return save.ok(node);

		return nullptr;
	}
};

struct ast_pseudo_statement_list {
	size_t count;
	ptr <ast_statement> stmts = nullptr;

	static ptr <ast_pseudo_statement_list> attempt(parse_state &state) {
		auto save = state.save("pseudo: statements");

		std::vector <ptr <ast_statement>> stmts;
		while (auto stmt = ast_statement::attempt(state))
			stmts.push_back(stmt);

		auto node = ptr <ast_pseudo_statement_list> ::from();
		node->count = stmts.size();
		node->stmts = ptr <ast_statement> ::block_from(stmts);

		for (auto stmt : stmts)
			stmt.disowned().free();

		return save.ok(node);
	}
};

// General structure of a block:
//      <stmt #1>
//      <stmt #2>
//      ...
struct ast_block {
	size_t count;
	ptr <ast_statement> stmts = nullptr;

	~ast_block() {
		stmts.free();
	}

	void disown() {
		stmts = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("block:");
		pp_println("  # of statements: {}", count);

		auto p = stmts;
		for (size_t i = 0; i < count; i++, p++)
			p->dump(indent + 1);
	}

	static ptr <ast_block> attempt(parse_state &state) {
		auto save = state.save("block");

		auto begin = payload_ref <> (token::sign_brace_left);
		auto end = payload_ref <> (token::sign_brace_right);
		ptr <ast_pseudo_statement_list> stmts = nullptr;

		auto free_stmts = save.release_on_fail(stmts);

		if (!match(state, begin, stmts, end))
			return nullptr;

		auto node = ptr <ast_block> ::from();
		node->count = stmts->count;
		node->stmts = stmts->stmts;

		stmts.free();

		return save.ok(node);
	}
};

struct ast_parameter {
	ptr <ast_string> name = nullptr;
	ptr <ast_type> type = nullptr;

	~ast_parameter() {
		name.free();
		type.free();
		disown();
	}

	void disown() {
		name = nullptr;
		type = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("parameter {{");
		pp_println("  name: {}", name->c_str());
		type->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_parameter> attempt(parse_state &state) {
		auto save = state.save("parameter");

		std::string name;

		auto node = ptr <ast_parameter> ::from();
		auto ref = payload_ref <std::string> (name, token::misc_identifier);
		if (!match(state, node->type, ref))
			return nullptr;

		node->name = ptr <ast_string> ::from(name);

		return save.ok(node);
	}
};

struct ast_pseudo_return {
	ptr <ast_type> type = nullptr;

	static ptr <ast_pseudo_return> attempt(parse_state &state) {
		auto save = state.save("pseudo: return");

		auto node = ptr <ast_pseudo_return> ::from();
		auto ref = payload_ref <> (token::sign_arrow);
		if (!match(state, ref, node->type))
			return nullptr;

		return save.ok(node);
	}
};

// Pseudo AST node; not actually stored
// General structure of a (pseudo) parameter group:
//     (<parameter #1>, <parameter #2>, ....)
struct ast_pseudo_parameter_group {
	size_t nargs;
	ptr <ast_parameter> parameters = nullptr;

	static ptr <ast_pseudo_parameter_group> attempt(parse_state &state) {
		auto save = state.save("pseudo: parameters");

		// TODO: pseudo_parameter_list
		if (!state.gettn_match(token::sign_parenthesis_left))
			return nullptr;

		std::vector <ptr <ast_parameter>> params;
		while (auto param = ast_parameter::attempt(state)) {
			params.push_back(param);
			if (!state.gettn_match(token::sign_comma))
				break;
		}

		if (!state.gettn_match(token::sign_parenthesis_right))
			return nullptr;

		auto node = ptr <ast_pseudo_parameter_group> ::from();
		node->nargs = params.size();
		node->parameters = ptr <ast_parameter> ::block_from(params);

		for (auto param : params)
			param.disowned().free();

		return save.ok(node);
	}
};

// General structure of a function:
//     <attributes>...
//     ftn <name> (<arguments>...) -> <return_type>
//     {
//         <block>
//     }
struct ast_function {
	size_t nargs;
	size_t nattrs;
	ptr <ast_string> name = nullptr;
	// TODO: attribute list
	ptr <ast_attribute> attributes = nullptr;
	ptr <ast_type> return_type = nullptr;
	ptr <ast_parameter> parameters = nullptr;
	ptr <ast_block> block = nullptr;

	~ast_function() {
		name.free();
		attributes.free();
		return_type.free();
		parameters.free();
		block.free();
		disown();
	}

	void disown() {
		name = nullptr;
		attributes = nullptr;
		return_type = nullptr;
		parameters = nullptr;
		block = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("function:");
		pp_println("  name: {}", name->c_str());
		pp_println("  # of attributes: {}", nattrs);
		pp_println("  # of arguments: {}", nargs);

		if (attributes)
			attributes->dump(indent + 1);

		pp_println("  return type:");
		if (return_type)
			return_type->dump(indent + 2);

		pp_println("  parameters:");
		auto p = parameters;
		for (size_t i = 0; i < nargs; i++, p++)
			p->dump(indent + 2);

		block->dump(indent + 1);
	}

	static ptr <ast_function> attempt(parse_state &state) {
		auto save = state.save("function");

		std::string name;

		auto node = ptr <ast_function> ::from();
		auto ref_kw = payload_ref <> (token::keyword_function);
		auto ref_name = payload_ref <std::string> (name, token::misc_identifier);
		ptr <ast_pseudo_parameter_group> p_args_group =  nullptr;
		ptr <ast_pseudo_return> p_return_type = nullptr;

		auto free_p_args_group = save.release_on_fail(p_args_group);
		auto free_p_return_type = save.release_on_fail(p_return_type);

		auto success =  match(state,
			/* begin */
			node->attributes,  // attribute
			ref_kw,            // keyword 'ftn'
			ref_name,          // function name
			p_args_group,      // parameters
			p_return_type,     // return type
			node->block        // function block statements
			/* end */ );

		if (!success)
			return nullptr;

		node->nargs = p_args_group->nargs;
		node->parameters = p_args_group->parameters;
		node->return_type = p_return_type->type;
		node->name = ptr <ast_string> ::from(name);

		p_args_group.free();
		p_return_type.free();

		return save.ok(node);
	}
};

// General structure of a global declaration:
//     <import>
//     or
//     <function>
struct ast_global_decl {
	ptr <ast_import> decl_import = nullptr;
	ptr <ast_function> decl_function = nullptr;

	~ast_global_decl() {
		decl_function.free();
		decl_import.free();
		disown();
	}

	void disown() {
		decl_import = nullptr;
		decl_function = nullptr;
	}

	void dump(int indent = 0) {
		pp_println("global declaration {{");

		if (decl_import)
			decl_import->dump(indent + 1);

		if (decl_function)
			decl_function->dump(indent + 1);

		pp_println("}}");
	}

	static ptr <ast_global_decl> attempt(parse_state &state) {
		auto save = state.save("decl: global");

		auto node = ptr <ast_global_decl> ::from();
		if (match(state, node->decl_import))
			return save.ok(node);

		if (match(state, node->decl_function))
			return save.ok(node);

		return nullptr;
	}
};

// General struture of a module
//     <global decl #1>
//     <global decl #2>
//     ...
struct ast_module {
	size_t count;
	ptr <ast_global_decl> decls = nullptr;

	~ast_module() {
		decls.free();
		decls = nullptr;
	}

	void dump(int indent = 0) {
		std::string spacing(indent << 1, ' ');

		pp_println("module {{");
		pp_println("  # of declarations: {}", count);

		auto p = decls;
		for (size_t i = 0; i < count; i++, p++)
			p->dump(indent + 1);

		pp_println("}}");
	}

	static ptr <ast_module> attempt(parse_state &state) {
		auto save = state.save("module");

		std::vector <ptr <ast_global_decl>> decls;

		while (!state.eof()) {
			auto decl = ast_global_decl::attempt(state);
			if (decl)
				decls.push_back(decl);
			else
				return nullptr;
		}

		auto node = ptr <ast_module> ::from();
		node->count = decls.size();
		node->decls = ptr <ast_global_decl> ::block_from(decls);

		for (auto &decl : decls)
			decl.disowned().free();

		return save.ok(node);
	}
};

int main()
{
	std::filesystem::path path = "include/lang/proto/triangle.jvl";
	std::string source = readfile(path);
	fmt::println("{}", source);

	parse_state ps(lexer(source), 0);

	auto module = ast_module::attempt(ps);

	module->dump();

	grand_bank_t::one().dump();

	module.free();

	grand_bank_t::one().dump();
}
