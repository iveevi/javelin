#include <array>

#include "lang/token.hpp"

namespace jvl::lang {

[[gnu::always_inline]]
inline bool is_legal_first_identifier_char(char c)
{
	return std::isalpha(c) || c == '_';
}

[[gnu::always_inline]]
inline bool is_legal_identifier_char(char c)
{
	return std::isalpha(c) || std::isdigit(c) || c == '_';
}

struct lex_state {
	const std::string &source;
	int index;
	int line;
	int column;

	std::vector <token> produced;

	bool eof() const {
		return index >= source.size();
	}

	char getc() const {
		return source[index];
	}

	char getcn() {
		return source[index++];
	}

	bool keyword_or_identifier() {
		static const std::array <decltype(token::type), 9> match_values {
			token::keyword_auto,
			token::keyword_for,
			token::keyword_function,
			token::keyword_import,
			token::keyword_return,
			token::keyword_struct,
			token::keyword_using,
			token::keyword_while,
		};

		static const std::array <std::string, 9> match_strings {
			"auto",
			"for",
			"ftn",
			"import",
			"return",
			"struct",
			"using",
			"while",
		};

		std::string buffer;
		while (!eof()) {
			if (!is_legal_identifier_char(getc()))
				break;

			buffer += getcn();
		}

		for (size_t i = 0; i < match_strings.size(); i++) {
			if (match_strings[i] == buffer)
				return push(match_values[i]);
		}

		return push(token::misc_identifier, buffer);
	}

	bool literal_numerical() {
		int integer = 0;

		char c;
		while ((c = getc()) && std::isdigit(c)) {
			integer = 10 * integer + (c - '0');
			getcn();
		}

		if (getc() == '.') {
			float real = integer;
			float power = -1;
			getcn();
			while ((c = getc()) && std::isdigit(c)) {
				real += std::pow(10, power--) * (c - '0');
				getcn();
			}

			return push(token::literal_float, real);
		}

		return push(token::literal_int, integer);
	}

	bool push(decltype(token::type) type) {
		token t;
		t.type = type;
		produced.push_back(t);
		return true;
	}

	template <typename T>
	bool push(decltype(token::type) type, const T &v) {
		token t;
		t.type = type;
		t.payload = v;
		produced.push_back(t);
		return true;
	}

	bool consume_and_push(decltype(token::type) type) {
		getcn();
		return push(type);
	}

	bool sign(char c) {
		token t;
		switch(c) {
		case ';': return consume_and_push(token::sign_semicolon);
		case '@': return consume_and_push(token::sign_attribute);
		case '.': return consume_and_push(token::sign_dot);
		case ',': return consume_and_push(token::sign_comma);
		case '=': return consume_and_push(token::sign_equals);

		case '(': return consume_and_push(token::sign_parenthesis_left);
		case ')': return consume_and_push(token::sign_parenthesis_right);

		case '[': return consume_and_push(token::sign_bracket_left);
		case ']': return consume_and_push(token::sign_bracket_right);

		case '{': return consume_and_push(token::sign_brace_left);
		case '}': return consume_and_push(token::sign_brace_right);

		case '-':
			  getcn();
			  if (getc() == '>')
				  return consume_and_push(token::sign_arrow);
			  return push(token::sign_minus);
		default:
			break;
		}

		return false;
	}

	void lex_assert(bool cond, const std::string &message) const {
		if (cond)
			return;

		fmt::println("error: {}", message);
		abort();
	}

	void entry() {
		while (!eof()) {
			char c = getc();
			if (std::isdigit(c)) {
				lex_assert(literal_numerical(),
					"expected numerical literal");
			} else if (is_legal_first_identifier_char(c)) {
				lex_assert(keyword_or_identifier(),
					"expected keyword or identifier");
			} else if (std::isspace(c)) {
				getcn();
				// TODO: line/column update
			} else if (sign(c)) {
				// Nothing to do after...
			} else {
				fmt::println("unexpected character: '{}'", c);
				abort();
			}
		}
	}
};

std::vector <token> lexer(const std::string &source)
{
	lex_state ls(source, 0);
	ls.entry();
	return ls.produced;
}

} // namespace jvl::lang
