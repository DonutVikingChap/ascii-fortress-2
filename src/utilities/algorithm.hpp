#ifndef AF2_UTILITIES_ALGORITHM_HPP
#define AF2_UTILITIES_ALGORITHM_HPP

#include "arrow_proxy.hpp" // util::ArrowProxy, util:arrowOf

#include <algorithm> // std::remove, std::remove_if, std::copy, std::copy_if, std::move, std::fill, std::fill_n, std::find, std::find_if, std::all_of, std::none_of, std::any_of, std::count, std::count_if, std::sort, std::stable_sort
#include <cstddef>   // std::size_t, std::ptrdiff_t
#include <iterator>  // std::begin, std::end, std::next, std::advance, std::iterator_traits, std::..._iterator_tag
#include <memory>    // std::addressof
#include <tuple>     // std::tie
#include <type_traits> // std::remove_..._t, std::is_..._v, std::enable_if_t, std::common_type_t, std::false_type, std::true_type
#include <utility>     // std::declval, std::forward, std::move, std::pair, std::make_pair

namespace util {
namespace detail {

template <typename Iterator, typename Sentinel>
struct View final {
public:
	using sentinel = Sentinel;
	using iterator = Iterator;

	template <typename Begin, typename End>
	constexpr View(Begin&& begin, End&& end) noexcept(std::is_nothrow_constructible_v<iterator, Begin>&& std::is_nothrow_constructible_v<sentinel, End>)
		: m_begin(std::forward<Begin>(begin))
		, m_end(std::forward<End>(end)) {}

	[[nodiscard]] friend constexpr auto operator==(const View& lhs, const View& rhs) noexcept -> bool {
		return lhs.m_begin == rhs.m_begin && lhs.m_end == rhs.m_end;
	}

	[[nodiscard]] friend constexpr auto operator!=(const View& lhs, const View& rhs) noexcept -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] constexpr auto begin() const noexcept(std::is_nothrow_copy_constructible_v<iterator>) -> iterator {
		return m_begin;
	}

	[[nodiscard]] constexpr auto end() const noexcept(std::is_nothrow_copy_constructible_v<sentinel>) -> sentinel {
		return m_end;
	}

private:
	iterator m_begin;
	sentinel m_end;
};

} // namespace detail

// Get a view of all elements in a range.
template <typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto view(Range&& range) {
	return detail::View<Iterator, Sentinel>{std::begin(range), std::end(range)};
}

// Get a view of all elements in a range.
template <typename Iterator, typename Sentinel>
[[nodiscard]] constexpr auto view(Iterator begin, Sentinel end) {
	return detail::View<Iterator, Sentinel>{begin, end};
}

// Get a view of some elements in a range.
template <typename Range, typename Offset, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto subview(Range&& range, const Offset& offset) {
	using Diff = typename std::iterator_traits<Iterator>::difference_type;
	return detail::View<Iterator, Sentinel>{std::next(std::begin(range), static_cast<Diff>(offset)), std::end(range)};
}

// Get a view of some elements in a range.
template <typename Range, typename Offset, typename Count, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto subview(Range&& range, const Offset& offset, const Count& count) {
	using Diff = typename std::iterator_traits<Iterator>::difference_type;
	auto&& it = std::begin(range);
	std::advance(it, static_cast<Diff>(offset));
	return detail::View<Iterator, Sentinel>{it, std::next(it, static_cast<Diff>(count))};
}

namespace detail {

template <typename Range, typename Iterator, typename Sentinel>
struct TakeView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = typename std::iterator_traits<Iterator>::value_type;
		using reference = typename std::iterator_traits<Iterator>::reference;
		using pointer = typename std::iterator_traits<Iterator>::pointer;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::bidirectional_iterator_tag>;

		constexpr iterator() = default;
		constexpr iterator(Iterator it, difference_type n)
			: m_it(std::move(it))
			, m_n(n) {}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it && m_n == other.m_n;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return !(*this == other);
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_n == 0 || m_it == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return !(*this == other);
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return *m_it;
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return util::arrowOf(m_it);
		}

		constexpr auto operator++() -> iterator& {
			++m_it;
			--m_n;
			return *this;
		}

		constexpr auto operator--() -> iterator& {
			--m_it;
			++m_n;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{m_it++, m_n--};
		}

		constexpr auto operator--(int) -> iterator {
			return iterator{m_it--, m_n++};
		}

	private:
		Iterator m_it;
		difference_type m_n = 0;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range), m_n};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
	typename iterator::difference_type m_n;
};

template <typename Count>
struct TakeViewProxy final {
	Count n;
};

} // namespace detail

