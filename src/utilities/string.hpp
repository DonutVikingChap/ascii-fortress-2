#ifndef AF2_UTILITIES_STRING_HPP
#define AF2_UTILITIES_STRING_HPP

#include <algorithm>   // std::transform
#include <cstddef>     // std::size_t
#include <iterator>    // std::begin, std::end, std::iterator_traits, std::forward_iterator_tag
#include <optional>    // std::optional
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::remove_..._t, std::is_..._v, std::conditional_t, std::common_type_t
#include <utility>     // std::move, std::forward, std::declval
// TODO: Always use <charconv> when GCC finally supports to_chars/from_chars for float types.
#ifdef _MSC_VER
#include <charconv>     // std::to_chars, std::from_chars, std::chars_format
#include <system_error> // std::errc
#else
#include <cstdio> // std::snprintf, std::sscanf
#endif

namespace util {
namespace detail {

template <bool STRING_DELIMITER, bool ANY_OF>
struct TokenizeView final {
	using Delimiter = std::conditional_t<STRING_DELIMITER, std::string_view, char>;
	using sentinel = const char*;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<std::string_view::iterator>::difference_type;
		using value_type = std::string_view;
		using reference = value_type&;
		using pointer = value_type*;
		using iterator_category = std::common_type_t<typename std::iterator_traits<std::string_view::iterator>::iterator_category, std::forward_iterator_tag>;

		constexpr iterator() = default;
		constexpr explicit iterator(const TokenizeView& tokenizer)
			: m_tokenizer(&tokenizer) {
			const auto data = m_tokenizer->m_range.data();
			const auto size = m_tokenizer->m_range.size();

			auto i = this->findDelimiter(0);
			if (i == std::string_view::npos) {
				i = size;
			}
			m_view = std::string_view{data, i};
		}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_view == other.m_view;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_view != other.m_view;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_view.data() == other && m_view.empty();
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_view.data() != other || !m_view.empty();
		}

		[[nodiscard]] constexpr auto operator*() -> reference {
			return m_view;
		}

		[[nodiscard]] constexpr auto operator->() -> pointer {
			return std::addressof(**this);
		}

		constexpr auto operator++() -> iterator& {
			this->findNext();
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			auto old = *this;
			++*this;
			return old;
		}

	private:
		[[nodiscard]] constexpr auto delimiterSize() const noexcept -> std::size_t {
			if constexpr (STRING_DELIMITER) {
				return m_tokenizer->m_delimiter.size();
			} else {
				return 1;
			}
		}

		[[nodiscard]] constexpr auto findDelimiter(std::size_t off) const noexcept -> std::size_t {
			if constexpr (ANY_OF) {
				return m_tokenizer->m_range.find_first_of(m_tokenizer->m_delimiter, off);
			} else {
				return m_tokenizer->m_range.find(m_tokenizer->m_delimiter, off);
			}
		}

		constexpr auto findNext() -> void {
			const auto data = m_tokenizer->m_range.data();
			const auto size = m_tokenizer->m_range.size();

			auto i = (m_view.data() - data) + m_view.size();
			if (i == size) {
				m_view = std::string_view{data + size, 0};
				return;
			}

			const auto begin = i + this->delimiterSize();

			i = this->findDelimiter(begin);
			if (i == std::string_view::npos) {
				i = size;
			}
			m_view = std::string_view{data + begin, i - begin};
		}

		std::string_view m_view;
		const TokenizeView* m_tokenizer = nullptr;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{*this};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return m_range.data() + m_range.size();
	}

	std::string_view m_range;
	Delimiter m_delimiter;
};

template <bool STRING_DELIMITER, bool ANY_OF, typename Delimiter>
struct TokenizeViewProxy final {
	Delimiter delimiter;
};

} // namespace detail

// Tokenize a string_view into several string_views according to a certain character delimiter.
// Example: tokenize("abc|def|ghi", '|') -> { "abc", "def", "ghi" }.
[[nodiscard]] constexpr auto tokenize(std::string_view str, char delimiter) {
	return detail::TokenizeView<false, false>{str, delimiter};
}

