#ifndef AF2_UTILITIES_SPAN_HPP
#define AF2_UTILITIES_SPAN_HPP

#include <algorithm>   // std::equal, std::lexicographical_compare
#include <array>       // std::array
#include <cassert>     // assert
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <iterator>    // std::data, std::size, std::reverse_iterator, std::make_reverse_iterator
#include <limits>      // std::numeric_limits
#include <type_traits> // std::remove_..._t, std::is_..._v, std::true_type, std::false_type, std::bool_constant
#include <utility>     // std::declval

namespace util {

inline constexpr auto DYNAMIC_SIZE = static_cast<std::size_t>(-1);

template <typename T, std::size_t SIZE = DYNAMIC_SIZE>
class Span;

namespace detail {

template <typename T>
struct is_span_impl : std::false_type {};

template <typename T, std::size_t SIZE>
struct is_span_impl<util::Span<T, SIZE>> : std::true_type {};

template <typename T>
struct is_span : is_span_impl<std::remove_cv_t<T>> {};

template <typename T>
inline constexpr auto is_span_v = is_span<T>::value;

template <typename T>
struct is_std_array_impl : std::false_type {};

template <typename T, std::size_t SIZE>
struct is_std_array_impl<std::array<T, SIZE>> : std::true_type {};

template <typename T>
struct is_std_array : is_std_array_impl<std::remove_cv_t<T>> {};

template <typename T>
inline constexpr auto is_std_array_v = is_std_array<T>::value;

template <std::size_t FROM, std::size_t TO>
struct is_allowed_size_conversion : std::bool_constant<FROM == TO || FROM == util::DYNAMIC_SIZE || TO == util::DYNAMIC_SIZE> {};

template <std::size_t FROM, std::size_t TO>
inline constexpr auto is_allowed_size_conversion_v = is_allowed_size_conversion<FROM, TO>::value;

template <typename From, typename To>
struct is_allowed_element_type_conversion : std::bool_constant<std::is_convertible_v<From (*)[], To (*)[]>> {};

template <typename From, typename To>
inline constexpr auto is_allowed_element_type_conversion_v = is_allowed_element_type_conversion<From, To>::value;

template <typename T, std::size_t SIZE>
struct SpanBase {
	using element_type = T;
	using pointer = element_type*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	constexpr SpanBase() noexcept = default;
	constexpr SpanBase(pointer data, [[maybe_unused]] size_type size)
		: m_data(data) {
		assert(size == SIZE);
	}

	template <size_type N>
	constexpr SpanBase(SpanBase<T, N> other)
		: m_data(other.data()) {
		static_assert(N == SIZE || N == DYNAMIC_SIZE);
		assert(other.size() == SIZE);
	}

	[[nodiscard]] constexpr auto data() const noexcept -> pointer {
		return m_data;
	}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return SIZE;
	}

private:
	pointer m_data = nullptr;
};

template <typename T>
struct SpanBase<T, DYNAMIC_SIZE> {
	using element_type = T;
	using pointer = element_type*;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	constexpr SpanBase() noexcept = default;
	constexpr SpanBase(pointer data, size_type size)
		: m_data(data)
		, m_size(size) {}

	template <size_type N>
	explicit constexpr SpanBase(SpanBase<T, N> other)
		: m_data(other.data())
		, m_size(other.size()) {}

	[[nodiscard]] constexpr auto data() const noexcept -> pointer {
		return m_data;
	}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return m_size;
	}

private:
	pointer m_data = nullptr;
	size_type m_size = 0;
};

template <typename T, std::size_t SIZE, std::size_t OFFSET, std::size_t N>
struct subspan_type {
	using type = util::Span<T, (N != util::DYNAMIC_SIZE) ? N : (SIZE != util::DYNAMIC_SIZE) ? SIZE - OFFSET : SIZE>;
};

template <typename T, std::size_t SIZE, std::size_t OFFSET, std::size_t COUNT>
using subspan_type_t = typename subspan_type<T, SIZE, OFFSET, COUNT>::type;

} // namespace detail

