#ifndef AF2_NETWORK_BYTE_STREAM_HPP
#define AF2_NETWORK_BYTE_STREAM_HPP

#include "../utilities/integer.hpp" // util::uint_t
#include "../utilities/span.hpp"    // util::Span, util::DYNAMIC_SIZE, util::asBytes, util::asWritableBytes
#include "platform.hpp"             // net::hton..., net::ntoh...

#include <algorithm>   // std::min, std::max
#include <array>       // std::array
#include <bitset>      // std::bitset
#include <cassert>     // assert
#include <cstddef>     // std::byte, std::size_t, std::ptrdiff_t
#include <cstdint>     // std::int..._t, std::uint..._t
#include <cstring>     // std::memcpy
#include <limits>      // std::numeric_limits
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <tuple>       // std::tuple, std::apply
#include <type_traits> // std::enable_if_t, std::is_..._v, std::underlying_type_t
#include <vector>      // std::vector

namespace net {

class ByteOutputStream final {
private:
	using Container = std::vector<std::byte>;

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

	ByteOutputStream() noexcept = default;

	explicit ByteOutputStream(Container& data)
		: m_data(&data) {}

	auto clear() noexcept -> void {
		assert(m_data);
		m_data->clear();
	}

	auto erase(const_iterator it) -> iterator {
		assert(m_data);
		return m_data->erase(it);
	}

	auto erase(const_iterator begin, const_iterator end) -> iterator {
		assert(m_data);
		return m_data->erase(begin, end);
	}

	auto resize(size_type size) -> void {
		assert(m_data);
		m_data->resize(size);
	}

	auto resize(size_type size, std::byte data) -> void {
		assert(m_data);
		m_data->resize(size, data);
	}

	auto reserve(size_type capacity) -> void {
		assert(m_data);
		m_data->reserve(capacity);
	}

	[[nodiscard]] auto empty() const noexcept -> bool {
		assert(m_data);
		return m_data->empty();
	}

	[[nodiscard]] auto size() const noexcept -> size_type {
		assert(m_data);
		return m_data->size();
	}

	[[nodiscard]] auto capacity() const noexcept -> size_type {
		assert(m_data);
		return m_data->capacity();
	}

	[[nodiscard]] auto operator[](size_type i) noexcept -> reference {
		assert(m_data);
		assert(i < m_data->size());
		return (*m_data)[i];
	}

	[[nodiscard]] auto operator[](size_type i) const noexcept -> const_reference {
		assert(m_data);
		assert(i < m_data->size());
		return (*m_data)[i];
	}

	[[nodiscard]] auto data() noexcept -> pointer {
		assert(m_data);
		return m_data->data();
	}

	[[nodiscard]] auto data() const noexcept -> const_pointer {
		assert(m_data);
		return m_data->data();
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		assert(m_data);
		return m_data->begin();
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		assert(m_data);
		return m_data->begin();
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		assert(m_data);
		return m_data->cbegin();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		assert(m_data);
		return m_data->rbegin();
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		assert(m_data);
		return m_data->rbegin();
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		assert(m_data);
		return m_data->crbegin();
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		assert(m_data);
		return m_data->end();
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		assert(m_data);
		return m_data->end();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		assert(m_data);
		return m_data->cend();
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		assert(m_data);
		return m_data->rend();
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		assert(m_data);
		return m_data->rend();
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		assert(m_data);
		return m_data->crend();
	}

	auto write(std::byte byte) -> void {
		assert(m_data);
		m_data->push_back(byte);
	}

	auto write(size_type n, std::byte byte) -> void {
		assert(m_data);
		m_data->insert(m_data->end(), n, byte);
	}

	auto write(util::Span<const std::byte> bytes) -> void {
		assert(m_data);
		m_data->insert(m_data->end(), bytes.begin(), bytes.end());
	}

	template <typename Integer>
	auto write(Integer val) -> std::enable_if_t<std::is_integral_v<Integer>, void> {
		if constexpr (sizeof(val) == 1) {
			this->write(static_cast<std::byte>(val));
		} else {
			const auto temp = std::array{net::htoni(val)};
			this->write(util::asBytes(util::Span{temp}));
		}
	}

	auto insert(size_type i, std::byte byte) -> void {
		assert(m_data);
		m_data->insert(m_data->begin() + static_cast<difference_type>(i), byte);
	}