[[nodiscard]] constexpr auto tokenize(char delimiter) {
	return detail::TokenizeViewProxy<false, false, char>{delimiter};
}

// Tokenize a string_view into several string_views according to a certain string delimiter.
// Example: tokenize("abc||def|ghi", "||") -> { "abc", "def|ghi" }.
[[nodiscard]] constexpr auto tokenize(std::string_view str, std::string_view delimiter) {
	return detail::TokenizeView<true, false>{str, delimiter};
}

[[nodiscard]] constexpr auto tokenize(std::string_view delimiter) {
	return detail::TokenizeViewProxy<true, false, std::string_view>{delimiter};
}

// Tokenize a string_view into several string_views according to any character delimiter in a certain string.
// Example: tokenizeAnyOf("abc|def,ghi", "|,") -> { "abc", "def", "ghi" }.
[[nodiscard]] constexpr auto tokenizeAnyOf(std::string_view str, std::string_view delimiters) {
	return detail::TokenizeView<true, true>{str, delimiters};
}

[[nodiscard]] constexpr auto tokenizeAnyOf(std::string_view delimiters) {
	return detail::TokenizeViewProxy<true, true, std::string_view>{delimiters};
}

} // namespace util

template <bool STRING_DELIMITER, bool ANY_OF, typename Delimiter>
[[nodiscard]] constexpr auto operator|(std::string_view lhs, util::detail::TokenizeViewProxy<STRING_DELIMITER, ANY_OF, Delimiter>&& rhs) {
	return util::detail::TokenizeView<STRING_DELIMITER, ANY_OF>{lhs, rhs.delimiter};
}

namespace util {

// Convert an ASCII char to lower case.
[[nodiscard]] constexpr auto toLower(char ch) noexcept -> char {
	return (ch >= 'A' && ch <= 'Z') ? static_cast<char>(ch + ('a' - 'A')) : ch;
}

// Convert an ASCII char to upper case.
[[nodiscard]] constexpr auto toUpper(char ch) noexcept -> char {
	return (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
}

// Convert an ASCII string to lower case.
[[nodiscard]] inline auto toLower(std::string str) -> std::string {
	std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return util::toLower(ch); });
	return str;
}

// Convert an ASCII string_view to lower case.
[[nodiscard]] inline auto toLower(std::string_view str) -> std::string {
	return util::toLower(std::string{str});
}

// Convert an ASCII string to upper case.
[[nodiscard]] inline auto toUpper(std::string str) -> std::string {
	std::transform(str.begin(), str.end(), str.begin(), [](char ch) { return util::toUpper(ch); });
	return str;
}

// Convert an ASCII string_view to upper case.
[[nodiscard]] inline auto toUpper(std::string_view str) -> std::string {
	return util::toUpper(std::string{str});
}

template <typename T>
[[nodiscard]] inline auto toString(T value) -> std::string {
	auto buffer = std::string(32, '\0');
	// TODO: Always use <charconv> when GCC finally supports to_chars/from_chars for float types.
#ifdef _MSC_VER
	if (const auto& result = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value); result.ec == std::errc{}) {
		buffer.erase(static_cast<std::size_t>(result.ptr - buffer.data()), std::string::npos);
	} else {
		buffer.clear();
	}
#else
	auto charsWritten = 0;

	if constexpr (std::is_floating_point_v<T>) {
		charsWritten = std::snprintf(buffer.data(), buffer.size(), "%f", static_cast<float>(value));
	} else if constexpr (std::is_signed_v<T>) {
		charsWritten = std::snprintf(buffer.data(), buffer.size(), "%d", static_cast<int>(value));
	} else if constexpr (std::is_unsigned_v<T>) {
		charsWritten = std::snprintf(buffer.data(), buffer.size(), "%u", static_cast<unsigned>(value));
	}

	if (charsWritten > 0 && static_cast<std::size_t>(charsWritten) < buffer.size()) {
		buffer.erase(static_cast<std::size_t>(charsWritten), std::string::npos);
		if constexpr (std::is_floating_point_v<T>) {
			if (const auto dotPos = buffer.rfind('.'); dotPos != std::string::npos) {
				while (buffer.size() > dotPos + 2 && buffer.back() == '0') {
					buffer.pop_back();
				}

				if (buffer.size() == dotPos + 2 && buffer.back() == '0') {
					buffer.pop_back();
					buffer.pop_back();
				}
			}
		}
	} else {
		buffer.clear();
	}