// Get a view of the first n elements in a range.
template <typename Range, typename Count, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto take(Range&& range, const Count& n) {
	using Diff = typename std::iterator_traits<Iterator>::difference_type;
	return detail::TakeView<Range, Iterator, Sentinel>{std::forward<Range>(range), static_cast<Diff>(n)};
}

template <typename Count>
[[nodiscard]] constexpr auto take(Count&& n) {
	return detail::TakeViewProxy<Count>{std::forward<Count>(n)};
}

} // namespace util

template <typename Range, typename Count, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::TakeViewProxy<Count>&& rhs) {
	return util::take(std::forward<Range>(lhs), std::forward<Count>(rhs.n));
}

namespace util {
namespace detail {

template <typename Offset>
struct DropProxy final {
	Offset n;
};

} // namespace detail

// Get a view of all elements in a range except the first n.
template <typename Range, typename Offset, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto drop(Range&& range, const Offset& n) {
	using Diff = typename std::iterator_traits<Iterator>::difference_type;
	using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;

	auto&& it = std::begin(range);
	auto&& end = std::end(range);
	if constexpr (std::is_convertible_v<IteratorCategory, std::random_access_iterator_tag>) {
		return detail::View<Iterator, Sentinel>{std::next(it, std::min(static_cast<Diff>(n), std::distance(it, end))), end};
	} else {
		for (auto i = Offset{0}; i < n && it != end; ++i) {
			++it;
		}
		return detail::View<Iterator, Sentinel>{std::move(it), std::move(end)};
	}
}

template <typename Offset>
[[nodiscard]] constexpr auto drop(Offset&& n) {
	return detail::DropProxy<Offset>{std::forward<Offset>(n)};
}

} // namespace util

template <typename Range, typename Offset, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::DropProxy<Offset>&& rhs) {
	return util::drop(std::forward<Range>(lhs), std::forward<Offset>(rhs.n));
}

namespace util {
namespace detail {

template <typename Range, typename Delimiter, typename Iterator, typename Sentinel>
struct SplitView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = View<Iterator, Iterator>;
		using reference = value_type&;
		using pointer = value_type*;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>;

		constexpr iterator() = default;
		constexpr explicit iterator(const SplitView& splitter)
			: m_view(std::begin(splitter.m_range), std::begin(splitter.m_range))
			, m_splitter(&splitter) {
			auto&& it = m_view.end();
			auto&& end = std::end(m_splitter->m_range);
			do {
				if (*it == m_splitter->m_delimiter) {
					break;
				}
				++it;
			} while (it != end);
			m_view = value_type{m_view.begin(), it};
		}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_view == other.m_view;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_view != other.m_view;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_view.begin() == other && m_view.end() == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_view.begin() != other || m_view.end() != other;
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
		constexpr auto findNext() -> void {
			auto&& it = m_view.end();
			auto&& end = m_splitter->end();
			if (it == end) {
				m_view = value_type{it, it};
				return;
			}
			auto&& begin = it;
			++begin;
			do {
				if (*it == m_splitter->m_delimiter) {
					break;
				}
				++it;
			} while (it != end);
			m_view = value_type{begin, it};
		}

		value_type m_view{};
		const SplitView* m_splitter = nullptr;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{*this};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
	Delimiter m_delimiter;
};

template <typename Delimiter>
struct SplitViewProxy final {
	Delimiter delimiter;
};

} // namespace detail

// Split a range into several views according to a certain delimiter value.
// Example: split( "abc|def|ghi", '|' ) -> { "abc", "def", "ghi" }.
template <typename Range, typename Delimiter, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto split(Range&& range, Delimiter&& delimiter) {
	return detail::SplitView<Range, Delimiter, Iterator, Sentinel>{std::forward<Range>(range), std::forward<Delimiter>(delimiter)};
}

template <typename Delimiter>
[[nodiscard]] constexpr auto split(Delimiter&& delimiter) {
	return detail::SplitViewProxy<Delimiter>{std::forward<Delimiter>(delimiter)};
}

} // namespace util

template <typename Range, typename Delimiter, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::SplitViewProxy<Delimiter>&& rhs) {
	return util::split(std::forward<Range>(lhs), std::forward<Delimiter>(rhs.delimiter));
}

namespace util {
namespace detail {

template <typename Range, typename Function, typename Iterator, typename Sentinel>
struct TransformView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = std::remove_reference_t<decltype(std::declval<Function>()(*std::declval<Iterator>()))>;
		using reference = decltype(std::declval<Function>()(*std::declval<Iterator>()));
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>;

		constexpr iterator() = default;
		constexpr iterator(Iterator it, const TransformView& adaptor)
			: m_it(std::move(it))
			, m_adaptor(&adaptor) {}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it != other.m_it;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_it == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_it != other;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return m_adaptor->m_func(*m_it);
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++() -> iterator& {
			++m_it;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{*m_adaptor, m_it++};
		}