	auto insert(size_type i, size_type n, std::byte byte) -> void {
		assert(m_data);
		m_data->insert(m_data->begin() + static_cast<difference_type>(i), n, byte);
	}

	auto insert(size_type i, util::Span<const std::byte> bytes) -> void {
		assert(m_data);
		m_data->insert(m_data->begin() + static_cast<difference_type>(i), bytes.begin(), bytes.end());
	}

	template <typename Integer>
	auto insert(size_type i, Integer val) -> std::enable_if_t<std::is_integral_v<Integer>, void> {
		if constexpr (sizeof(val) == 1) {
			this->insert(i, static_cast<std::byte>(val));
		} else {
			const auto temp = std::array{net::htoni(val)};
			this->insert(i, util::asBytes(util::Span{temp}));
		}
	}

	auto replace(size_type i, std::byte byte) noexcept -> void {
		assert(m_data);
		assert(i < m_data->size());
		(*m_data)[i] = byte;
	}

	auto replace(size_type i, util::Span<const std::byte> bytes) noexcept -> void {
		assert(m_data);
		assert(i + bytes.size() <= m_data->size());
		std::memcpy(m_data->data() + i, bytes.data(), bytes.size());
	}

	template <typename Integer>
	auto replace(size_type i, Integer val) noexcept -> std::enable_if_t<std::is_integral_v<Integer>, void> {
		if constexpr (sizeof(val) == 1) {
			this->replace(i, static_cast<std::byte>(val));
		} else {
			const auto temp = std::array{net::htoni(val)};
			this->replace(i, util::asBytes(util::Span{temp}));
		}
	}

private:
	Container* m_data = nullptr;
};

class ByteInputStream final {
private:
	using ConstBytesView = util::Span<const std::byte>;

public:
	using value_type = typename ConstBytesView::value_type;
	using iterator = typename ConstBytesView::iterator;
	using const_iterator = typename ConstBytesView::iterator;
	using reverse_iterator = typename ConstBytesView::reverse_iterator;
	using const_reverse_iterator = typename ConstBytesView::reverse_iterator;
	using size_type = typename ConstBytesView::size_type;
	using pointer = typename ConstBytesView::pointer;
	using const_pointer = typename ConstBytesView::pointer;
	using reference = typename ConstBytesView::reference;
	using const_reference = typename ConstBytesView::reference;

	ByteInputStream() noexcept = default;

	explicit ByteInputStream(util::Span<const std::byte> bytes)
		: m_data(bytes) {}

	explicit operator bool() const noexcept {
		return m_valid;
	}

	[[nodiscard]] auto valid() const noexcept -> bool {
		return m_valid;
	}

	auto invalidate() noexcept -> void {
		m_valid = false;
	}

	[[nodiscard]] auto eof() const noexcept -> bool {
		return m_data.empty();
	}

	[[nodiscard]] auto empty() const noexcept -> bool {
		return m_data.empty();
	}

	[[nodiscard]] auto size() const noexcept -> size_type {
		return m_data.size();
	}

	[[nodiscard]] auto data() const noexcept -> const_pointer {
		return m_data.data();
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return m_data.begin();
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return m_data.begin();
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return m_data.begin();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		return m_data.rbegin();
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		return m_data.rbegin();
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		return m_data.rbegin();
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return m_data.end();
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return m_data.end();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return m_data.end();
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		return m_data.rend();
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		return m_data.rend();
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		return m_data.rend();
	}

	auto read(util::Span<std::byte> bytes) noexcept -> void {
		if ((m_valid = (m_valid && bytes.size() <= m_data.size()))) {
			std::memcpy(bytes.data(), m_data.data(), bytes.size());
			m_data = m_data.subspan(bytes.size());
		}
	}

	auto read(std::byte& byte) noexcept -> void {
		if ((m_valid = (m_valid && !m_data.empty()))) {
			byte = m_data.front();
			m_data = m_data.subspan(1);
		}
	}

	template <typename Integer>
	auto read(Integer& val) noexcept -> std::enable_if_t<std::is_integral_v<Integer>, void> {
		if constexpr (sizeof(val) == 1) {
			auto temp = std::byte{};
			this->read(temp);
			if (m_valid) {
				val = static_cast<Integer>(temp);
			}
		} else {
			auto temp = std::array<decltype(net::htoni(val)), 1>{};
			this->read(util::asWritableBytes(util::Span{temp}));
			if (m_valid) {
				val = static_cast<Integer>(net::ntohi(temp.front()));
			}
		}
	}

private:
	ConstBytesView m_data{};
	bool m_valid = true;
};

class ByteCountStream final {
public:
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	constexpr auto clear() noexcept -> void {
		m_size = 0;
	}

