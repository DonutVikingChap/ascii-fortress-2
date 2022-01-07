#ifndef AF2_UTILITIES_TILE_MATRIX_HPP
#define AF2_UTILITIES_TILE_MATRIX_HPP

#include "algorithm.hpp" // util::copy, util::fill, util::fillN

#include <algorithm>   // std::max
#include <cassert>     // assert
#include <iterator>    // std::back_inserter
#include <string>      // std::basic_string
#include <string_view> // std::basic_string_view
#include <utility>     // std::move
#include <vector>      // std::vector

namespace util {

template <typename Tile>
class TileMatrix final {
private:
	using Container = std::vector<Tile>;

public:
	using size_type = typename Container::size_type;
	using difference_type = typename Container::difference_type;
	using value_type = typename Container::value_type;
	using reference = typename Container::reference;
	using const_reference = typename Container::const_reference;
	using pointer = typename Container::pointer;
	using const_pointer = typename Container::const_pointer;
	using iterator = typename Container::iterator;
	using const_iterator = typename Container::const_iterator;
	using reverse_iterator = typename Container::reverse_iterator;
	using const_reverse_iterator = typename Container::const_reverse_iterator;

	TileMatrix() = default;

	TileMatrix(size_type width, size_type height)
		: m_matrix(width * height)
		, m_width(width)
		, m_height(height) {}

	TileMatrix(size_type width, size_type height, Tile value)
		: m_matrix(width * height, value)
		, m_width(width)
		, m_height(height) {}

	TileMatrix(size_type width, size_type height, Container tiles)
		: m_matrix(std::move(tiles))
		, m_width(width)
		, m_height(height) {
		assert(m_width * m_height == m_matrix.size());
	}

	template <typename It>
	TileMatrix(size_type width, size_type height, It begin, It end)
		: m_matrix(std::move(begin), std::move(end))
		, m_width(width)
		, m_height(height) {
		assert(m_width * m_height == m_matrix.size());
	}

	TileMatrix(size_type width, size_type height, std::initializer_list<Tile> tiles)
		: m_matrix(tiles)
		, m_width(width)
		, m_height(height) {
		assert(m_width * m_height == m_matrix.size());
	}

	explicit TileMatrix(std::basic_string_view<Tile> str, Tile defaultVal = '\0')
		: m_height(1) {
		auto rows = std::vector<Container>{Container{}};
		for (const auto& ch : str) {
			if (ch == static_cast<Tile>('\n')) {
				m_width = std::max(m_width, rows.back().size());
				rows.emplace_back();
				++m_height;
			} else {
				rows.back().push_back(ch);
			}
		}
		m_width = std::max(m_width, rows.back().size());

		m_matrix.reserve(m_width * m_height);
		for (const auto& row : rows) {
			util::copy(row, std::back_inserter(m_matrix));
			util::fillN(std::back_inserter(m_matrix), m_width - row.size(), defaultVal);
		}
	}

	[[nodiscard]] auto getString() const -> std::basic_string<Tile> {
		auto str = std::basic_string<Tile>{};
		if (!m_matrix.empty()) {
			str.reserve(m_matrix.size() + m_height - 1);
			auto x = size_type{0};
			for (const auto& ch : m_matrix) {
				str.push_back(ch);
				++x;
				if (x == m_width) {
					str.push_back(static_cast<Tile>('\n'));
					x = 0;
				}
			}
		}
		return str;
	}

	[[nodiscard]] friend constexpr auto operator==(const TileMatrix& lhs, const TileMatrix& rhs) noexcept -> bool {
		return lhs.m_width == rhs.m_width && lhs.m_height == rhs.m_height && lhs.m_matrix == rhs.m_matrix;
	}

	[[nodiscard]] friend constexpr auto operator!=(const TileMatrix& lhs, const TileMatrix& rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] auto getWidth() const noexcept -> size_type {
		return m_width;
	}

	[[nodiscard]] auto getHeight() const noexcept -> size_type {
		return m_height;
	}

	[[nodiscard]] auto empty() const noexcept -> bool {
		return m_matrix.empty();
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return m_matrix.begin();
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return m_matrix.begin();
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return m_matrix.end();
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return m_matrix.end();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		return m_matrix.rbegin();
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		return m_matrix.rbegin();
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		return m_matrix.rend();
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		return m_matrix.rend();
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return m_matrix.cbegin();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return m_matrix.cend();
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		return m_matrix.crbegin();
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		return m_matrix.crend();
	}