#endif
	return buffer;
}

// Convert a string to the given numeric type. Returns std::nullopt if there was a conversion error.
template <typename T>
[[nodiscard]] inline auto stringTo(std::string_view str) noexcept -> std::optional<T> {
	// TODO: Always use <charconv> when GCC finally supports to_chars/from_chars for float types.
#ifdef _MSC_VER
	T value;

	const auto end = str.data() + str.size();
	if (const auto& result = std::from_chars(str.data(), end, value); result.ec != std::errc{} || result.ptr != end) {
		return std::nullopt;
	}
	return value;
#else
	const auto buffer = std::string{str}; // Copy to make sure the string is 0-terminated. Not necessary in the <charconv> version.
	if constexpr (std::is_floating_point_v<T>) {
		auto value = 0.0f;
		auto readChars = 0;
		if (std::sscanf(buffer.c_str(), "%f%n", &value, &readChars) == 1 && static_cast<std::size_t>(readChars) == buffer.size()) {
			return static_cast<T>(value);
		}
	} else if constexpr (std::is_signed_v<T>) {
		auto value = 0;
		auto readChars = 0;
		if (std::sscanf(buffer.c_str(), "%d%n", &value, &readChars) == 1 && static_cast<std::size_t>(readChars) == buffer.size()) {
			return static_cast<T>(value);
		}
	} else if constexpr (std::is_unsigned_v<T>) {
		auto value = 0u;
		auto readChars = 0;
		if (std::sscanf(buffer.c_str(), "%u%n", &value, &readChars) == 1 && static_cast<std::size_t>(readChars) == buffer.size()) {
			return static_cast<T>(value);
		}
	}
	return std::nullopt;
#endif
}

// Convert a string to the given numeric type. Returns false if there was a conversion error, otherwise true.
template <typename T>
inline auto stringTo(T& value, std::string_view str) noexcept -> bool {
	// TODO: Always use <charconv> when GCC finally supports to_chars/from_chars for float types.
#ifdef _MSC_VER
	const auto end = str.data() + str.size();

	const auto& [ptr, ec] = std::from_chars(str.data(), end, value);
	return ec == std::errc{} && ptr == end;
#else
	if (auto result = util::stringTo<T>(str)) {
		value = *result;
		return true;
	}
	return false;
#endif
}

// Check if a string contains a substring.
[[nodiscard]] constexpr auto contains(std::string_view str, std::string_view substr) noexcept -> bool {
	return str.find(substr) != std::string_view::npos;
}

// Check if a string contains a character.
[[nodiscard]] constexpr auto contains(std::string_view str, char ch) noexcept -> bool {
	return str.find(ch) != std::string_view::npos;
}

// Case insensitive equality comparison.
[[nodiscard]] constexpr auto iequals(std::string_view lhs, std::string_view rhs) noexcept -> bool {
	if (lhs.size() != rhs.size()) {
		return false;
	}

	for (auto itLhs = lhs.begin(), itRhs = rhs.begin(); itLhs != lhs.end(); ++itLhs, ++itRhs) {
		if (util::toLower(*itLhs) != util::toLower(*itRhs)) {
			return false;
		}
	}
	return true;
}

// Case insensitive find. Returns the index to the first string, or npos if the substring was not found.
[[nodiscard]] constexpr auto ifind(std::string_view str, std::string_view substr) noexcept -> std::size_t {
	for (auto i = std::size_t{0};; ++i) {
		auto subI = i;
		for (auto j = std::size_t{0};; ++j, ++subI) {
			if (j == substr.size()) {
				return i;
			}
			if (subI == str.size()) {
				return std::string_view::npos;
			}
			if (util::toLower(str[subI]) != util::toLower(substr[j])) {
				break;
			}
		}
	}
	return std::string_view::npos;
}

