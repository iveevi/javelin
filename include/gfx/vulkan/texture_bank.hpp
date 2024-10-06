#pragma once

#include <string>
#include <map>

#include <littlevk/littlevk.hpp>

namespace jvl::gfx::vulkan {

struct TextureBank : public std::map <std::string, littlevk::Image> {
	using std::map <std::string, littlevk::Image> ::map;
};

} // namespace jvl::gfx::vulkan