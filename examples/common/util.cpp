#include <vector>

#include <fmt/printf.h>

#include "util.hpp"

std::string repeat(const std::string &a, size_t b)
{
	std::string output;
	while (b--)
		output += a;

	return output;
}

void dump_lines(const std::string &name, const std::string &contents)
{
	std::vector <std::string> lines;

	std::string s = contents + "\n";

	std::string interim;

	size_t mlen = 0;
	for (auto c : s) {
		if (c == '\n') {
			lines.emplace_back(interim);
			mlen = std::max(mlen, interim.size());
			interim.clear();
			continue;
		}

		interim += c;
	}

	std::string s1 = "\u250C" + repeat("\u2500", mlen + 4) + "\u2510";
	std::string s2 = "\u2514" + repeat("\u2500", mlen + 4) + "\u2518";
	std::string s3 = std::string((mlen + 4 - name.size()) / 2, ' ');
	std::string s4 = std::string((mlen + 4 - name.size()) - s3.size(), ' ');
	fmt::println("{}", s1);
	fmt::println("{}{}{}{}{}", "\u2502", s3, name, s4, "\u2502");
	fmt::println("{}", s2);
	for (size_t i = 0; i < lines.size(); i++)
		fmt::println("{:4d}: {}", i + 1, lines[i]);
}