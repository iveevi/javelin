#include "common/logging.hpp"

namespace jvl::io {

void assertion(bool cond, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

void assertion(bool cond, const std::string &module, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

void assertion(bool cond, const std::string &module, const std::string &msg, const char *const file, int line)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cadet_blue), "note: ");
	fmt::println("declared from {}:{}", file, line);
	std::fflush(stdout);
	__builtin_trap();
}

[[noreturn]]
void abort(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

[[noreturn]]
void abort(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
	__builtin_trap();
}

[[noreturn]]
void abort(const std::string &module, const std::string &msg, const char *const file, int line)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cadet_blue), "note: ");
	fmt::println("declared from {}:{}", file, line);
	std::fflush(stdout);
	__builtin_trap();
}

void error(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void error(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void warning(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), "warning: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void warning(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::magenta), "warning: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void info(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cadet_blue), "info: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void info(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cadet_blue), "info: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

void note(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cadet_blue), "note: ");
	fmt::println("{}", msg);
	std::fflush(stdout);
}

stage_bracket::stage_bracket(const std::string &module_) : module(module_)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "begin: ");
	fmt::print(fmt::emphasis::underline | fmt::emphasis::bold | fmt::fg(fmt::color::gray), "{}\n", module);
	std::fflush(stdout);

	start = clk.now();
}

stage_bracket::~stage_bracket()
{
	end = clk.now();

	auto us = std::chrono::duration_cast <std::chrono::microseconds> (end - start).count();
	auto ms = us/1000.0;

	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "close: ");
	fmt::print(fmt::emphasis::underline | fmt::emphasis::bold | fmt::fg(fmt::color::gray), "{}", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), " ({} ms)\n", ms);
	std::fflush(stdout);
}

} // namespace jvl::log