#pragma once

#include "token.hpp"
#include "memory.hpp"

namespace jvl::lang {

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

} // namespace jvl::lang