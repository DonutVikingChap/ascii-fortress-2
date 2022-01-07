#ifndef AF2_GUI_CHECKBOX_HPP
#define AF2_GUI_CHECKBOX_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>      // SDL_...
#include <cstdint>    // std::uint8_t
#include <functional> // std::function

namespace gui {

class Checkbox : public Element {
public:
	using Function = std::function<void(Checkbox& checkbox)>;
	enum class State : std::uint8_t {
		NORMAL,
		HOVER,
		PRESSED,
	};

	Checkbox(Vec2 position, Vec2 size, Color color, bool value, Function function);
	~Checkbox() override = default;

	Checkbox(const Checkbox&) = default;
	Checkbox(Checkbox&&) noexcept = default;

	auto operator=(const Checkbox&) -> Checkbox& = default;
	auto operator=(Checkbox&&) noexcept -> Checkbox& = default;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setFunction(Function function) -> void;
	auto setValue(bool value) -> void;
	auto setState(State state) noexcept -> void;
	auto setColor(Color color) noexcept -> void;

	[[nodiscard]] auto getValue() const noexcept -> bool;
	[[nodiscard]] auto getState() const noexcept -> State;
	[[nodiscard]] auto getColor() const noexcept -> Color;

private:
	auto onActivate() -> void override;
	auto onDeactivate() -> void override;

	Color m_color;
	State m_state = State::NORMAL;
	bool m_value;
	util::TileMatrix<char> m_enabledMatrix{};
	util::TileMatrix<char> m_disabledMatrix{};
	util::TileMatrix<char> m_hoverEnabledMatrix{};
	util::TileMatrix<char> m_hoverDisabledMatrix{};
	util::TileMatrix<char> m_pressedMatrix{};
	Function m_function;
};

} // namespace gui

#endif
