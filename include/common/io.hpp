#pragma once

#include <filesystem>
#include <string>

namespace jvl::io {

////////////////////////
// Printing utilities //
////////////////////////

// Boxed headers
void header(const std::string &title, size_t size, bool source = false);

// Displaying source code
void display_lines(const std::string &title, const std::string &content, bool source = false);

void write_lines(const std::filesystem::path &path, const std::string &content);

} // namespace jvl::io