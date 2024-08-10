#pragma once

#include <string>
#include <vector>

#include "../wrapped_types.hpp"

// TODO: move to lang/format.hpp
#include <fmt/format.h>

namespace jvl::lang {

using token_payload_t = wrapped::variant <
	int, float, std::string
>;

struct token {
	enum kind {
		literal_float,
		literal_int,
		literal_string,

		sign_arrow,
		sign_attribute,
		sign_comma,
		sign_dot,
		sign_equals,
		sign_minus,
		sign_plus,
		sign_semicolon,
		sign_star,

		sign_parenthesis_left,
		sign_parenthesis_right,
		sign_bracket_left,
		sign_bracket_right,
		sign_brace_left,
		sign_brace_right,

		keyword_auto,
		keyword_for,
		keyword_function,
		keyword_import,
		keyword_return,
		keyword_struct,
		keyword_using,
		keyword_while,

		misc_identifier,
	} type;

	int line;
	int column;

	token_payload_t payload;

	bool is(decltype(type) t) const {
		return type == t;
	}

	static constexpr const char *name[] = {
		"literal_float",
		"literal_int",
		"literal_string",

		"sign_arrow",
		"sign_attribute",
		"sign_comma",
		"sign_dot",
		"sign_equals",
		"sign_minus",
		"sign_plus",
		"sign_semicolon",
		"sign_star",

		"sign_parenthesis_left",
		"sign_parenthesis_right",
		"sign_bracket_left",
		"sign_bracket_right",
		"sign_brace_left",
		"sign_brace_right",

		"keyword_auto",
		"keyword_for",
		"keyword_function",
		"keyword_import",
		"keyword_return",
		"keyword_struct",
		"keyword_using",
		"keyword_while",

		"misc_identifier",
	};
};

std::vector <token> lexer(const std::string &);

inline auto format_as(const token &t)
{
	std::string payload = "(nil)";
	if (t.type == token::literal_float)
		payload = fmt::format("{}", t.payload.as <float> ());
	if (t.type == token::literal_int)
		payload = fmt::format("{}", t.payload.as <int> ());
	if (t.type == token::literal_string)
		payload = "\"" + t.payload.as <std::string> () + "\"";
	if (t.type == token::misc_identifier)
		payload = t.payload.as <std::string> ();

	return fmt::format("({}: {})", token::name[t.type], payload);
}

} // namespace jvl::lang