	private:
		Iterator m_it{};
		const TransformView* m_adaptor = nullptr;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range), *this};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
	Function m_func;
};

template <typename Function>
struct TransformViewProxy final {
	Function func;
};

} // namespace detail

// Transform a range to return the result of an adaptor function on iteration.
template <typename Range, typename Function, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto transform(Range&& range, Function&& func) {
	return detail::TransformView<Range, Function, Iterator, Sentinel>{std::forward<Range>(range), std::forward<Function>(func)};
}

template <typename Function>
[[nodiscard]] constexpr auto transform(Function&& func) {
	return detail::TransformViewProxy<Function>{std::forward<Function>(func)};
}

} // namespace util

template <typename Range, typename Function, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::TransformViewProxy<Function>&& rhs) {
	return util::transform(std::forward<Range>(lhs), std::forward<Function>(rhs.func));
}

namespace util {
namespace detail {

template <typename Range, typename Iterator, typename Sentinel>
struct EnumerateView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = typename std::iterator_traits<Iterator>::value_type;
		using reference = std::pair<const std::size_t&, decltype(*std::declval<const Iterator>())>;
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::bidirectional_iterator_tag>;

		constexpr iterator() = default;
		constexpr iterator(Iterator it, std::size_t index)
			: m_it(std::move(it))
			, m_index(index) {}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it != other.m_it;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_it == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_it != other;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return reference{m_index, *m_it};
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++() -> iterator& {
			++m_it;
			++m_index;
			return *this;
		}

		constexpr auto operator--() -> iterator& {
			--m_it;
			--m_index;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{m_it++, m_index++};
		}

		constexpr auto operator--(int) -> iterator {
			return iterator{m_it--, m_index--};
		}

	private:
		Iterator m_it{};
		std::size_t m_index = 0;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range), 0};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
};

struct EnumerateViewProxy final {};

} // namespace detail

// Enumerate the elements of a range along with their index.
template <typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto enumerate(Range&& range) {
	return detail::EnumerateView<Range, Iterator, Sentinel>{std::forward<Range>(range)};
}

[[nodiscard]] constexpr auto enumerate() {
	return detail::EnumerateViewProxy{};
}

} // namespace util

template <typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::EnumerateViewProxy&&) {
	return util::enumerate(std::forward<Range>(lhs));
}

namespace util {
namespace detail {

template <typename Range, typename Predicate, typename Iterator, typename Sentinel>
struct FilterView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = typename std::iterator_traits<Iterator>::value_type;
		using reference = typename std::iterator_traits<Iterator>::reference;
		using pointer = typename std::iterator_traits<Iterator>::pointer;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::forward_iterator_tag>;

		constexpr iterator() = default;
		constexpr iterator(Iterator it, const FilterView& filter)
			: m_it(std::move(it))
			, m_filter(&filter) {
			this->skip();
		}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it != other.m_it;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_it == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_it != other;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return *m_it;
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return util::arrowOf(m_it);
		}

		constexpr auto operator++() -> iterator& {
			++m_it;
			this->skip();
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			auto old = *this;
			++*this;
			return old;
		}

	private:
		constexpr auto skip() -> void {
			auto&& end = std::end(m_filter->m_range);
			while (m_it != end && !m_filter->m_pred(*m_it)) {
				++m_it;
			}
		}

		Iterator m_it{};
		const FilterView* m_filter = nullptr;
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range), *this};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
	Predicate m_pred;
};

template <typename Predicate>
struct FilterViewProxy final {
	Predicate pred;
};

} // namespace detail

// Filter the elements of a range based on a predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto filter(Range&& range, Predicate&& pred) {
	return detail::FilterView<Range, Predicate, Iterator, Sentinel>{std::forward<Range>(range), std::forward<Predicate>(pred)};
}

template <typename Predicate>
[[nodiscard]] constexpr auto filter(Predicate&& pred) {
	return detail::FilterViewProxy<Predicate>{std::forward<Predicate>(pred)};
}

} // namespace util

template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::FilterViewProxy<Predicate>&& rhs) {
	return util::filter(std::forward<Range>(lhs), std::forward<Predicate>(rhs.pred));
}

namespace util {
namespace detail {

template <typename Range, typename Iterator, typename Sentinel>
struct AdjacentView final {
	using sentinel = Sentinel;
	class iterator final {
	public:
		using difference_type = typename std::iterator_traits<Iterator>::difference_type;
		using value_type = typename std::iterator_traits<Iterator>::value_type;
		using reference = std::pair<decltype(*std::declval<const Iterator>()), decltype(*std::declval<const Iterator>())>;
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator>::iterator_category, std::bidirectional_iterator_tag>;

