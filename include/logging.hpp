#include <fmt/color.h>

namespace jvl::log {

inline void abort(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	exit(-1);
}

inline void abort(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "fatal error: ");
	fmt::println("{}", msg);
	exit(-1);
}

inline void assertion(bool cond, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	__builtin_trap();
}

inline void assertion(bool cond, const std::string &module, const std::string &msg)
{
	if (cond) return;
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::purple), "assertion failed: ");
	fmt::println("{}", msg);
	__builtin_trap();
}

inline void error(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
}

inline void error(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "error: ");
	fmt::println("{}", msg);
}

inline void warning(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "warning: ");
	fmt::println("{}", msg);
}

inline void warning(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::orange_red), "warning: ");
	fmt::println("{}", msg);
}

inline void info(const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin: ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "info: ");
	fmt::println("{}", msg);
}

inline void info(const std::string &module, const std::string &msg)
{
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gray), "javelin ");
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::gold), "({}): ", module);
	fmt::print(fmt::emphasis::bold | fmt::fg(fmt::color::cyan), "info: ");
	fmt::println("{}", msg);
}

// Helper macros for easier logging
#define MODULE(name) static constexpr const char __module__[] = #name

#define JVL_ASSERT(cond, ...)	log::assertion(cond, __module__, fmt::format(__VA_ARGS__))
#define JVL_ABORT(...)		log::abort(__module__, fmt::format(__VA_ARGS__))

} // namespace log