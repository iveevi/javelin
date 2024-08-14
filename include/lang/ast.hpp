#pragma once

#include "token.hpp"
#include "memory.hpp"
#include "parsing.hpp"
#include <cstdlib>

namespace jvl::lang {

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

	ast_string(const ast_string &) = delete;
	ast_string &operator=(const ast_string &) = delete;

	ast_string(ast_string &&other) {
		*this = std::move(other);
	}

	ast_string &operator=(ast_string &&other) {
		if (this != &other) {
			data.free();
			size = other.size;
			data = other.data;
			other.size = 0;
			other.data = nullptr;
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

		auto free_node = save.release_on_fail(node);
		if (!match(state, ref, node->nspace))
			return nullptr;

		return save.ok(node);
	}
};

struct ast_pseudo_attribute_list {
	size_t count;
	ptr <ast_attribute> attributes = nullptr;

	~ast_pseudo_attribute_list() {
		attributes.free();
		disown();
	}

	void disown() {
		attributes = nullptr;
	}

	static ptr <ast_pseudo_attribute_list> attempt(parse_state &state) {
		auto save = state.save("pseuo: attribute list");

		std::vector <ptr <ast_attribute>> attrs;
		while (auto attribute = ast_attribute::attempt(state))
			attrs.push_back(attribute);

		if (attrs.empty())
			return nullptr;

		auto node = ptr <ast_pseudo_attribute_list> ::from();
		node->count = attrs.size();
		node->attributes = ptr <ast_attribute> ::block_from(attrs);

		// TODO: disown_and_free_list for vector of ptrs?
		for (auto attr : attrs)
			attr.disowned().free();

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
	ptr <ast_factor> a = nullptr;
	ptr <ast_factor> b = nullptr;

	enum {
		eAdd,
		eSubtract,
		eSolo
	} operation;

	~ast_expression() {
		a.free();
		b.free();
		disown();
	}

	void disown() {
		a = nullptr;
		b = nullptr;
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

inline void ast_expression::dump(int indent)
{
	pp_println("expression:");

	std::string repr;
	switch (operation) {
	case eAdd:
		repr = "add";
		break;
	default:
		repr = "?";
		break;
	}

	pp_println("  operation: {}", repr);
	a->dump(indent + 1);
	if (b)
		b->dump(indent + 1);
}

inline ptr <ast_expression> ast_expression::attempt(parse_state &state)
{
	auto save = state.save("expression");

	auto node = ptr <ast_expression> ::from();

	// Addition of terms
	auto ref_plus = payload_ref <> (token::sign_plus);
	if (match(state, node->a, ref_plus, node->b)) {
		node->operation = eAdd;
		return save.ok(node);
	}

	// Single term
	if (match(state, node->a)) {
		node->operation = eSolo;
		return save.ok(node);
	}

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

	~ast_pseudo_statement_list() {
		stmts.free();
		disown();
	}

	void disown() {
		stmts = nullptr;
	}

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

		stmts.disowned().free();

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

	~ast_pseudo_return() {
		type.free();
		disown();
	}

	void disown() {
		type = nullptr;
	}

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

	~ast_pseudo_parameter_group() {
		parameters.free();
		disown();
	}

	void disown() {
		parameters = nullptr;
	}

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

		pp_println("  attributes:");
		auto ap = attributes;
		for (size_t i = 0; i < nattrs; i++, ap++)
			ap->dump(indent + 1);

		pp_println("  return type:");
		if (return_type)
			return_type->dump(indent + 2);

		pp_println("  parameters:");
		auto pp = parameters;
		for (size_t i = 0; i < nargs; i++, pp++)
			pp->dump(indent + 2);

		block->dump(indent + 1);
	}

	static ptr <ast_function> attempt(parse_state &state) {
		auto save = state.save("function");

		std::string name;

		auto node = ptr <ast_function> ::from();
		auto ref_kw = payload_ref <> (token::keyword_function);
		auto ref_name = payload_ref <std::string> (name, token::misc_identifier);

		ptr <ast_pseudo_attribute_list> p_attrs =  nullptr;
		ptr <ast_pseudo_parameter_group> p_args_group =  nullptr;
		ptr <ast_pseudo_return> p_return_type = nullptr;

		auto free_p_attrs = save.release_on_fail(p_attrs);
		auto free_p_args_group = save.release_on_fail(p_args_group);
		auto free_p_return_type = save.release_on_fail(p_return_type);

		bool success = false;

		// First try: everything
		if (!success) {
			// TODO: opt <...> types for match
			success =  match(state,
				/* begin */
				p_attrs,  // attribute
				ref_kw,            // keyword 'ftn'
				ref_name,          // function name
				p_args_group,      // parameters
				p_return_type,     // return type
				node->block        // function block statements
				/* end */ );
		} else {
			// TODO: free intermediates...
		}

		// Second try: skipping the attribute
		if (!success && !p_attrs) {
			success =  match(state,
				/* begin */
				ref_kw,            // keyword 'ftn'
				ref_name,          // function name
				p_args_group,      // parameters
				p_return_type,     // return type
				node->block        // function block statements
				/* end */ );
		}

		if (!success) {
			node.free();
			return nullptr;
		}

		node->nargs = p_args_group->nargs;
		node->parameters = p_args_group->parameters;

		node->nattrs = p_attrs->count;
		node->attributes = p_attrs->attributes;

		node->return_type = p_return_type->type;
		node->name = ptr <ast_string> ::from(name);

		p_attrs.disowned().free();
		p_args_group.disowned().free();
		p_return_type.disowned().free();

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
		auto free_node = save.release_on_fail(node);

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

} // namespace jvl::lang