		constexpr iterator() = default;
		constexpr explicit iterator(Iterator it)
			: m_it(std::move(it)) {}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it || std::next(m_it) == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it != other.m_it && std::next(m_it) != other.m_it;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_it == other || std::next(m_it) == other;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_it != other && std::next(m_it) != other;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return reference{*m_it, *std::next(m_it)};
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++() -> iterator& {
			++m_it;
			return *this;
		}

		constexpr auto operator--() -> iterator& {
			--m_it;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{m_it++};
		}

		constexpr auto operator--(int) -> iterator {
			return iterator{m_it--};
		}

	private:
		Iterator m_it{};
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range)};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return std::end(m_range);
	}

	Range m_range;
};

struct AdjacentViewProxy final {};

} // namespace detail

// Iterate adjacent elements in a range.
template <typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto adjacent(Range&& range) {
	return detail::AdjacentView<Range, Iterator, Sentinel>{std::forward<Range>(range)};
}

[[nodiscard]] constexpr auto adjacent() {
	return detail::AdjacentViewProxy{};
}

} // namespace util

template <typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::AdjacentViewProxy&&) {
	return util::adjacent(std::forward<Range>(lhs));
}

namespace util {
namespace detail {

template <typename Range1, typename Range2, typename Iterator1, typename Iterator2, typename Sentinel1, typename Sentinel2>
struct ZipView final {
	using sentinel = std::pair<Sentinel1, Sentinel2>;
	class iterator final {
	public:
		using difference_type = std::common_type_t<typename std::iterator_traits<Iterator1>::difference_type, typename std::iterator_traits<Iterator2>::difference_type>;
		using value_type =
			std::pair<std::remove_reference_t<decltype(*std::declval<const Iterator1>())>, std::remove_reference_t<decltype(*std::declval<const Iterator2>())>>;
		using reference = std::pair<decltype(*std::declval<const Iterator1>()), decltype(*std::declval<const Iterator2>())>;
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::common_type_t<typename std::iterator_traits<Iterator1>::iterator_category,
		                                             typename std::iterator_traits<Iterator2>::iterator_category, std::bidirectional_iterator_tag>;

		constexpr iterator() = default;
		constexpr explicit iterator(Iterator1 it1, Iterator2 it2)
			: m_it1(std::move(it1))
			, m_it2(std::move(it2)) {}

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it1 == other.m_it1 && m_it2 == other.m_it2;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it1 != other.m_it1 || m_it2 != other.m_it2;
		}

		[[nodiscard]] constexpr auto operator==(const sentinel& other) const -> bool {
			return m_it1 == other.first || m_it2 == other.second;
		}

		[[nodiscard]] constexpr auto operator!=(const sentinel& other) const -> bool {
			return m_it1 != other.first && m_it2 != other.second;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return reference{*m_it1, *m_it2};
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++() -> iterator& {
			++m_it1;
			++m_it2;
			return *this;
		}

		constexpr auto operator--() -> iterator& {
			--m_it1;
			--m_it2;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{m_it1++, m_it2++};
		}

		constexpr auto operator--(int) -> iterator {
			return iterator{m_it1--, m_it2++};
		}

	private:
		Iterator1 m_it1{};
		Iterator2 m_it2{};
	};

	[[nodiscard]] constexpr auto begin() const -> iterator {
		return iterator{std::begin(m_range1), std::begin(m_range2)};
	}

	[[nodiscard]] constexpr auto end() const -> sentinel {
		return sentinel{std::end(m_range1), std::end(m_range2)};
	}

	Range1 m_range1;
	Range2 m_range2;
};

template <typename Range2>
struct ZipViewProxy final {
	Range2 range2;
};

} // namespace detail

// Iterate two ranges pairwise.
template <typename Range1, typename Range2, typename Iterator1 = decltype(std::begin(std::declval<Range1>())),
          typename Iterator2 = decltype(std::begin(std::declval<Range2>())),
          typename Sentinel1 = decltype(std::end(std::declval<Range1>())), typename Sentinel2 = decltype(std::end(std::declval<Range2>()))>
[[nodiscard]] constexpr auto zip(Range1&& range1, Range2&& range2) {
	return detail::ZipView<Range1, Range2, Iterator1, Iterator2, Sentinel1, Sentinel2>{std::forward<Range1>(range1), std::forward<Range2>(range2)};
}

template <typename Range2, typename Iterator2 = decltype(std::begin(std::declval<Range2>())), typename Sentinel2 = decltype(std::end(std::declval<Range2>()))>
[[nodiscard]] constexpr auto zip(Range2&& range2) {
	return detail::ZipViewProxy<Range2>{std::forward<Range2>(range2)};
}

} // namespace util

