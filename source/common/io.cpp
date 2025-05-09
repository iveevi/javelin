#include <vector>

#include <fmt/printf.h>

#include "common/io.hpp"
#include "common/logging.hpp"

namespace jvl::io {

MODULE(io);

std::string repeat(const std::string &a, size_t b)
{
	std::string output;
	while (b--)
		output += a;

	return output;
}

void header(const std::string &title, size_t size, bool source)
{
	std::string s1 = "\u250C" + repeat("\u2500", size - 2) + "\u2510";
	std::string s2 = "\u2514" + repeat("\u2500", size - 2) + "\u2518";
	std::string s3 = std::string((size - 2 - title.size()) / 2, ' ');
	std::string s4 = std::string((size - 2 - title.size()) - s3.size(), ' ');

	if (source) {
		fmt::println("// {}", s1);
		fmt::println("// {}{}{}{}{}", "\u2502", s3, title, s4, "\u2502");
		fmt::println("// {}", s2);
	} else {
		fmt::println("{}", s1);
		fmt::println("{}{}{}{}{}", "\u2502", s3, title, s4, "\u2502");
		fmt::println("{}", s2);
	}
}

void display_lines(const std::string &title, const std::string &contents, bool source)
{
	std::vector <std::string> lines;
	lines.emplace_back("");

	std::string s = contents;

	size_t mlen = 0;
	for (auto c : s) {
		auto &str = lines.back();
		
		if (c == '\n') {
			mlen = std::max(mlen, str.size());
			lines.emplace_back("");
		} else {
			str += c;
		}
	}

	if (lines.back().empty())
		lines.pop_back();

	header(title, 50, source);

	if (source) {
		for (size_t i = 0; i < lines.size(); i++)
			fmt::println("{}", lines[i]);
	} else {
		for (size_t i = 0; i < lines.size(); i++)
			fmt::println("{:4d}: {}", i + 1, lines[i]);
	}
}

void write_lines(const std::filesystem::path &path, const std::string &contents)
{
	std::vector <std::string> lines;
	lines.emplace_back("");

	std::string s = contents;

	size_t mlen = 0;
	for (auto c : s) {
		auto &str = lines.back();
		
		if (c == '\n') {
			mlen = std::max(mlen, str.size());
			lines.emplace_back("");
		} else {
			str += c;
		}
	}

	if (lines.back().empty())
		lines.pop_back();

	FILE *fout = fopen(path.c_str(), "w");
	if (!fout)
		JVL_ABORT("failed to open file '{}'", path.string());
	
	for (size_t i = 0; i < lines.size(); i++)
		fmt::println(fout, "{}", lines[i]);

	fclose(fout);
}

} // namespace jvl::io