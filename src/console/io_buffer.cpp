#include "io_buffer.hpp"

#include <utility> // std::move

auto IOBuffer::canRead() const noexcept -> bool {
	return m_str.has_value();
}

auto IOBuffer::isDone() const noexcept -> bool {
	return m_done;
}

auto IOBuffer::setDone(bool done) noexcept -> void {
	m_done = done;
}

auto IOBuffer::write(std::string_view str) -> void {
	if (m_str) {
		m_str->append(str);
	} else {
		m_str = str;
	}
}

auto IOBuffer::writeln(std::string_view str) -> void {
	if (m_str) {
		m_str->push_back('\n');
		m_str->append(str);
	} else {
		m_str = str;
	}
}

auto IOBuffer::read() -> std::optional<std::string> {
	auto moved = std::move(m_str);
	m_str.reset();
	return moved;
}

auto IOBuffer::readln() -> std::optional<std::string> {
	if (m_str) {
		if (const auto newlinePos = m_str->find('\n'); newlinePos != std::string::npos) {
			auto line = m_str->substr(0, newlinePos);
			m_str->erase(0, newlinePos + 1);
			return line;
		}
		return this->read();
	}
	return std::nullopt;
}