template <typename Range1, typename Range2, typename Iterator1 = decltype(std::begin(std::declval<Range1>())),
          typename Iterator2 = decltype(std::begin(std::declval<Range2>())),
          typename Sentinel1 = decltype(std::end(std::declval<Range1>())), typename Sentinel2 = decltype(std::end(std::declval<Range2>()))>
[[nodiscard]] constexpr auto operator|(Range1&& lhs, util::detail::ZipViewProxy<Range2>&& rhs) {
	return util::zip(std::forward<Range1>(lhs), std::forward<Range2>(rhs.range2));
}

namespace util {
namespace detail {

template <typename T>
class IotaIterator final {
public:
	using difference_type = std::ptrdiff_t;
	using value_type = const T;
	using reference = value_type&;
	using pointer = value_type*;
	using iterator_category = std::random_access_iterator_tag;

	constexpr IotaIterator() noexcept = default;
	explicit constexpr IotaIterator(T value) noexcept
		: m_value(value) {}

	[[nodiscard]] constexpr auto operator==(const IotaIterator& other) const noexcept -> bool {
		return m_value == other.m_value;
	}

	[[nodiscard]] constexpr auto operator!=(const IotaIterator& other) const noexcept -> bool {
		return m_value != other.m_value;
	}

	[[nodiscard]] constexpr auto operator<(const IotaIterator& other) const noexcept -> bool {
		return m_value < other.m_value;
	}

	[[nodiscard]] constexpr auto operator>(const IotaIterator& other) const noexcept -> bool {
		return m_value > other.m_value;
	}

	[[nodiscard]] constexpr auto operator<=(const IotaIterator& other) const noexcept -> bool {
		return m_value <= other.m_value;
	}

	[[nodiscard]] constexpr auto operator>=(const IotaIterator& other) const noexcept -> bool {
		return m_value >= other.m_value;
	}

	[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
		return m_value;
	}

	[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
		return std::addressof(**this);
	}

	constexpr auto operator++() noexcept -> IotaIterator& {
		++m_value;
		return *this;
	}

	constexpr auto operator--() noexcept -> IotaIterator& {
		--m_value;
		return *this;
	}

	constexpr auto operator++(int) noexcept -> IotaIterator {
		return IotaIterator{m_value++};
	}

	constexpr auto operator--(int) noexcept -> IotaIterator {
		return IotaIterator{m_value--};
	}

	constexpr auto operator+=(difference_type n) noexcept -> IotaIterator& {
		m_value += n;
		return *this;
	}

	constexpr auto operator-=(difference_type n) noexcept -> IotaIterator& {
		m_value -= n;
		return *this;
	}

	[[nodiscard]] constexpr auto operator[](difference_type n) noexcept -> T {
		return m_value + static_cast<T>(n);
	}

	[[nodiscard]] constexpr friend auto operator+(const IotaIterator& lhs, difference_type rhs) noexcept -> IotaIterator {
		return IotaIterator{lhs.m_value + rhs};
	}

	[[nodiscard]] constexpr friend auto operator+(difference_type lhs, const IotaIterator& rhs) noexcept -> IotaIterator {
		return IotaIterator{lhs + rhs.m_value};
	}

	[[nodiscard]] constexpr friend auto operator-(const IotaIterator& lhs, difference_type rhs) noexcept -> IotaIterator {
		return IotaIterator{lhs.m_value - rhs};
	}

	[[nodiscard]] constexpr friend auto operator-(const IotaIterator& lhs, const IotaIterator& rhs) noexcept -> difference_type {
		return static_cast<difference_type>(lhs.m_value - rhs.m_value);
	}

private:
	T m_value{};
};

template <typename T>
struct IotaView final {
	using iterator = IotaIterator<T>;
	using sentinel = iterator;

	template <typename First, typename Last>
	constexpr IotaView(First&& first, Last&& last) noexcept
		: m_begin(std::forward<First>(first))
		, m_end(std::forward<Last>(last)) {}

	[[nodiscard]] constexpr auto begin() const noexcept -> iterator {
		return m_begin;
	}

	[[nodiscard]] constexpr auto end() const noexcept -> sentinel {
		return m_end;
	}

	iterator m_begin;
	sentinel m_end;
};

template <typename T>
struct InfiniteIotaView final {
	using iterator = IotaIterator<T>;
	struct sentinel final {
		friend constexpr auto operator==(const iterator&, const sentinel&) noexcept -> bool {
			return false;
		}

		friend constexpr auto operator!=(const iterator&, const sentinel&) noexcept -> bool {
			return true;
		}
	};

	template <typename First>
	constexpr InfiniteIotaView(First&& first) noexcept
		: m_begin(std::forward<First>(first)) {}

