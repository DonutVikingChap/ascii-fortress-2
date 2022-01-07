#ifndef AF2_GRAPHICS_ERROR_HPP
#define AF2_GRAPHICS_ERROR_HPP

#include <stdexcept> // std::runtime_error
#include <string>    // std::string

namespace gfx {

struct Error : std::runtime_error {
	explicit Error(const char* message)
		: std::runtime_error(message) {}

	explicit Error(const std::string& message)
		: std::runtime_error(message) {}
};

} // namespace gfx

#endif
