#pragma once

#include <chrono>

#include <fmt/color.h>

namespace jvl::io {

void assertion(bool cond, const std::string &);
void assertion(bool cond, const std::string &, const std::string &);
void assertion(bool cond, const std::string &, const std::string &, const char *const, int);

[[noreturn]] void abort(const std::string &);
[[noreturn]] void abort(const std::string &, const std::string &);
[[noreturn]] void abort(const std::string &, const std::string &, const char *const, int);

void error(const std::string &);
void error(const std::string &, const std::string &);

void warning(const std::string &);
void warning(const std::string &, const std::string &);

void info(const std::string &);
void info(const std::string &, const std::string &);

void note(const std::string &);

struct stage_bracket {
	std::string module;

	using clock_t = std::chrono::high_resolution_clock;
	using time_t = clock_t::time_point;

	clock_t clk;
	time_t start;
	time_t end;

	stage_bracket(const std::string &);

	~stage_bracket();
};

// Helper macros for easier logging
#define MODULE(name) static constexpr const char __module__[] = #name

} // namespace jvl::io

#ifdef JVL_DEBUG

#define JVL_ASSERT(cond, ...)	jvl::io::assertion(cond, __module__, fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define JVL_ASSERT_PLAIN(cond)	jvl::io::assertion(cond, __module__, fmt::format("{}:{}\n\t{}", __FILE__, __LINE__, #cond))

#define JVL_ABORT(...)		jvl::io::abort(__module__, fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define JVL_ERROR(...)		jvl::io::error(__module__, fmt::format(__VA_ARGS__))
#define JVL_WARNING(...)	jvl::io::warning(__module__, fmt::format(__VA_ARGS__))
#define JVL_INFO(...)		jvl::io::info(__module__, fmt::format(__VA_ARGS__))
#define JVL_DEBUG_INFO(...)	jvl::io::info(__module__, fmt::format(__VA_ARGS__))
#define JVL_NOTE(...)		jvl::io::note(fmt::format(__VA_ARGS__))

#define JVL_STAGE()		jvl::io::stage_bracket __stage(__module__)
#define JVL_STAGE_SECTION(s)	jvl::io::stage_bracket __stage(#s)

#else

#define JVL_ASSERT(cond, ...)	if (cond && __module__) {}
#define JVL_ASSERT_PLAIN(cond)	if (cond && __module__) {}

#define JVL_ABORT(...)		jvl::io::abort(__module__, fmt::format(__VA_ARGS__), __FILE__, __LINE__)
#define JVL_ERROR(...)		jvl::io::error(__module__, fmt::format(__VA_ARGS__))
#define JVL_WARNING(...)	jvl::io::warning(__module__, fmt::format(__VA_ARGS__))
#define JVL_INFO(...)		jvl::io::info(__module__, fmt::format(__VA_ARGS__))
#define JVL_DEBUG_INFO(...)
#define JVL_NOTE(...)		jvl::io::note(fmt::format(__VA_ARGS__))

#define JVL_STAGE()
#define JVL_STAGE_SECTION(s)

#endif