	[[nodiscard]] constexpr auto begin() const noexcept -> iterator {
		return m_begin;
	}

	[[nodiscard]] constexpr auto end() const noexcept -> sentinel {
		return sentinel{};
	}

	iterator m_begin;
};

} // namespace detail

// Get a range of all values from first (inclusive) to last (exclusive).
template <typename T, typename Last>
[[nodiscard]] constexpr auto iota(T&& first, Last&& last) {
	return detail::IotaView<T>{std::forward<T>(first), std::forward<Last>(last)};
}

// Get a range of all values from first (inclusive) to infinity.
template <typename T>
[[nodiscard]] constexpr auto iota(T&& first) {
	return detail::InfiniteIotaView<T>{std::forward<T>(first)};
}

namespace detail {

template <typename T, typename = int>
struct has_reserve : std::false_type {};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
#endif

template <typename T>
struct has_reserve<T, decltype(T::reserve, 0)> : std::true_type {};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

template <typename T>
inline constexpr auto has_reserve_v = has_reserve<T>::value;

} // namespace detail

// Copy a range into an output iterator.
template <typename Range, typename OutputIterator, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto copy(const Range& range, OutputIterator output) {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::copy(std::begin(range), std::end(range), std::move(output));
	} else {
		for (const auto& elem : range) {
			*output++ = elem;
		}
		return output;
	}
}

// Copy all elements in a range that match a predicate into an output iterator.
template <typename Range, typename OutputIterator, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto copyIf(const Range& range, OutputIterator output, Predicate&& pred) {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::copy_if(std::begin(range), std::end(range), std::move(output), std::move(pred));
	} else {
		for (const auto& elem : range) {
			if (pred(elem)) {
				*output++ = elem;
			}
		}
		return output;
	}
}

// Move a range into an output iterator.
template <typename Range, typename OutputIterator, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto move(Range&& range, OutputIterator output) {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::move(std::begin(range), std::end(range), std::move(output));
	} else {
		for (auto&& elem : range) {
			*output++ = std::move(elem);
		}
		return output;
	}
}

// Move all elements in a range that match a predicate into an output iterator.
template <typename Range, typename OutputIterator, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto moveIf(Range&& range, OutputIterator output, Predicate&& pred) {
	for (auto&& elem : range) {
		if (const auto& cElem = elem; pred(cElem)) {
			*output++ = std::move(elem);
		}
	}
	return output;
}

// Fill a range with a value.
template <typename Range, typename T, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto fill(Range&& range, const T& value) -> void {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		std::fill(std::begin(range), std::end(range), value);
	} else {
		for (auto& elem : range) {
			elem = value;
		}
	}
}

// Fill an output iterator with N values.
template <typename OutputIterator, typename Size, typename T>
constexpr auto fillN(OutputIterator output, Size n, const T& value) {
	return std::fill_n(std::move(output), std::move(n), value);
}

namespace detail {

template <typename Container>
struct CollectProxy {};

} // namespace detail

// Collect a range into a given container.
template <typename Container, typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto collect(Range&& range) {
	auto container = Container{};

	auto&& it = std::begin(range);
	auto&& end = std::end(range);

	using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;
	if constexpr (std::is_convertible_v<IteratorCategory, std::random_access_iterator_tag> && detail::has_reserve_v<Container>) {
		container.reserve(std::distance(it, end));
	}
	for (; it != end; ++it) {
		container.emplace_back(std::forward<decltype(*it)>(*it));
	}
	return container;
}

template <typename Container>
[[nodiscard]] constexpr auto collect() {
	return detail::CollectProxy<Container>{};
}

} // namespace util

template <typename Container, typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto operator|(Range&& lhs, util::detail::CollectProxy<Container>&&) {
	return util::collect<Container>(std::forward<Range>(lhs));
}

namespace util {
namespace detail {

struct SortProxy final {};

template <typename Compare>
struct SortProxyWithComparator final {
	Compare comp;
};

struct StableSortProxy final {};

template <typename Compare>
struct StableSortProxyWithComparator final {
	Compare comp;
};

} // namespace detail

// Sort a container and return the result.
[[nodiscard]] constexpr auto sort() {
	return detail::SortProxy{};
}

// Sort a container using a certain comparator and return the result.
template <typename Compare>
[[nodiscard]] constexpr auto sort(Compare&& comp) {
	return detail::SortProxyWithComparator<Compare>{std::forward<Compare>(comp)};
}

// Stable sort a container and return the result.
[[nodiscard]] constexpr auto stableSort() {
	return detail::StableSortProxy{};
}

// Stable sort a container using a certain comparator and return the result.
template <typename Compare>
[[nodiscard]] constexpr auto stableSort(Compare&& comp) {
	return detail::StableSortProxyWithComparator<Compare>{std::forward<Compare>(comp)};
}

} // namespace util

