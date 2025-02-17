#include <vector>

#include <fmt/printf.h>

#include "util.hpp"

void dump_lines(const std::string &contents)
{
	std::vector <std::string> lines;

	std::string s = contents + "\n";

	std::string interim;
	for (auto c : s) {
		if (c == '\n') {
			lines.emplace_back(interim);
			interim.clear();
			continue;
		}

		interim += c;
	}

	for (size_t i = 0; i < lines.size(); i++)
		fmt::println("{:4d}: {}", i + 1, lines[i]);
}