	auto clear() noexcept -> void {
		m_matrix.clear();
		m_width = 0;
		m_height = 0;
	}

	auto resize(size_type width, size_type height) -> void {
		m_matrix.resize(width * height);
		m_width = width;
		m_height = height;
	}

	auto resize(size_type width, size_type height, const Tile& ch) -> void {
		m_matrix.resize(width * height, ch);
		m_width = width;
		m_height = height;
	}

	auto set(size_type x, size_type y, Tile ch) noexcept -> void {
		if (x < m_width && y < m_height) {
			m_matrix[m_width * y + x] = std::move(ch);
		}
	}

	auto fill(const Tile& ch) noexcept -> void {
		util::fill(m_matrix, ch);
	}

	auto draw(size_type x, size_type y, const TileMatrix& other) noexcept -> void {
		for (auto iy = y; iy < m_height && iy - y < other.m_height; ++iy) {
			for (auto ix = x; ix < m_width && ix - x < other.m_width; ++ix) {
				this->setUnchecked(ix, iy, other.getUnchecked(ix - x, iy - y));
			}
		}
	}

	template <typename It>
	auto draw(size_type x, size_type y, It it, It end) noexcept -> void {
		if (y < m_height) {
			for (auto ix = x; ix < m_width && it != end; ++ix, ++it) {
				this->setUnchecked(ix, y, *it);
			}
		}
	}

	auto drawLineVertical(size_type x, size_type y, size_type length, const Tile& ch) noexcept -> void {
		if (x < m_width) {
			for (auto iy = y; iy < m_height && iy - y < length; ++iy) {
				this->setUnchecked(x, iy, ch);
			}
		}
	}

	auto drawLineHorizontal(size_type x, size_type y, size_type length, const Tile& ch) noexcept -> void {
		if (y < m_height) {
			for (auto ix = x; ix < m_width && ix - x < length; ++ix) {
				this->setUnchecked(ix, y, ch);
			}
		}
	}

	auto drawRect(size_type x, size_type y, size_type w, size_type h, const Tile& ch) noexcept -> void {
		this->drawLineHorizontal(x, y, w, ch);
		this->drawLineHorizontal(x, y + h - 1, w, ch);
		this->drawLineVertical(x, y + 1, h - 2, ch);
		this->drawLineVertical(x + w - 1, y + 1, h - 2, ch);
	}

	auto fillRect(size_type x, size_type y, size_type w, size_type h, const Tile& ch) noexcept -> void {
		for (auto iy = y; iy < m_height && iy - y < h; ++iy) {
			for (auto ix = x; ix < m_width && ix - x < w; ++ix) {
				this->setUnchecked(ix, iy, ch);
			}
		}
	}

	[[nodiscard]] auto get(size_type x, size_type y, Tile defaultVal = '\0') const noexcept -> Tile {
		if (x < m_width && y < m_height) {
			return m_matrix[m_width * y + x];
		}
		return defaultVal;
	}

	[[nodiscard]] auto getUnchecked(size_type x, size_type y) const noexcept -> const Tile& {
		assert(m_width * y + x < m_matrix.size());
		return m_matrix[m_width * y + x];
	}

	auto setUnchecked(size_type x, size_type y, Tile ch) noexcept -> void {
		assert(m_width * y + x < m_matrix.size());
		m_matrix[m_width * y + x] = std::move(ch);
	}

	template <typename Stream>
	friend auto operator<<(Stream& stream, const TileMatrix& val) -> Stream& {
		stream << val.m_width << val.m_height;
		for (const auto& ch : val.m_matrix) {
			stream << ch;
		}
		return stream;
	}

	template <typename Stream>
	friend auto operator>>(Stream& stream, TileMatrix& val) -> Stream& {
		stream >> val.m_width >> val.m_height;
		const auto size = val.m_width * val.m_height;
		val.m_matrix.resize(size);
		for (auto i = size_type{0}; i < size; ++i) {
			stream >> val.m_matrix[i];
		}
		return stream;
	}

private:
	Container m_matrix{};
	size_type m_width = 0;
	size_type m_height = 0;
};

} // namespace util

#endif
