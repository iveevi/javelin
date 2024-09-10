#pragma once

#include <gtest/gtest.h>

#include <string>

// Source comparison utilities
std::string trim_shader_source(const std::string &A)
{
	static constexpr const char ws[] = " \t\n\r\f\v";
	std::string s = A;
	s.erase(s.find_last_not_of(ws) + 1);
	s.erase(0, s.find_first_not_of(ws));
	return s;
}

void check_shader_sources(const std::string &A, const std::string &B)
{
	auto tA = trim_shader_source(A);
	auto tB = trim_shader_source(B);
	ASSERT_EQ(tA, tB);
}