// Case insensitive find. Returns the index to the first string, or npos if the character was not found.
[[nodiscard]] constexpr auto ifind(std::string_view str, char ch) noexcept -> std::size_t {
	const auto lowerCh = util::toLower(ch);
	for (auto i = std::size_t{0}; i != str.size(); ++i) {
		if (util::toLower(str[i]) == lowerCh) {
			return i;
		}
	}
	return std::string_view::npos;
}

// Case insensitive reverse find. Returns the index to the first string, or npos if the substring was not found.
[[nodiscard]] constexpr auto irfind(std::string_view str, std::string_view substr) noexcept -> std::size_t {
	for (auto i = str.size();; --i) {
		auto subI = i;
		for (auto j = substr.size();;) {
			if (j-- == 0) {
				return i - substr.size();
			}
			if (subI-- == 0) {
				return std::string_view::npos;
			}
			if (util::toLower(str[subI]) != util::toLower(substr[j])) {
				break;
			}
		}
	}
	return std::string_view::npos;
}

// Case insensitive reverse find. Returns the index to the first string, or npos if the character was not found.
[[nodiscard]] constexpr auto irfind(std::string_view str, char ch) noexcept -> std::size_t {
	const auto lowerCh = util::toLower(ch);
	for (auto i = str.size(); i-- != 0;) {
		if (util::toLower(str[i]) == lowerCh) {
			return i;
		}
	}
	return std::string_view::npos;
}

// Case insensitive contains.
[[nodiscard]] constexpr auto icontains(std::string_view str, std::string_view substr) noexcept -> bool {
	return util::ifind(str, substr) != std::string_view::npos;
}

// Case insensitive contains.
[[nodiscard]] constexpr auto icontains(std::string_view str, char ch) noexcept -> bool {
	return util::ifind(str, ch) != std::string_view::npos;
}

// Case insensitive comparison.
[[nodiscard]] inline auto icompare(std::string_view lhs, std::string_view rhs) noexcept -> int {
	return util::toLower(lhs).compare(util::toLower(rhs));
}

// Concatenate a range of strings.
template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] inline auto concat(Range&& range) -> std::string {
	auto result = std::string{};
	for (auto&& str : range) {
		result.append(str);
	}
	return result;
}

// Concatenate a range of strings and add a delimiter between each string.
template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] inline auto join(Range&& range, char delimiter) -> std::string {
	auto result = std::string{};

	auto&& it = std::begin(range);
	auto&& end = std::end(range);
	if (it != end) {
		while (true) {
			result.append(*it);
			++it;
			if (it == end) {
				break;
			}
			result.push_back(delimiter);
		}
	}
	return result;
}

// Concatenate a range of strings and add a delimiter between each string.
template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] inline auto join(Range&& range, std::string_view delimiter) -> std::string {
	auto result = std::string{};

	auto&& it = std::begin(range);
	auto&& end = std::end(range);
	if (it != end) {
		while (true) {
			result.append(*it);
			++it;
			if (it == end) {
				break;
			}
			result.append(delimiter);
		}
	}
	return result;
}

namespace detail {

struct ConcatProxy final {};

struct JoinProxyCharDelimiter final {
	char delimiter;
};

struct JoinProxyStringDelimiter final {
	std::string_view delimiter;
};

} // namespace detail

[[nodiscard]] constexpr auto concat() {
	return util::detail::ConcatProxy{};
}

[[nodiscard]] constexpr auto join(char delimiter) {
	return util::detail::JoinProxyCharDelimiter{delimiter};
}

[[nodiscard]] constexpr auto join(std::string_view delimiter) {
	return util::detail::JoinProxyStringDelimiter{delimiter};
}

} // namespace util

template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] auto operator|(Range&& lhs, util::detail::ConcatProxy&&) -> std::string {
	return util::concat(std::forward<Range>(lhs));
}

template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] auto operator|(Range&& lhs, util::detail::JoinProxyCharDelimiter&& rhs) -> std::string {
	return util::join(std::forward<Range>(lhs), rhs.delimiter);
}

template <typename Range, typename = decltype(std::begin(std::declval<Range>())), typename = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] auto operator|(Range&& lhs, util::detail::JoinProxyStringDelimiter&& rhs) -> std::string {
	return util::join(std::forward<Range>(lhs), rhs.delimiter);
}

#endif
