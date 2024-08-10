#include <tuple>
#include <vector>
#include <filesystem>
#include <fstream>
#include <array>
#include <stack>

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

// State management
struct parse_state {
	std::vector <token> tokens;
	int index;

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
		int prior;
		bool success;

		template <typename T>
		const T &ok(const T &t) {
			// TODO: success message
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

// Memory management
struct grand_bank_t {
	// Tracks memory usage from within the compiler
	size_t bytes = 0;

	static grand_bank_t &one() {
		static grand_bank_t gb;
		return gb;
	}
};

template <typename T>
struct bank_t {
	// TODO: allocate in page chunks
};

template <typename T>
struct ptr {
	T *address;

	ptr(const std::nullptr_t &) : address(nullptr) {}
	ptr(T *address_) : address(address_) {}

	operator bool() const {
		return address;
	}

	T &operator*() {
		return *address;
	}

	T *operator->() {
		return address;
	}

	ptr &operator++(int) {
		address++;
		return *this;
	}

	template <typename ... Args>
	static ptr from(const Args &... args) {
		// TODO: use the bank allocator
		T *p = new T(args...);
		return p;
	}

	static ptr block_from(const std::vector <T> &values) {
		T *p = new T[values.size()];
		for (size_t i = 0; i < values.size(); i++)
			p[i] = values[i];
		return p;
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

// General structure of a namespace:
//     <name #1>.<name #2>. ....
struct ast_namespace {
	std::vector <std::string> names;

	void dump(int indent = 0) {
		pp_println("namespace {{");
		for (auto s : names)
			pp_println("  name: {}", s);
		pp_println("}}");
	}

	static ptr <ast_namespace> attempt(parse_state &state) {
		auto save = state.save("namespace");

		if (!state.gett().is(token::misc_identifier))
			return nullptr;

		auto node = ptr <ast_namespace> ::from();
		while (true) {
			const token &t = state.gettn();
			node->names.push_back(t.payload.as <std::string> ());
			if (!state.gettn_match(token::sign_dot) || !state.gett_match(token::misc_identifier))
				break;
		}

		return save.ok(node);
	}
};

// General structure of an import:
//     import <namespace>;
struct ast_import {
	ptr <ast_namespace> nspace = nullptr;
	// TODO: sub

	void dump(int indent = 0) {
		pp_println("import {{");
		nspace->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_import> attempt(parse_state &state) {
		auto save = state.save("import");

		auto node = ptr <ast_import> ::from();
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

	void dump(int indent = 0) {
		pp_println("type:");
		nspace->dump(indent + 1);
	}

	static ptr <ast_type> attempt(parse_state &state) {
		auto save = state.save("type");

		auto node = ptr <ast_type> ::from();
		if (!match(state, node->nspace))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_decl_using {
	ptr <ast_namespace> nspace = nullptr;

	void dump(int indent = 0) {
		pp_println("decl using:");
		nspace->dump(indent + 1);
	}

	static ptr <ast_decl_using> attempt(parse_state &state) {
		auto save = state.save("decl: using");

		auto node = ptr <ast_decl_using> ::from();
		auto ref_kw = payload_ref <> (token::keyword_using);
		auto ref_end = payload_ref <> (token::sign_semicolon);
		if (!match(state, ref_kw, node->nspace, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_factor;

struct ast_expression {
	// TODO: extend
	ptr <ast_factor> factor = nullptr;

	void dump(int indent = 0);

	static ptr <ast_expression> attempt(parse_state &);
};

struct ast_pseudo_expression_list {
	size_t count;
	ptr <ast_expression> list = nullptr;

	static ptr <ast_pseudo_expression_list> attempt(parse_state &state) {
		auto save = state.save("pseudo: expressions");

		std::vector <ast_expression> exprs;
		while (true) {
			auto expr = ast_expression::attempt(state);
			if (!expr)
				break;

			exprs.push_back(*expr);

			if (!state.gett_match(token::sign_comma))
				break;

			state.gettn();
		}

		auto node = ptr <ast_pseudo_expression_list> ::from();
		node->count = exprs.size();
		node->list = ptr <ast_expression> ::block_from(exprs);

		return save.ok(node);
	}
};

struct ast_pseudo_arguments {
	size_t count;
	ptr <ast_expression> list = nullptr;

	static ptr <ast_pseudo_arguments> attempt(parse_state &state) {
		auto save = state.save("pseudo: arguments");

		auto begin = payload_ref <> (token::sign_parenthesis_left);
		auto end = payload_ref <> (token::sign_parenthesis_right);
		ptr <ast_pseudo_expression_list> stmts = nullptr;

		if (!match(state, begin, stmts, end))
			return nullptr;

		auto node = ptr <ast_pseudo_arguments> ::from();
		node->count = stmts->count;
		node->list = stmts->list;

		return save.ok(node);
	}
};

// TODO: refactor to call... <factor>(args...)
struct ast_factor_construct {
	ptr <ast_type> type = nullptr;

	// TODO: should be list...
	ptr <ast_pseudo_arguments> args = nullptr;

	void dump(int indent = 0) {
		pp_println("construct {{");
		type->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_factor_construct> attempt(parse_state &state) {
		auto save = state.save("factor: construct");

		auto node = ptr <ast_factor_construct> ::from();
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
		if (!match(state, node->dst, ref_eq, node->value, ref_end))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_decl_return {
	ptr <ast_expression> value = nullptr;

	void dump(int indent = 0) {
		pp_println("decl return {{");
		value->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_decl_return> attempt(parse_state &state) {
		auto save = state.save("decl: return");

		auto node = ptr <ast_decl_return> ::from();
		auto ref_kw = payload_ref <> (token::keyword_return);
		auto ref_end = payload_ref <> (token::sign_semicolon);
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
		fmt::println("statement:");
		fmt::println("  next token: {}", state.gett());

		auto node = ptr <ast_statement> ::from();
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

		std::vector <ast_statement> stmts;
		while (auto stmt = ast_statement::attempt(state))
			stmts.push_back(*stmt);

		auto node = ptr <ast_pseudo_statement_list> ::from();
		node->count = stmts.size();
		node->stmts = ptr <ast_statement> ::block_from(stmts);

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

		if (!match(state, begin, stmts, end))
			return nullptr;

		auto node = ptr <ast_block> ::from();
		node->count = stmts->count;
		node->stmts = stmts->stmts;

		return save.ok(node);
	}
};

struct ast_parameter {
	std::string name;
	ptr <ast_type> type = nullptr;

	void dump(int indent = 0) {
		pp_println("parameter {{");
		pp_println("  name: {}", name);
		type->dump(indent + 1);
		pp_println("}}");
	}

	static ptr <ast_parameter> attempt(parse_state &state) {
		auto save = state.save("parameter");

		auto node = ptr <ast_parameter> ::from();
		auto ref = payload_ref <std::string> (node->name, token::misc_identifier);
		if (!match(state, node->type, ref))
			return nullptr;

		fmt::println("parameter: {}", node->name);

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

		std::vector <ast_parameter> params;
		while (auto param = ast_parameter::attempt(state)) {
			params.push_back(*param);
			fmt::println("!!! param group: got {}", param->name);
			if (!state.gettn_match(token::sign_comma))
				break;
		}

		if (!state.gettn_match(token::sign_parenthesis_right))
			return nullptr;

		// auto params_ptr = ptr <ast_parameter> ::block_from(params);
		// auto node = ptr <ast_pseudo_parameter_group> ::from(params.size(), params_ptr);

		auto node = ptr <ast_pseudo_parameter_group> ::from();
		node->nargs = params.size();
		node->parameters = ptr <ast_parameter> ::block_from(params);

		fmt::println("recap args:");
		auto p = node->parameters;
		for (size_t i = 0; i < node->nargs; i++, p++)
			p->dump();

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
	std::string name;
	// TODO: attribute list
	ptr <ast_attribute> attributes = nullptr;
	ptr <ast_type> return_type = nullptr;
	ptr <ast_parameter> parameters = nullptr;
	ptr <ast_block> block = nullptr;

	void dump(int indent = 0) {
		pp_println("function:");
		pp_println("  name: {}", name);
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

		auto node = ptr <ast_function> ::from();
		auto ref_kw = payload_ref <> (token::keyword_function);
		auto ref_name = payload_ref <std::string> (node->name, token::misc_identifier);
		ptr <ast_pseudo_parameter_group> p_args_group =  nullptr;
		ptr <ast_pseudo_return> p_return_type = nullptr;

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

		// TODO: move operation to null out
		node->nargs = p_args_group->nargs;
		node->parameters = p_args_group->parameters;

		node->return_type = p_return_type->type;

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

		std::vector <ast_global_decl> decls;

		while (!state.eof()) {
			auto decl = ast_global_decl::attempt(state);
			if (decl) {
				decls.push_back(*decl);
				continue;
			}

			return nullptr;
		}

		auto node = ptr <ast_module> ::from();
		node->count = decls.size();
		node->decls = ptr <ast_global_decl> ::block_from(decls);

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
}
