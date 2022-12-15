#include "console.hpp"

#include "../game/client/char_window.hpp" // CharWindow

#include <cmath>    // std::signbit
#include <iterator> // std::make_move_iterator
#include <utility>  // std::move

namespace gui {

Console::Console(Vec2 position, Vec2 size, Color color, std::size_t maxRows)
	: Element(position, size)
	, m_color(color)
	, m_text(1)
	, // Start with one empty row.
	m_maxRows(maxRows) {
	const auto w = static_cast<std::size_t>(m_size.x);
	const auto h = static_cast<std::size_t>(m_size.y);
	m_matrix.resize(w, h, ' ');
	m_matrix.drawLineHorizontal(0, 0, w, '#');
	m_matrix.drawLineVertical(0, 1, h - 1, '#');
	m_matrix.drawLineVertical(w - 1, 1, h - 1, '#');
}

auto Console::handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void {
	if (e.type == SDL_MOUSEMOTION) {
		m_mouseOver = this->getScreenRect(charWindow).contains(Vec2{e.motion.x, e.motion.y});
	} else if (m_mouseOver) {
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_PAGEUP) {
				this->scrollUp();
			} else if (e.key.keysym.sym == SDLK_PAGEUP) {
				this->scrollDown();
			}
		} else if (e.type == SDL_MOUSEWHEEL) {
			auto scrolls = static_cast<std::ptrdiff_t>(e.wheel.preciseY);
			if (scrolls == 0) {
				scrolls = (std::signbit(e.wheel.preciseY)) ? std::ptrdiff_t{-1} : std::ptrdiff_t{1};
			}
			this->scroll(scrolls);
		}
	}
}

auto Console::update(float) -> void {}

auto Console::draw(CharWindow& charWindow) const -> void {
	charWindow.draw(m_position, m_matrix, m_color);
	if (m_size.x > 2 && m_size.y > 2) {
		const auto startX = static_cast<decltype(Vec2::x)>(m_position.x + 1);
		const auto newlineCompensation = (m_text.back().empty() || m_text.back().back().ch == '\n') ? 1 : 0;
		const auto start = static_cast<std::ptrdiff_t>(m_text.size()) - 1 - newlineCompensation - m_scrollRow;

		auto position = Vec2{startX, static_cast<decltype(Vec2::y)>(m_position.y + m_size.y - 2)};
		for (auto i = start; i >= 0 && m_size.y - 3 + i - start >= 0; --i) {
			for (const auto& coloredChar : m_text[static_cast<std::size_t>(i)]) {
				charWindow.draw(position, coloredChar.ch, coloredChar.color);
				++position.x;
			}
			position.x = startX;
			--position.y;
		}
	}
}

auto Console::setMaxRows(std::size_t maxRows) -> void {
	m_maxRows = maxRows;
	this->eraseExcessRows();
}

auto Console::getMaxRows() const -> std::size_t {
	return m_maxRows;
}

auto Console::print(std::string_view text, Color color) -> void {
	const auto width = static_cast<std::size_t>(std::max(decltype(m_size.x){0}, static_cast<decltype(m_size.x)>(m_size.x - 2)));

	auto newRows = 0;
	auto wordLength = std::size_t{0};
	for (const auto ch : text) {
		if (ch == ' ') {
			wordLength = 0;
		} else if (ch == '\n') {
			++newRows;
			wordLength = 0;
			m_text.emplace_back();
			continue;
		} else {
			++wordLength;
		}

		m_text.back().push_back(ColoredChar{ch, color});
		if (m_text.back().size() > width) {
			++newRows;
			auto newRow = TextRow{};
			if (m_text.back().size() == wordLength) {
				newRow.push_back(m_text.back().back());
				m_text.back().pop_back();
				wordLength = 1;
			} else {
				const auto begin = m_text.back().end() - static_cast<std::ptrdiff_t>(wordLength);
				const auto end = m_text.back().end();
				newRow.insert(newRow.end(), std::make_move_iterator(begin), std::make_move_iterator(end));
				m_text.back().erase(begin, end);
				while (!m_text.back().empty() && m_text.back().back().ch == ' ') {
					m_text.back().pop_back();
				}
			}
			m_text.push_back(std::move(newRow));
		}
	}
	this->eraseExcessRows();
	if (m_scrollRow != 0) {
		m_scrollRow += newRows;
	}
}

auto Console::clear() -> void {
	m_text.resize(1);
	m_text.back().clear();
	m_scrollRow = 0;
}

auto Console::resetScroll() -> void {
	m_scrollRow = 0;
}

auto Console::scrollUp() -> void {
	++m_scrollRow;
}

auto Console::scrollDown() -> void {
	--m_scrollRow;
	if (m_scrollRow < 0) {
		m_scrollRow = 0;
	}
}

auto Console::scroll(std::ptrdiff_t scrolls) -> void {
	m_scrollRow += scrolls;
	if (m_scrollRow < 0) {
		m_scrollRow = 0;
	}
}

auto Console::eraseExcessRows() -> void {
	if (m_text.size() > m_maxRows) {
		m_text.erase(m_text.begin(), m_text.begin() + static_cast<std::ptrdiff_t>(m_text.size() - m_maxRows));
		if (m_text.empty()) {
			m_text.emplace_back();
		}
	}
}

} // namespace gui
