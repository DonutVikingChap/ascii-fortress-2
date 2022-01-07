#ifndef AF2_GUI_BUTTON_HPP
#define AF2_GUI_BUTTON_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>       // SDL_...
#include <cstdint>     // std::uint8_t
#include <functional>  // std::function
#include <string>      // std::string
#include <string_view> // std::string_view

namespace gui {

class Button : public Element {
public:
	using Function = std::function<void(Button& button)>;
	enum class State : std::uint8_t {
		NORMAL,
		HOVER,
		PRESSED,
	};

	Button(Vec2 position, Vec2 size, Color color, std::string text, Function function);
	~Button() override = default;

	Button(const Button&) = default;
	Button(Button&&) noexcept = default;

	auto operator=(const Button&) -> Button& = default;
	auto operator=(Button&&) noexcept -> Button& = default;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setFunction(Function function) -> void;
	auto setText(std::string text) -> void;
	auto setState(State state) noexcept -> void;
	auto setColor(Color color) noexcept -> void;

	[[nodiscard]] auto getText() const noexcept -> std::string_view;
	[[nodiscard]] auto getState() const noexcept -> State;
	[[nodiscard]] auto getColor() const noexcept -> Color;

private:
	auto onActivate() -> void override;
	auto onDeactivate() -> void override;

	Color m_color;
	State m_state = State::NORMAL;
	std::string m_text{};
	util::TileMatrix<char> m_normalMatrix{};
	util::TileMatrix<char> m_hoverMatrix{};
	util::TileMatrix<char> m_pressedMatrix{};
	Function m_function;
};

} // namespace gui

#endif
