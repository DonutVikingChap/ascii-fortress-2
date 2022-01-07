#ifndef AF2_GUI_TEXT_INPUT_HPP
#define AF2_GUI_TEXT_INPUT_HPP

#include "../game/data/color.hpp"       // Color
#include "../utilities/tile_matrix.hpp" // util::TileMatrix
#include "element.hpp"

#include <SDL.h>       // SDL_..., KMOD_...
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <functional>  // std::function
#include <limits>      // std::numeric_limits
#include <string>      // std::string
#include <string_view> // std::string_view
#include <vector>      // std::vector

namespace gui {

class TextInput : public Element {
public:
	using Function = std::function<void(TextInput& textInput)>;

	TextInput(Vec2 position, Vec2 size, Color color, std::string text, Function onSubmit, Function onDeactivate, Function onType, Function onAutoComplete,
			  std::size_t maxLength = std::numeric_limits<std::size_t>::max(), bool isPrivate = false, bool replaceMode = false);

	~TextInput() override = default;

	TextInput(const TextInput&) = default;
	TextInput(TextInput&&) noexcept = default;

	auto operator=(const TextInput&) -> TextInput& = default;
	auto operator=(TextInput&&) noexcept -> TextInput& = default;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) const -> void override;

	auto setSubmitFunction(Function onSubmit) -> void;
	auto setDeactivateFunction(Function onDeactivate) -> void;
	auto setTypeFunction(Function onType) -> void;
	auto setAutoCompleteFunction(Function onAutoComplete) -> void;

	auto setColor(Color color) noexcept -> void;
	auto setPrivate(bool isPrivate) noexcept -> void;
	auto setReplaceMode(bool replaceMode) noexcept -> void;

	auto setText(std::string text) -> void;
	auto clearText() noexcept -> void;

	auto addToHistory(std::string text) -> void;

	auto submit() -> void;

	[[nodiscard]] auto isPrivate() const noexcept -> bool;
	[[nodiscard]] auto getText() const noexcept -> std::string_view;
	[[nodiscard]] auto getColor() const noexcept -> Color;
	[[nodiscard]] auto getReplaceMode() const noexcept -> bool;

private:
	auto onActivate() -> void override;
	auto onDeactivate() -> void override;

	auto updateHorizontalScroll() -> void;

	auto resetSelection() -> void;
	auto eraseSelection() -> void;

	auto setCursorPosition(std::size_t pos) -> void;
	auto setCursorPosition(Vec2 mousePos, const CharWindow& charWindow) -> void;

	auto moveCursorLeft() -> void;
	auto moveCursorRight() -> void;
	auto moveCursorToPreviousWord() -> void;
	auto moveCursorToNextWord() -> void;

	auto eraseCharacter(std::size_t pos) -> void;

	auto handleUp() -> void;
	auto handleDown() -> void;
	auto handleLeft(bool shift, bool ctrl) -> void;
	auto handleRight(bool shift, bool ctrl) -> void;
	auto handleHome(bool shift) -> void;
	auto handleEnd(bool shift) -> void;
	auto handleBackspace(bool ctrl) -> void;
	auto handleDelete() -> void;
	auto handleInsert() -> void;
	auto handleLeftClick(int x, int y, const CharWindow& charWindow) -> void;

	auto autoComplete() -> void;

	auto selectAll() -> void;

	auto copy() -> void;
	auto paste() -> void;
	auto cut() -> void;

	[[nodiscard]] auto getSelectionBegin() const -> std::size_t;
	[[nodiscard]] auto getSelectionLength() const -> std::size_t;

	[[nodiscard]] auto getSelection() const -> std::string;

	static constexpr auto CURSOR_BLINK_INTERVAL = 1.0f;
	static constexpr auto DOUBLE_CLICK_TIME = 0.5f;

	Color m_color;
	util::TileMatrix<char> m_matrix{};
	std::string m_text;
	Function m_onSubmit;
	Function m_onDeactivate;
	Function m_onType;
	Function m_onAutoComplete;
	std::size_t m_maxLength;
	bool m_private;
	bool m_replaceMode;
	float m_cursorBlinkTimer = 0.0f;
	float m_doubleClickTimer = 0.0f;
	std::vector<std::string> m_inputHistory{};
	std::size_t m_currentInputIndex = 0;
	std::size_t m_cursorPosition = 0;
	std::size_t m_selectionStart = 0;
	std::size_t m_horizontalScroll = 0;
	bool m_cursorBlink = true;
	bool m_dragging = false;
};

} // namespace gui

#endif
