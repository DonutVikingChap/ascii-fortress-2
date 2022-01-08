#include "text_input.hpp"

#include "../console/commands/gui_text_input_commands.hpp" // gui_input_...
#include "../game/client/char_window.hpp"                  // CharWindow
#include "../utilities/algorithm.hpp"                      // util::eraseIf

#include <algorithm> // std::max, std::min
#include <utility>   // std::move

namespace gui {

TextInput::TextInput(Vec2 position, Vec2 size, Color color, std::string text, Function onSubmit, Function onDeactivate, Function onType,
                     Function onAutoComplete, std::size_t maxLength, bool isPrivate, bool replaceMode)
	: Element(position, size)
	, m_color(color)
	, m_text(std::move(text))
	, m_onSubmit(std::move(onSubmit))
	, m_onDeactivate(std::move(onDeactivate))
	, m_onType(std::move(onType))
	, m_onAutoComplete(std::move(onAutoComplete))
	, m_maxLength(maxLength)
	, m_private(isPrivate)
	, m_replaceMode(replaceMode) {
	const auto w = static_cast<std::size_t>(m_size.x);
	const auto h = static_cast<std::size_t>(m_size.y);
	m_matrix.resize(w, h, gui_input_fill_char);
	m_matrix.drawLineVertical(0, 0, h, gui_input_side_char);
	m_matrix.drawLineVertical(w - 1, 0, h, gui_input_side_char);
	m_matrix.drawLineHorizontal(1, 0, w - 2, gui_input_top_bottom_char);
	m_matrix.drawLineHorizontal(1, h - 1, w - 2, gui_input_top_bottom_char);
	m_matrix.set(1, h / 2, '>');
}

auto TextInput::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	if (m_activated) {
		switch (e.type) {
			case SDL_MOUSEMOTION:
				if (m_dragging) {
					m_cursorBlinkTimer = 0.0f;
					m_cursorBlink = true;
					this->setCursorPosition(Vec2{e.motion.x, e.motion.y}, charWindow);
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
				if (e.button.button == SDL_BUTTON_LEFT) {
					this->handleLeftClick(e.button.x, e.button.y, charWindow);
				}
				break;
			case SDL_MOUSEBUTTONUP:
				if (e.button.button == SDL_BUTTON_LEFT) {
					m_dragging = false;
				}
				break;
			case SDL_TEXTINPUT:
				for (auto i = std::size_t{0}; i < sizeof(e.text.text) && e.text.text[i] != '\0'; ++i) {
					if (const auto ch = e.text.text[i]; ch >= 32 && ch <= 126) {
						this->eraseSelection();
						if (m_replaceMode && m_cursorPosition < m_text.size()) {
							this->eraseCharacter(m_cursorPosition);
						}
						if (m_text.size() < m_maxLength) {
							m_text.insert(m_text.begin() + static_cast<std::ptrdiff_t>(m_cursorPosition), ch);
							this->moveCursorRight();
							this->resetSelection();
						}
						if (m_onType) {
							m_onType(*this);
						}
					}
				}
				break;
			case SDL_KEYDOWN: {
				const auto shift = (e.key.keysym.mod & KMOD_SHIFT) != KMOD_NONE;
				const auto ctrl = (e.key.keysym.mod & KMOD_CTRL) != KMOD_NONE;
				m_cursorBlinkTimer = 0.0f;
				m_cursorBlink = true;
				switch (e.key.keysym.sym) {
					case SDLK_a:
						if (ctrl) {
							this->selectAll();
						}
						break;
					case SDLK_c:
						if (ctrl) {
							this->copy();
						}
						break;
					case SDLK_v:
						if (ctrl) {
							this->paste();
						}
						break;
					case SDLK_x:
						if (ctrl) {
							this->cut();
						}
						break;
					case SDLK_RETURN: this->submit(); break;
					case SDLK_BACKSPACE: this->handleBackspace(ctrl); break;
					case SDLK_DELETE: this->handleDelete(); break;
					case SDLK_INSERT: this->handleInsert(); break;
					case SDLK_LEFT: this->handleLeft(shift, ctrl); break;
					case SDLK_RIGHT: this->handleRight(shift, ctrl); break;
					case SDLK_UP: this->handleUp(); break;
					case SDLK_DOWN: this->handleDown(); break;
					case SDLK_HOME: this->handleHome(shift); break;
					case SDLK_END: this->handleEnd(shift); break;
					case SDLK_ESCAPE: this->deactivate(); break;
					case SDLK_TAB: this->autoComplete(); break;
					default: break;
				}
				break;
			}
			default: break;
		}
	} else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
		this->handleLeftClick(e.button.x, e.button.y, charWindow);
	}
}

