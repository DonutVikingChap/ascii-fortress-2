#include "color.hpp"

#include "../../utilities/string.hpp" // util::iequals

#include <sstream> // std::istringstream, std::ostringstream
#include <string>  // std::string

auto Color::parse(std::string_view str) noexcept -> std::optional<Color> {
	for (const auto& color : Color::getAll()) {
		if (util::iequals(color.getName(), str) || util::iequals(color.getCodeName(), str)) {
			return color;
		}
	}

	try {
		auto stream = std::istringstream{std::string{str}};
		auto red = 0;
		auto green = 0;
		auto blue = 0;
		auto alpha = 0;
		if (!(stream >> red >> green >> blue)) {
			return std::nullopt;
		}
		if (!(stream >> alpha)) {
			alpha = 255;
		}
		return Color{
			static_cast<std::uint8_t>(red),
			static_cast<std::uint8_t>(green),
			static_cast<std::uint8_t>(blue),
			static_cast<std::uint8_t>(alpha),
		};
	} catch (...) {
	}
	return std::nullopt;
}

auto Color::getString() const -> std::string {
	if (const auto name = this->getName(); !name.empty()) {
		return std::string{name};
	}
	auto stream = std::ostringstream{};
	stream << static_cast<int>(r) << ' ' << static_cast<int>(g) << ' ' << static_cast<int>(b);
	if (a != 255) {
		stream << ' ' << static_cast<int>(a);
	}
	return stream.str();
}