template <typename T, std::size_t SIZE>
class Span final : public detail::SpanBase<T, SIZE> {
public:
	using element_type = typename detail::SpanBase<T, SIZE>::element_type;
	using pointer = typename detail::SpanBase<T, SIZE>::pointer;
	using size_type = typename detail::SpanBase<T, SIZE>::size_type;
	using difference_type = typename detail::SpanBase<T, SIZE>::difference_type;
	using value_type = std::remove_cv_t<element_type>;
	using reference = element_type&;
	using iterator = element_type*;
	using const_iterator = const element_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	constexpr Span() noexcept = default;

	constexpr Span(pointer data, size_type size)
		: detail::SpanBase<T, SIZE>(data, size) {}

	template <typename U, std::size_t N, typename = std::enable_if_t<detail::is_allowed_size_conversion_v<N, SIZE>>,
	          typename = std::enable_if_t<detail::is_allowed_element_type_conversion_v<U, T>>>
	constexpr Span(const Span<U, N>& other)
		: Span(other.data(), other.size()) {}

	constexpr Span(pointer begin, pointer end)
		: Span(begin, end - begin) {}

	template <std::size_t N>
	constexpr Span(element_type (&arr)[N]) noexcept
		: Span(std::data(arr), N) {}

	template <std::size_t N, typename = std::enable_if_t<N != 0>>
	constexpr Span(std::array<value_type, N>& arr) noexcept
		: Span(std::data(arr), N) {}

	constexpr Span(std::array<value_type, 0>&) noexcept
		: Span() {}

	template <std::size_t N, typename = std::enable_if_t<N != 0>>
	constexpr Span(const std::array<value_type, N>& arr) noexcept
		: Span(std::data(arr), N) {}

	constexpr Span(const std::array<value_type, 0>&) noexcept
		: Span() {}

	template <typename Container, typename = std::enable_if_t<!detail::is_span_v<Container>>, typename = std::enable_if_t<!detail::is_std_array_v<Container>>,
	          typename = decltype(std::data(std::declval<Container>())), typename = decltype(std::size(std::declval<Container>())),
	          typename = std::enable_if_t<std::is_convertible_v<typename Container::pointer, pointer>>,
	          typename = std::enable_if_t<std::is_convertible_v<typename Container::pointer, decltype(std::data(std::declval<Container>()))>>>
	constexpr Span(Container& container)
		: Span(std::data(container), std::size(container)) {}

	template <typename Container, typename Element = element_type, typename = std::enable_if_t<std::is_const_v<Element>>,
	          typename = std::enable_if_t<!detail::is_span_v<Container>>, typename = std::enable_if_t<!detail::is_std_array_v<Container>>,
	          typename = decltype(std::data(std::declval<Container>())), typename = decltype(std::size(std::declval<Container>())),
	          typename = std::enable_if_t<std::is_convertible_v<typename Container::pointer, pointer>>,
	          typename = std::enable_if_t<std::is_convertible_v<typename Container::pointer, decltype(std::data(std::declval<Container>()))>>>
	constexpr Span(const Container& container)
		: Span(std::data(container), std::size(container)) {}

	[[nodiscard]] constexpr auto begin() const noexcept -> iterator {
		return this->data();
	}

	[[nodiscard]] constexpr auto cbegin() const noexcept -> const_iterator {
		return this->data();
	}

	[[nodiscard]] constexpr auto end() const noexcept -> iterator {
		return this->data() + this->size();
	}

	[[nodiscard]] constexpr auto cend() const noexcept -> const_iterator {
		return this->data() + this->size();
	}

	[[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->end());
	}

