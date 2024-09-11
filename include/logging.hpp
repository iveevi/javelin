#pragma once

#include <chrono>
#include <ratio>

#include <fmt/color.h>

namespace jvl::log {

inline void assertion(bool cond, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

inline void assertion(bool cond, const std::string &module, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

[[noreturn]]
inline void abort(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	exit(-1);
}

[[noreturn]]
inline void abort(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	exit(-1);
}

inline void error(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

inline void error(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

inline void warning(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), "warning: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

inline void warning(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), "warning: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

inline void info(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "info: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

inline void info(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "info: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

struct stage_bracket {
	std::string module;

	using clock_t = std::chrono::high_resolution_clock;
	using time_t = clock_t::time_point;

	clock_t clk;
	time_t start;
	time_t end;

	stage_bracket(const std::string &module_) : module(module_) {
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "stage: ");
		fmt::print(fmt::emphasis::underline | fmt::emphasis::bold | fmt::fg(fmt::color::white_smoke), "{}\n", module);
		std::fflush(stdout);

		start = clk.now();
	}

	~stage_bracket() {
		end = clk.now();

		auto us = std::chrono::duration_cast <std::chrono::microseconds> (end - start).count();
		auto ms = us/1000.0;

		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "close: ");
		fmt::print(fmt::emphasis::underline | fmt::emphasis::bold | fmt::fg(fmt::color::white_smoke), "{}", module);
		fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::white_smoke), " ({} ms)\n", ms);
		std::fflush(stdout);
	}
};

// Helper macros for easier logging
#define MODULE(name) static constexpr const char __module__[] = #name

#define JVL_ASSERT(cond, ...)	log::assertion(cond, __module__, fmt::format(__VA_ARGS__))
#define JVL_ASSERT_PLAIN(cond)	log::assertion(cond, __module__, fmt::format("{}:{}\t{}", __FILE__, __LINE__, #cond))

#define JVL_ABORT(...)		log::abort(__module__, fmt::format(__VA_ARGS__))
#define JVL_ERROR(...)		log::error(__module__, fmt::format(__VA_ARGS__))
#define JVL_WARNING(...)	log::warning(__module__, fmt::format(__VA_ARGS__))
#define JVL_INFO(...)		log::info(__module__, fmt::format(__VA_ARGS__))
#define JVL_STAGE()		log::stage_bracket __stage(__module__)
#define JVL_STAGE_SECTION(s)	log::stage_bracket __stage(s)

// TODO: info_verbose (logging in cmd line)

} // namespace log