auto TextInput::update(float deltaTime) -> void {
	if (m_activated) {
		m_doubleClickTimer -= deltaTime;
		if (m_doubleClickTimer < 0.0f) {
			m_doubleClickTimer = 0.0f;
		}

		m_cursorBlinkTimer += deltaTime;
		while (m_cursorBlinkTimer >= CURSOR_BLINK_INTERVAL * 0.5f) {
			m_cursorBlink = !m_cursorBlink;
			m_cursorBlinkTimer -= CURSOR_BLINK_INTERVAL * 0.5f;
		}
	}
}

auto TextInput::draw(CharWindow& charWindow) const -> void {
	charWindow.draw(m_position, m_matrix, m_color);
	if (m_size.x > 4 && m_size.y > 2) {
		const auto width = static_cast<std::size_t>(m_size.x - 4);
		assert(m_horizontalScroll <= m_text.size());
		assert(m_horizontalScroll <= m_cursorPosition);
		const auto text = (m_private) ? std::string(std::min(m_text.size() - m_horizontalScroll, width), '*') :
                                        m_text.substr(m_horizontalScroll, width);

		// Draw text.
		charWindow.draw(Vec2{m_position.x + 2, m_position.y + m_size.y / 2}, text, m_color);

		if (m_activated) {
			if (m_cursorBlink) {
				// Draw selection.
				const auto selectionLength = this->getSelectionLength();
				if (selectionLength > 0) {
					const auto selectionBegin = this->getSelectionBegin();
					const auto selectionEnd = selectionBegin + selectionLength;
					if (selectionEnd > m_horizontalScroll && selectionBegin < width + m_horizontalScroll) {
						const auto selectionBeginX = selectionBegin - std::min(m_horizontalScroll, selectionBegin);
						const auto selectionEndX = std::min(selectionEnd - m_horizontalScroll, width);
						const auto selectionW = selectionEndX - selectionBeginX;
						charWindow.fillRect(Rect{static_cast<Rect::Length>(m_position.x + 2 + static_cast<Rect::Length>(selectionBeginX)),
						                         static_cast<Rect::Length>(m_position.y + m_size.y / 2),
						                         static_cast<Rect::Length>(selectionW),
						                         1},
						                    Color::blue());
					}
				}

				// Draw cursor.
				charWindow.draw(Vec2{m_position.x + 2 + static_cast<Vec2::Length>(m_cursorPosition - m_horizontalScroll), m_position.y + m_size.y / 2},
				                (m_replaceMode) ? Color::dark_gray() : Color::white());
			}
		}
	}
}

auto TextInput::setSubmitFunction(Function onSubmit) -> void {
	m_onSubmit = std::move(onSubmit);
}

auto TextInput::setDeactivateFunction(Function onDeactivate) -> void {
	m_onDeactivate = std::move(onDeactivate);
}

auto TextInput::setTypeFunction(Function onType) -> void {
	m_onType = std::move(onType);
}

auto TextInput::setAutoCompleteFunction(Function onAutoComplete) -> void {
	m_onAutoComplete = std::move(onAutoComplete);
}

auto TextInput::setColor(Color color) noexcept -> void {
	m_color = color;
}

auto TextInput::setPrivate(bool isPrivate) noexcept -> void {
	m_private = isPrivate;
}

auto TextInput::setReplaceMode(bool replaceMode) noexcept -> void {
	m_replaceMode = replaceMode;
}

auto TextInput::setText(std::string text) -> void {
	m_text = std::move(text);
	this->setCursorPosition(m_text.size());
	this->resetSelection();
	m_currentInputIndex = 0;
}

auto TextInput::clearText() noexcept -> void {
	m_text.clear();
	this->setCursorPosition(0);
	this->resetSelection();
	m_currentInputIndex = 0;
}

auto TextInput::addToHistory(std::string text) -> void {
	m_inputHistory.push_back(std::move(text));
}

auto TextInput::submit() -> void {
	if (m_onSubmit) {
		m_onSubmit(*this);
	}
}

auto TextInput::isPrivate() const noexcept -> bool {
	return m_private;
}

auto TextInput::getText() const noexcept -> std::string_view {
	return m_text;
}

auto TextInput::getColor() const noexcept -> Color {
	return m_color;
}