	constexpr auto operator++() noexcept -> ByteCountStream& {
		++m_size;
		m_capacity = std::max(m_capacity, m_size);
		return *this;
	}

	constexpr auto operator+=(size_type n) noexcept -> ByteCountStream& {
		m_size += n;
		m_capacity = std::max(m_capacity, m_size);
		return *this;
	}

	constexpr auto reserve(size_type n) noexcept -> void {
		m_capacity = std::max(m_capacity, n);
	}

	constexpr auto resize(size_type n) noexcept -> void {
		m_size = n;
		m_capacity = std::max(m_capacity, m_size);
	}

	constexpr auto write(std::byte) noexcept -> void {
		++*this;
	}

	constexpr auto write(util::Span<const std::byte> bytes) noexcept -> void {
		*this += bytes.size();
	}

	template <typename Integer>
	constexpr auto write(Integer) noexcept -> std::enable_if_t<std::is_integral_v<Integer>, void> {
		if constexpr (sizeof(Integer) == 1) {
			++*this;
		} else {
			*this += sizeof(Integer);
		}
	}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return m_size;
	}

	[[nodiscard]] constexpr auto capacity() const noexcept -> size_type {
		return m_capacity;
	}

private:
	size_type m_size = 0;
	size_type m_capacity = 0;
};

inline auto operator<<(ByteOutputStream& stream, std::byte val) -> ByteOutputStream& {
	stream.write(val);
	return stream;
}

inline auto operator>>(ByteInputStream& stream, std::byte& val) noexcept -> ByteInputStream& {
	stream.read(val);
	return stream;
}

constexpr auto operator<<(ByteCountStream& stream, std::byte) noexcept -> ByteCountStream& {
	return ++stream;
}

template <typename Integer>
inline auto operator<<(ByteOutputStream& stream, Integer val) -> std::enable_if_t<std::is_integral_v<Integer>, ByteOutputStream&> {
	stream.write(val);
	return stream;
}

template <typename Integer>
inline auto operator>>(ByteInputStream& stream, Integer& val) noexcept -> std::enable_if_t<std::is_integral_v<Integer>, ByteInputStream&> {
	stream.read(val);
	return stream;
}

template <typename Integer>
constexpr auto operator<<(ByteCountStream& stream, Integer) noexcept -> std::enable_if_t<std::is_integral_v<Integer>, ByteCountStream&> {
	if constexpr (sizeof(Integer) == 1) {
		return ++stream;
	} else {
		return stream += sizeof(Integer);
	}
}

inline auto operator<<(ByteOutputStream& stream, float val) -> ByteOutputStream& {
	const auto temp = std::array{net::htonf(val)};
	stream.write(util::asBytes(util::Span{temp}));
	return stream;
}

inline auto operator>>(ByteInputStream& stream, float& val) noexcept -> ByteInputStream& {
	auto temp = std::array<decltype(net::htonf(val)), 1>{};
	stream.read(util::asWritableBytes(util::Span{temp}));
	if (stream) {
		val = net::ntohf(temp.front());
	}
	return stream;
}

constexpr auto operator<<(ByteCountStream& stream, float) noexcept -> ByteCountStream& {
	return stream += sizeof(float);
}

inline auto operator<<(ByteOutputStream& stream, double val) -> ByteOutputStream& {
	const auto temp = std::array{net::htond(val)};
	stream.write(util::asBytes(util::Span{temp}));
	return stream;
}

inline auto operator>>(ByteInputStream& stream, double& val) noexcept -> ByteInputStream& {
	auto temp = std::array<decltype(net::htond(val)), 1>{};
	stream.read(util::asWritableBytes(util::Span{temp}));
	if (stream) {
		val = net::ntohd(temp.front());
	}
	return stream;
}

constexpr auto operator<<(ByteCountStream& stream, double) noexcept -> ByteCountStream& {
	return stream += sizeof(double);
}

template <typename Enum>
inline auto operator<<(ByteOutputStream& stream, Enum val) -> std::enable_if_t<std::is_enum_v<Enum>, ByteOutputStream&> {
	return stream << static_cast<std::underlying_type_t<Enum>>(val);
}

