#ifndef AF2_GUI_DROPDOWN_HPP
#define AF2_GUI_DROPDOWN_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>       // SDL_...
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace gui {

class Dropdown : public Element {
public:
	using Function = std::function<void(Dropdown& dropdown)>;

	Dropdown(Vec2 position, Vec2 size, Color color, std::vector<std::string> options, std::size_t selectedOptionIndex, Function function);
	~Dropdown() override = default;

	Dropdown(const Dropdown&) = default;
	Dropdown(Dropdown&&) noexcept = default;

	auto operator=(const Dropdown&) -> Dropdown& = default;
	auto operator=(Dropdown&&) noexcept -> Dropdown& = default;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setColor(Color color) noexcept -> void;
	auto setSelectedOptionIndex(std::size_t selectedOptionIndex) noexcept -> void;
	auto setFunction(Function function) -> void;

	[[nodiscard]] auto getColor() const noexcept -> Color;
	[[nodiscard]] auto getOptionCount() const noexcept -> std::size_t;
	[[nodiscard]] auto getSelectedOptionIndex() const noexcept -> std::size_t;
	[[nodiscard]] auto getSelectedOption() const noexcept -> std::string_view;
	[[nodiscard]] auto getOption(std::size_t i) const -> std::string_view;

private:
	static constexpr auto NO_INDEX = std::numeric_limits<std::size_t>::max();
	static constexpr auto BASE_INDEX = std::size_t{NO_INDEX - 1};

	auto onActivate() -> void override;
	auto onDeactivate() -> void override;

	[[nodiscard]] auto getOptionScreenRect(std::size_t i, const CharWindow& charWindow) const -> Rect;

	Color m_color;
	std::vector<std::string> m_options;
	bool m_open = false;
	std::size_t m_selectedOptionIndex;
	std::size_t m_hoverOptionIndex;
	std::size_t m_pressedOptionIndex = 0;
	util::TileMatrix<char> m_closedMatrix{};
	util::TileMatrix<char> m_openMatrix{};
	Function m_function;
};

} // namespace gui

#endif