auto TextInput::getReplaceMode() const noexcept -> bool {
	return m_replaceMode;
}

auto TextInput::onActivate() -> void {
	SDL_StartTextInput();
}

auto TextInput::onDeactivate() -> void {
	SDL_StopTextInput();
	m_doubleClickTimer = 0.0f;
	m_dragging = false;
	if (m_onDeactivate) {
		m_onDeactivate(*this);
	}
}

auto TextInput::updateHorizontalScroll() -> void {
	const auto width = static_cast<std::size_t>(m_size.x - 4);
	if (m_cursorPosition >= width && m_horizontalScroll < m_cursorPosition - width) {
		m_horizontalScroll = m_cursorPosition - width;
	}
	if (m_horizontalScroll > m_cursorPosition) {
		m_horizontalScroll = m_cursorPosition;
	}
}

auto TextInput::resetSelection() -> void {
	m_selectionStart = m_cursorPosition;
}

auto TextInput::eraseSelection() -> void {
	m_text.erase(this->getSelectionBegin(), this->getSelectionLength());
	m_cursorPosition = this->getSelectionBegin();
	this->updateHorizontalScroll();
	this->resetSelection();
}

auto TextInput::setCursorPosition(std::size_t pos) -> void {
	m_cursorPosition = (pos > m_text.size()) ? m_text.size() : pos;
	this->updateHorizontalScroll();
}

auto TextInput::setCursorPosition(Vec2 mousePos, const CharWindow& charWindow) -> void {
	const auto cursorOffset = static_cast<Vec2::Length>(charWindow.screenToGridCoordinates(mousePos).x - m_position.x - 2);
	this->setCursorPosition(static_cast<std::size_t>(
		std::max(Vec2::Length{0}, static_cast<Vec2::Length>(static_cast<Vec2::Length>(m_horizontalScroll) + cursorOffset))));
}

auto TextInput::moveCursorLeft() -> void {
	if (m_cursorPosition > 0) {
		--m_cursorPosition;
		this->updateHorizontalScroll();
	}
}

auto TextInput::moveCursorRight() -> void {
	if (m_cursorPosition < m_text.size()) {
		++m_cursorPosition;
		this->updateHorizontalScroll();
	}
}

auto TextInput::moveCursorToPreviousWord() -> void {
	if (m_cursorPosition > 0) {
		--m_cursorPosition;
		while (m_cursorPosition > 0 && (m_text[m_cursorPosition] == ' ' || m_text[m_cursorPosition] == '\t')) {
			--m_cursorPosition;
		}

		while (m_cursorPosition > 0 && m_text[m_cursorPosition - 1] != ' ' && m_text[m_cursorPosition - 1] != '\t') {
			--m_cursorPosition;
		}

		this->updateHorizontalScroll();
	}
}

auto TextInput::moveCursorToNextWord() -> void {
	while (m_cursorPosition < m_text.size() && m_text[m_cursorPosition] != ' ' && m_text[m_cursorPosition] != '\t') {
		++m_cursorPosition;
	}
	while (m_cursorPosition < m_text.size() && (m_text[m_cursorPosition] == ' ' || m_text[m_cursorPosition] == '\t')) {
		++m_cursorPosition;
	}

	this->updateHorizontalScroll();
}

auto TextInput::eraseCharacter(std::size_t pos) -> void {
	m_text.erase(m_text.begin() + static_cast<std::ptrdiff_t>(pos));
}

auto TextInput::handleUp() -> void {
	++m_currentInputIndex;
	if (m_currentInputIndex > m_inputHistory.size()) {
		m_currentInputIndex = m_inputHistory.size();
	} else {
		m_text = m_inputHistory[m_inputHistory.size() - m_currentInputIndex];
	}

	this->setCursorPosition(m_text.size());
	this->resetSelection();
}

auto TextInput::handleDown() -> void {
	if (m_currentInputIndex == 1) {
		m_text.clear();
	}

	if (m_currentInputIndex > 0) {
		--m_currentInputIndex;
	}

	if (m_currentInputIndex > 0) {
		m_text = m_inputHistory[m_inputHistory.size() - m_currentInputIndex];
	}

	this->setCursorPosition(m_text.size());
	this->resetSelection();
}

auto TextInput::handleLeft(bool shift, bool ctrl) -> void {
	if (ctrl) {
		this->moveCursorToPreviousWord();
		if (!shift) {
			this->resetSelection();
		}
	} else if (m_cursorPosition != m_selectionStart) {
		if (shift) {
			this->moveCursorLeft();
		} else {
			this->setCursorPosition(this->getSelectionBegin());
			this->resetSelection();
		}
	} else {
		this->moveCursorLeft();
		if (!shift) {
			this->resetSelection();
		}
	}
}

