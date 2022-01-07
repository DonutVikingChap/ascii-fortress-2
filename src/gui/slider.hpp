#ifndef AF2_GUI_SLIDER_HPP
#define AF2_GUI_SLIDER_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>      // SDL_...
#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint8_t
#include <functional> // std::function

namespace gui {

class Slider : public Element {
public:
	using Function = std::function<void(Slider& slider)>;
	enum class State : std::uint8_t {
		NORMAL,
		HOVER,
		PRESSED,
	};

	Slider(Vec2 position, Vec2 size, Color color, float value, float delta, Function function);
	~Slider() override = default;

	Slider(const Slider&) = default;
	Slider(Slider&&) noexcept = default;

	auto operator=(const Slider&) -> Slider& = default;
	auto operator=(Slider&&) noexcept -> Slider& = default;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setFunction(Function function) -> void;
	auto setState(State state) noexcept -> void;
	auto setColor(Color color) noexcept -> void;
	auto setValue(float value) noexcept -> void;
	auto setHoverValue(float value) noexcept -> void;

	auto setValueToHoverValue() noexcept -> void;
	auto setHoverValueToValue() noexcept -> void;

	[[nodiscard]] auto getState() const noexcept -> State;
	[[nodiscard]] auto getColor() const noexcept -> Color;
	[[nodiscard]] auto getValue() const noexcept -> float;

private:
	[[nodiscard]] auto getActiveRect(const CharWindow& charWindow) const -> Rect;

	auto slideHoverValueLeft() -> void;
	auto slideHoverValueRight() -> void;

	auto updateHoverValue(const CharWindow& charWindow, Vec2 mousePosition) -> void;

	auto onActivate() -> void override;
	auto onDeactivate() -> void override;

	Color m_color;
	State m_state = State::NORMAL;
	char m_slideHoverChar;
	char m_slideChar;
	float m_delta;
	Function m_function;
	float m_value = 0.0f;
	float m_hoverValue = 0.0f;
	Vec2::Length m_hoverOffset = 0;
	Vec2::Length m_valueOffset = 0;
	util::TileMatrix<char> m_normalMatrix{};
	util::TileMatrix<char> m_hoverMatrix{};
};

} // namespace gui

#endif