template <typename Enum>
inline auto operator>>(ByteInputStream& stream, Enum& val) noexcept -> std::enable_if_t<std::is_enum_v<Enum>, ByteInputStream&> {
	if (auto temp = std::underlying_type_t<Enum>{}; stream >> temp) {
		val = static_cast<Enum>(temp);
	}
	return stream;
}

template <typename Enum>
constexpr auto operator<<(ByteCountStream& stream, Enum) noexcept -> std::enable_if_t<std::is_enum_v<Enum>, ByteCountStream&> {
	return stream += sizeof(std::underlying_type_t<Enum>);
}

template <typename T, std::size_t N>
inline auto operator<<(ByteOutputStream& stream, util::Span<T, N> val) -> ByteOutputStream& {
	if constexpr (N == util::DYNAMIC_SIZE) {
		const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
		stream << static_cast<std::uint16_t>(size);
		for (const auto& elem : val.first(size)) {
			stream << elem;
		}
	} else {
		for (const auto& elem : val) {
			stream << elem;
		}
	}
	return stream;
}

template <typename T, std::size_t N>
inline auto operator>>(ByteInputStream& stream, util::Span<T, N> val) -> ByteInputStream& {
	static_assert(N != util::DYNAMIC_SIZE);
	for (auto& elem : val) {
		stream >> elem;
	}
	return stream;
}

template <typename T, std::size_t N>
constexpr auto operator<<(ByteCountStream& stream, util::Span<T, N> val) -> ByteCountStream& {
	if constexpr (N == util::DYNAMIC_SIZE) {
		const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
		stream += sizeof(std::uint16_t);
		for (const auto& elem : val.first(size)) {
			stream << elem;
		}
	} else {
		for (const auto& elem : val) {
			stream << elem;
		}
	}
	return stream;
}

template <typename T, std::size_t N>
inline auto operator<<(ByteOutputStream& stream, const std::array<T, N>& val) -> ByteOutputStream& {
	for (const auto& elem : val) {
		stream << elem;
	}
	return stream;
}

template <typename T, std::size_t N>
inline auto operator>>(ByteInputStream& stream, std::array<T, N>& val) -> ByteInputStream& {
	for (auto& elem : val) {
		stream >> elem;
	}
	return stream;
}

template <typename T, std::size_t N>
constexpr auto operator<<(ByteCountStream& stream, const std::array<T, N>& val) -> ByteCountStream& {
	for (const auto& elem : val) {
		stream << elem;
	}
	return stream;
}

inline auto operator<<(ByteOutputStream& stream, const std::string& val) -> ByteOutputStream& {
	const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	stream << static_cast<std::uint16_t>(size);
	stream.write(util::asBytes(util::Span<const char>{val}.first(size)));
	return stream;
}

inline auto operator>>(ByteInputStream& stream, std::string& val) noexcept -> ByteInputStream& {
	if (auto count = std::uint16_t{}; stream >> count) {
		const auto size = static_cast<std::size_t>(count);
		val.resize(size);
		stream.read(util::asWritableBytes(util::Span<char>{val}));
	}
	return stream;
}

inline auto operator<<(ByteCountStream& stream, const std::string& val) noexcept -> ByteCountStream& {
	const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	return stream += sizeof(std::uint16_t) + size;
}

inline auto operator<<(ByteOutputStream& stream, std::string_view val) -> ByteOutputStream& {
	const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	stream << static_cast<std::uint16_t>(size);
	stream.write(util::asBytes(util::Span<const char>{val}.first(size)));
	return stream;
}

inline auto operator<<(ByteCountStream& stream, std::string_view val) noexcept -> ByteCountStream& {
	const auto size = std::min(val.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	return stream += sizeof(std::uint16_t) + size;
}

inline auto operator<<(ByteOutputStream& stream, const char* val) -> ByteOutputStream& {
	const auto i = stream.size();
	stream.write(2, std::byte{});
	auto size = std::size_t{0};
	for (; *val != '\0' && size < static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()); ++val) {
		stream.write(static_cast<std::byte>(*val));
		++size;
	}
	stream.replace(i, static_cast<std::uint16_t>(size));
	return stream;
}

