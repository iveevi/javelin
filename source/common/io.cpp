#include <vector>

#include <fmt/printf.h>

#include "common/io.hpp"

namespace jvl::io {

std::string repeat(const std::string &a, size_t b)
{
	std::string output;
	while (b--)
		output += a;

	return output;
}

void header(const std::string &name, size_t size)
{
	std::string s1 = "\u250C" + repeat("\u2500", size - 2) + "\u2510";
	std::string s2 = "\u2514" + repeat("\u2500", size - 2) + "\u2518";
	std::string s3 = std::string((size - 2 - name.size()) / 2, ' ');
	std::string s4 = std::string((size - 2 - name.size()) - s3.size(), ' ');
	fmt::println("{}", s1);
	fmt::println("{}{}{}{}{}", "\u2502", s3, name, s4, "\u2502");
	fmt::println("{}", s2);
}

void display_lines(const std::string &name, const std::string &contents)
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

	// header(name, mlen + 6);
	header(name, 50);
	for (size_t i = 0; i < lines.size(); i++)
		fmt::println("{:4d}: {}", i + 1, lines[i]);
}

} // namespace jvl::io