template <typename Container>
[[nodiscard]] constexpr auto operator|(Container lhs, util::detail::SortProxy&&) {
	std::sort(std::begin(lhs), std::end(lhs));
	return lhs;
}

template <typename Container, typename Compare>
[[nodiscard]] constexpr auto operator|(Container lhs, util::detail::SortProxyWithComparator<Compare>&& rhs) {
	std::sort(std::begin(lhs), std::end(lhs), std::forward<Compare>(rhs.comp));
	return lhs;
}

template <typename Container>
[[nodiscard]] constexpr auto operator|(Container lhs, util::detail::StableSortProxy&&) {
	std::stable_sort(std::begin(lhs), std::end(lhs));
	return lhs;
}

template <typename Container, typename Compare>
[[nodiscard]] constexpr auto operator|(Container lhs, util::detail::StableSortProxyWithComparator<Compare>&& rhs) {
	std::stable_sort(std::begin(lhs), std::end(lhs), std::forward<Compare>(rhs.comp));
	return lhs;
}

namespace util {

// Append a range onto a given container.
template <typename Container, typename Range, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
constexpr auto append(Container& container, Range&& range) -> void {
	using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;

	auto&& it = std::begin(range);
	auto&& end = std::end(range);
	if constexpr (std::is_convertible_v<IteratorCategory, std::random_access_iterator_tag> && detail::has_reserve_v<Container>) {
		container.reserve(container.size() + std::distance(it, end));
	}
	for (; it != end; ++it) {
		container.emplace_back(std::forward<decltype(*it)>(*it));
	}
}

// Find the first element in a range that is equal to a given value.
template <typename Range, typename T, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto find(Range&& range, const T& value) {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::find(std::begin(range), std::end(range), value);
	} else {
		auto&& it = std::begin(range);
		auto&& end = std::end(range);
		for (; it != end; ++it) {
			if (*it == value) {
				return it;
			}
		}
		return end;
	}
}

// Find the first element in a range that satisfies a given predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto findIf(Range&& range, Predicate&& pred) {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::find_if(std::begin(range), std::end(range), std::move(pred));
	} else {
		auto&& it = std::begin(range);
		auto&& end = std::end(range);
		for (; it != end; ++it) {
			if (pred(*it)) {
				return it;
			}
		}
		return end;
	}
}

// Return true if every element in a range satisfies a predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto allOf(const Range& range, Predicate&& pred) -> bool {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::all_of(std::begin(range), std::end(range), std::move(pred));
	} else {
		for (const auto& elem : range) { // NOLINT(readability-use-anyofallof)
			if (!pred(elem)) {
				return false;
			}
		}
		return true;
	}
}

// Return true if no element in a range satisfies a predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto noneOf(const Range& range, Predicate&& pred) -> bool {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::none_of(std::begin(range), std::end(range), std::move(pred));
	} else {
		for (const auto& elem : range) {
			if (pred(elem)) {
				return false;
			}
		}
		return true;
	}
}

// Return true if any element in a range satisfies a predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto anyOf(const Range& range, Predicate&& pred) -> bool {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::any_of(std::begin(range), std::end(range), std::move(pred));
	} else {
		for (const auto& elem : range) { // NOLINT(readability-use-anyofallof)
			if (pred(elem)) {
				return true;
			}
		}
		return false;
	}
}

// Return true if a range contains an element.
template <typename Range, typename T, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto contains(const Range& range, const T& value) -> bool {
	return util::anyOf(range, [&](const auto& elem) { return elem == value; });
}

// Return the number of elements in a range that are equal to a value.
template <typename Range, typename T, typename Iterator = decltype(std::begin(std::declval<Range>())), typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto count(const Range& range, const T& value) -> std::size_t {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::count(std::begin(range), std::end(range), value);
	} else {
		auto total = std::size_t{0};
		for (const auto& elem : range) {
			if (elem == value) {
				++total;
			}
		}
		return total;
	}
}

// Return the number of elements in a range that match a predicate.
template <typename Range, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Range>())),
          typename Sentinel = decltype(std::end(std::declval<Range>()))>
[[nodiscard]] constexpr auto countIf(const Range& range, Predicate&& pred) -> std::size_t {
	if constexpr (std::is_convertible_v<Sentinel, Iterator>) {
		return std::count_if(std::begin(range), std::end(range), std::move(pred));
	} else {
		auto total = std::size_t{0};
		for (const auto& elem : range) {
			if (pred(elem)) {
				++total;
			}
		}
		return total;
	}
}