inline auto operator<<(ByteCountStream& stream, const char* val) noexcept -> ByteCountStream& {
	auto size = std::size_t{0};
	for (; *val != '\0' && size < static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()); ++val) {
		++size;
	}
	return stream += sizeof(std::uint16_t) + size;
}

template <std::size_t N>
inline auto operator<<(ByteOutputStream& stream, const char (&val)[N]) -> ByteOutputStream& {
	constexpr auto size = std::min(N, static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	stream << static_cast<std::uint16_t>(size);
	stream.write(util::asBytes(util::Span<const char, N>{val}.first(size)));
	return stream;
}

template <std::size_t N>
inline auto operator<<(ByteCountStream& stream, const char (&)[N]) noexcept -> ByteCountStream& {
	constexpr auto size = std::min(N, static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()));
	return stream += sizeof(std::uint16_t) + size;
}

template <typename T>
inline auto operator<<(ByteOutputStream& stream, const std::optional<T>& val) -> ByteOutputStream& {
	if (val) {
		stream << true << *val;
	} else {
		stream << false;
	}
	return stream;
}

template <typename T>
inline auto operator>>(ByteInputStream& stream, std::optional<T>& val) -> ByteInputStream& {
	if (auto hasValue = bool{}; stream >> hasValue && hasValue) {
		val.emplace();
		stream >> *val;
	} else {
		val.reset();
	}
	return stream;
}

template <typename T>
constexpr auto operator<<(ByteCountStream& stream, const std::optional<T>& val) -> ByteCountStream& {
	stream.reserve(sizeof(bool) + sizeof(T));
	stream += sizeof(bool);
	if (val) {
		stream << *val;
	}
	return stream;
}

template <typename T>
inline auto operator<<(ByteOutputStream& stream, const std::vector<T>& val) -> ByteOutputStream& {
	return stream << util::Span<const T>{val};
}

template <typename T>
inline auto operator>>(ByteInputStream& stream, std::vector<T>& val) -> ByteInputStream& {
	if (auto count = std::uint16_t{}; stream >> count) {
		const auto size = static_cast<std::size_t>(count);
		val.resize(size);
		for (auto i = std::size_t{0}; i < size; ++i) {
			stream >> val[i];
		}
	}
	return stream;
}

template <typename T>
inline auto operator<<(ByteCountStream& stream, const std::vector<T>& val) -> ByteCountStream& {
	return stream << util::Span<const T>{val};
}

template <std::size_t N>
inline auto operator<<(ByteOutputStream& stream, const std::bitset<N>& val) -> ByteOutputStream& {
	return stream << static_cast<util::uint_t<N>>(val.to_ullong());
}

template <std::size_t N>
inline auto operator>>(ByteInputStream& stream, std::bitset<N>& val) -> ByteInputStream& {
	if (auto temp = util::uint_t<N>{}; stream >> temp) {
		val = static_cast<unsigned long long>(temp);
	}
	return stream;
}

template <std::size_t N>
inline auto operator<<(ByteCountStream& stream, const std::bitset<N>& val) -> ByteCountStream& {
	return stream << static_cast<util::uint_t<N>>(val.to_ullong());
}

template <typename... Ts>
inline auto operator<<(ByteOutputStream& stream, const std::tuple<Ts...>& a) -> ByteOutputStream& {
	std::apply(
		[&](const auto&... args) {
			if constexpr (sizeof...(args) > 0) {
				(stream << ... << args);
			}
		},
		a);
	return stream;
}

template <typename... Ts>
inline auto operator>>(ByteInputStream& stream, std::tuple<Ts...>& a) -> ByteInputStream& {
	std::apply(
		[&](auto&... args) {
			if constexpr (sizeof...(args) > 0) {
				(stream >> ... >> args);
			}
		},
		a);
	return stream;
}

template <typename... Ts>
inline auto operator>>(ByteInputStream& stream, std::tuple<Ts&...> a) -> ByteInputStream& {
	std::apply(
		[&](auto&... args) {
			if constexpr (sizeof...(args) > 0) {
				(stream >> ... >> args);
			}
		},
		a);
	return stream;
}

template <typename... Ts>
constexpr auto operator<<(ByteCountStream& stream, const std::tuple<Ts...>& a) -> ByteCountStream& {
	std::apply(
		[&](const auto&... args) {
			if constexpr (sizeof...(args) > 0) {
				(stream << ... << args);
			}
		},
		a);
	return stream;
}

} // namespace net

#endif