auto TextInput::handleRight(bool shift, bool ctrl) -> void {
	if (ctrl) {
		this->moveCursorToNextWord();
		if (!shift) {
			this->resetSelection();
		}
	} else if (m_cursorPosition != m_selectionStart) {
		if (shift) {
			this->moveCursorRight();
		} else {
			this->setCursorPosition(this->getSelectionBegin() + this->getSelectionLength());
			this->resetSelection();
		}
	} else {
		this->moveCursorRight();
		if (!shift) {
			this->resetSelection();
		}
	}
}

auto TextInput::handleHome(bool shift) -> void {
	this->setCursorPosition(0);
	if (!shift) {
		this->resetSelection();
	}
}

auto TextInput::handleEnd(bool shift) -> void {
	this->setCursorPosition(m_text.size());
	if (!shift) {
		this->resetSelection();
	}
}

auto TextInput::handleBackspace(bool ctrl) -> void {
	if (this->getSelectionLength() > 0) {
		this->eraseSelection();
	} else if (m_cursorPosition > 0) {
		if (ctrl) {
			this->moveCursorToPreviousWord();
			this->eraseSelection();
		} else {
			this->moveCursorLeft();
			this->eraseCharacter(m_cursorPosition);
			this->resetSelection();
		}
	}
	if (m_onType) {
		m_onType(*this);
	}
}

auto TextInput::handleDelete() -> void {
	if (this->getSelectionLength() > 0) {
		this->eraseSelection();
	} else if (m_cursorPosition < m_text.size()) {
		this->eraseCharacter(m_cursorPosition);
	}
	if (m_onType) {
		m_onType(*this);
	}
}

auto TextInput::handleInsert() -> void {
	m_replaceMode = !m_replaceMode;
}

auto TextInput::handleLeftClick(int x, int y, const CharWindow& charWindow) -> void {
	if (const auto mousePos = Vec2{x, y}; this->getScreenRect(charWindow).contains(mousePos)) {
		if (!m_activated) {
			this->activate();
		}

		if (m_doubleClickTimer > 0.0f) {
			this->selectAll();
		} else {
			this->setCursorPosition(mousePos, charWindow);
			this->resetSelection();
		}
		m_dragging = true;
		m_doubleClickTimer = DOUBLE_CLICK_TIME;
		m_cursorBlinkTimer = 0.0f;
		m_cursorBlink = true;
	} else if (m_activated) {
		this->deactivate();
	}
}

auto TextInput::autoComplete() -> void {
	if (m_onAutoComplete) {
		m_onAutoComplete(*this);
	}
}

auto TextInput::selectAll() -> void {
	m_selectionStart = 0;
	m_cursorPosition = m_text.size();
	this->updateHorizontalScroll();
}

auto TextInput::copy() -> void {
	SDL_SetClipboardText(this->getSelection().c_str());
}

auto TextInput::paste() -> void {
	this->eraseSelection();
	auto* const text = SDL_GetClipboardText();
	try {
		const auto maxLength = m_maxLength - m_text.size();
		auto length = std::size_t{0};
		while (length < maxLength && text[length] != '\0') {
			++length;
		}
		auto str = std::string{text, length};
		util::eraseIf(str, [](char ch) { return ch < 32 || ch > 126; }); // Remove newline characters, etc.
		m_text.insert(m_cursorPosition, str);
		this->setCursorPosition(m_cursorPosition + str.size());
		this->resetSelection();
		if (m_onType) {
			m_onType(*this);
		}
		SDL_free(text);
	} catch (...) {
		SDL_free(text);
		throw;
	}
}

auto TextInput::cut() -> void {
	this->copy();
	this->eraseSelection();
	if (m_onType) {
		m_onType(*this);
	}
}

auto TextInput::getSelectionBegin() const -> size_t {
	return std::min(m_selectionStart, m_cursorPosition);
}

auto TextInput::getSelectionLength() const -> size_t {
	return (m_cursorPosition < m_selectionStart) ? m_selectionStart - m_cursorPosition : m_cursorPosition - m_selectionStart;
}

auto TextInput::getSelection() const -> std::string {
	return m_text.substr(this->getSelectionBegin(), this->getSelectionLength());
}

} // namespace gui