	[[nodiscard]] constexpr auto crbegin() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->cend());
	}

	[[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->begin());
	}

	[[nodiscard]] constexpr auto crend() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->cbegin());
	}

	[[nodiscard]] constexpr auto operator[](size_type i) const noexcept -> reference {
		assert(i < this->size());
		return this->data()[i];
	}

	[[nodiscard]] constexpr auto operator()(size_type i) const noexcept -> reference {
		assert(i < this->size());
		return this->data()[i];
	}

	[[nodiscard]] constexpr auto size_bytes() const noexcept -> size_type {
		return this->size() * sizeof(element_type);
	}

	[[nodiscard]] constexpr auto empty() const noexcept -> bool {
		return this->size() == 0;
	}

	[[nodiscard]] constexpr auto front() const noexcept -> reference {
		assert(this->size() > 0);
		return this->data()[0];
	}

	[[nodiscard]] constexpr auto back() const noexcept -> reference {
		assert(this->size() > 0);
		return this->data()[this->size() - 1];
	}

	template <std::size_t N>
	[[nodiscard]] constexpr auto first() const -> Span<T, N> {
		static_assert(N != DYNAMIC_SIZE && N <= SIZE);
		return {this->data(), N};
	}

	template <std::size_t N>
	[[nodiscard]] constexpr auto last() const -> Span<T, N> {
		static_assert(N != DYNAMIC_SIZE && N <= SIZE);
		return {this->data() + (SIZE - N), N};
	}

	template <std::size_t OFFSET, std::size_t N = DYNAMIC_SIZE>
	[[nodiscard]] constexpr auto subspan() const -> detail::subspan_type_t<T, SIZE, OFFSET, N> {
		static_assert(OFFSET <= SIZE);
		return {this->data() + OFFSET, (N == DYNAMIC_SIZE) ? this->size() - OFFSET : N};
	}

	[[nodiscard]] constexpr auto first(size_type n) const -> Span<T, DYNAMIC_SIZE> {
		assert(n <= this->size());
		return {this->data(), n};
	}

	[[nodiscard]] constexpr auto last(size_type n) const -> Span<T, DYNAMIC_SIZE> {
		return this->subspan(this->size() - n);
	}

	[[nodiscard]] constexpr auto subspan(size_type offset, size_type n = DYNAMIC_SIZE) const -> Span<T, DYNAMIC_SIZE> {
		if constexpr (SIZE == DYNAMIC_SIZE) {
			assert(offset <= this->size());
			if (n == DYNAMIC_SIZE) {
				return {this->data() + offset, this->size() - offset};
			}
			assert(n <= this->size());
			assert(offset + n <= this->size());
			return {this->data() + offset, n};
		} else {
			return Span<T, DYNAMIC_SIZE>{*this}.subspan(offset, n);
		}
	}
};

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator==(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator!=(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return !(lhs == rhs);
}

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator<(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
}

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator<=(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return !(lhs > rhs);
}

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator>(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return rhs < lhs;
}

template <typename T, std::size_t LHS_SIZE, std::size_t RHS_SIZE>
[[nodiscard]] constexpr auto operator>=(Span<T, LHS_SIZE> lhs, Span<T, RHS_SIZE> rhs) -> bool {
	return !(lhs < rhs);
}

template <typename Container>
Span(Container&) -> Span<typename Container::value_type>;

template <typename Container>
Span(const Container&) -> Span<const typename Container::value_type>;

template <typename T, std::size_t N>
Span(T (&)[N]) -> Span<T, N>;

template <typename T, std::size_t N>
Span(std::array<T, N>&) -> Span<T, N>;

template <typename T, std::size_t N>
Span(const std::array<T, N>&) -> Span<const T, N>;

template <typename T, typename Dummy>
Span(T, Dummy&&) -> Span<std::remove_reference_t<decltype(std::declval<T>()[0])>>;

template <typename T, std::size_t N>
auto asBytes(Span<T, N> s) noexcept {
	if constexpr (N == DYNAMIC_SIZE) {
		return Span<const std::byte, DYNAMIC_SIZE>{reinterpret_cast<const std::byte*>(s.data()), s.size_bytes()};
	} else {
		return Span<const std::byte, sizeof(T) * N>{reinterpret_cast<const std::byte*>(s.data()), s.size_bytes()};
	}
}

template <typename T, std::size_t N>
auto asWritableBytes(Span<T, N> s) noexcept {
	if constexpr (N == DYNAMIC_SIZE) {
		return Span<std::byte, DYNAMIC_SIZE>{reinterpret_cast<std::byte*>(s.data()), s.size_bytes()};
	} else {
		return Span<std::byte, sizeof(T) * N>{reinterpret_cast<std::byte*>(s.data()), s.size_bytes()};
	}
}

} // namespace util

#endif