// Find the element that has the best value given a unary value getter and a binary comparison function.
// Returns a pair containing an iterator to the element as well as its value.
// If the range was empty, returns the first iterator with a default-constructed value.
template <typename Range, typename ValueFunction, typename ComparisonFunction>
[[nodiscard]] constexpr auto findBestValue(Range&& range, ValueFunction&& getValue, ComparisonFunction&& compare) {
	auto&& it = std::begin(range);
	auto&& end = std::end(range);

	if (it == end) {
		return std::make_pair(std::move(it), decltype(getValue(*it)){});
	}

	auto best = it;
	auto bestValue = getValue(*it);
	for (++it; it != end; ++it) {
		if (auto value = getValue(*it); compare(value, bestValue)) {
			best = it;
			bestValue = std::move(value);
		}
	}
	return std::make_pair(std::move(best), std::move(bestValue));
}

// Find the element in a range that is closest to a certain position, given a function that gets the position of each element.
template <typename Range, typename Vector, typename PositionFunction>
[[nodiscard]] constexpr auto findClosestDistanceSquared(Range&& range, const Vector& position, PositionFunction&& getPosition) {
	return util::findBestValue(
		std::forward<Range>(range),
		[&](const auto& elem) { return Vector::distanceSquared(getPosition(elem), position); },
		std::less{});
}

template <typename Range, typename Vector>
[[nodiscard]] constexpr auto findClosestDistanceSquared(Range&& range, const Vector& position) {
	return util::findClosestDistanceSquared(std::forward<Range>(range), position, [](const Vector& position) { return position; });
}

// Find the element in a range that is furthest from a certain position, given a function that gets the position of each element.
template <typename Range, typename Vector, typename PositionFunction>
[[nodiscard]] constexpr auto findFurthestDistanceSquared(Range&& range, const Vector& position, PositionFunction&& getPosition) {
	return util::findBestValue(
		std::forward<Range>(range),
		[&](const auto& elem) { return Vector::distanceSquared(getPosition(elem), position); },
		std::greater{});
}

template <typename Range, typename Vector>
[[nodiscard]] constexpr auto findFurthestDistanceSquared(Range&& range, const Vector& position) {
	return util::findFurthestDistanceSquared(std::forward<Range>(range), position, [](const Vector& position) { return position; });
}

namespace detail {

template <typename Container, typename T, typename = int>
struct has_erase : std::false_type {};

template <typename Container, typename T>
struct has_erase<Container, T, decltype(std::declval<Container>().erase(std::declval<T>()), 0)> : std::true_type {};

template <typename Container, typename T>
inline constexpr auto has_erase_v = has_erase<Container, T>::value;

} // namespace detail

// Erase all of a certain value from a container.
template <typename Container, typename T, typename Iterator = decltype(std::begin(std::declval<Container>())),
          typename Sentinel = decltype(std::end(std::declval<Container>()))>
inline auto erase(Container& container, const T& value) -> void {
	using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;
	if constexpr (detail::has_erase_v<Container, T>) {
		container.erase(value);
	} else if constexpr (std::is_convertible_v<IteratorCategory, std::random_access_iterator_tag>) {
		auto&& end = std::end(container);
		container.erase(std::remove(std::begin(container), end, value), end);
	} else {
		auto&& it = std::begin(container);
		while (it != std::end(container)) {
			if (*it == value) {
				it = container.erase(it);
			} else {
				++it;
			}
		}
	}
}

// Erase all values from a container that match a predicate.
template <typename Container, typename Predicate, typename Iterator = decltype(std::begin(std::declval<Container>())),
          typename Sentinel = decltype(std::end(std::declval<Container>()))>
inline auto eraseIf(Container& container, Predicate&& pred) -> void {
	using IteratorCategory = typename std::iterator_traits<Iterator>::iterator_category;
	if constexpr (std::is_convertible_v<IteratorCategory, std::random_access_iterator_tag>) {
		auto&& end = std::end(container);
		container.erase(std::remove_if(std::begin(container), end, std::forward<Predicate>(pred)), end);
	} else {
		auto&& it = std::begin(container);
		while (it != std::end(container)) {
			if (pred(*it)) {
				it = container.erase(it);
			} else {
				++it;
			}
		}
	}
}

template <typename Range, typename T>
constexpr auto replace(Range& range, const T& oldValue, const T& newValue) -> void {
	for (auto& value : range) {
		if (value == oldValue) {
			value = newValue;
		}
	}
}

template <typename Range, typename Predicate, typename T>
constexpr auto replaceIf(Range& range, Predicate&& pred, const T& newValue) -> void {
	for (auto& value : range) {
		if (pred(value)) {
			value = newValue;
		}
	}
}

} // namespace util

#endif
