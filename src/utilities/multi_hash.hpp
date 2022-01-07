#ifndef AF2_UTILITIES_MULTI_HASH_HPP
#define AF2_UTILITIES_MULTI_HASH_HPP

#include "algorithm.hpp" // util::anyOf
#include "reference.hpp" // util::Reference
#include "span.hpp"      // util::Span

#include <algorithm>  // std::max, std::lexicographical_compare, std::equal
#include <cassert>    // assert
#include <cmath>      // std::ceil
#include <cstddef>    // std::ptrdiff_t, std::size_t, std::byte
#include <functional> // std::hash
#include <iterator>   // std::reverse_iterator, std::make_reverse_iterator, std::prev, std::random_access_iterator_tag
#include <limits>     // std::numeric_limits
#include <memory>     // std::addressof, std::destroy_at, std::destroy, std::destroy_n, std::unique_ptr
#include <new>        // std::launder
#include <stdexcept>  // std::out_of_range
#include <tuple>      // std::tuple, std::tuple_size, std::tuple_element, std::tuple_element_t, std::get
#include <type_traits> // std::invoke_result_t, std::remove_..._t, std::conditional_t, std::integral_contastant, std::is_..._v, std::enable_if_t
#include <utility>     // std::pair, std::forward, std::move, std::swap
#include <vector>      // std::vector

namespace util {

template <typename T, typename... Keys>
class MultiHash;

namespace detail {

inline constexpr auto MULTI_HASH_MIN_CAPACITY = std::size_t{5};

template <typename T>
class StorageArray final {
public:
	using value_type = T;
	using reference = value_type&;
	using const_reference = const value_type&;
	using size_type = std::size_t;
	using pointer = value_type*;
	using const_pointer = const value_type*;

private:
	struct AlignedStorage final {
		alignas(value_type) std::array<std::byte, sizeof(value_type)> data;
	};

public:
	using iterator = pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_iterator = const_pointer;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using difference_type = std::ptrdiff_t;

	StorageArray() noexcept = default;

	~StorageArray() {
		this->clear();
	}

	StorageArray(const StorageArray& other) {
		*this = other;
	}

	StorageArray(StorageArray&& other) noexcept {
		*this = std::move(other);
	}

	auto operator=(const StorageArray& other) -> StorageArray& {
		if (&other == this) {
			return *this;
		}

		this->clear();
		this->reserve(other.capacity());
		for (const auto& value : other) {
			this->push_back(value);
		}

		return *this;
	}

	auto operator=(StorageArray&& other) noexcept -> StorageArray& {
		if (&other == this) {
			return *this;
		}

		this->clear();
		m_storage = std::move(other.m_storage);
		m_size = other.m_size;
		m_capacity = other.m_capacity;
		return *this;
	}

	[[nodiscard]] friend auto operator==(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	[[nodiscard]] friend auto operator!=(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend auto operator<(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	[[nodiscard]] friend auto operator<=(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend auto operator>(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend auto operator>=(const StorageArray& lhs, const StorageArray& rhs) -> bool {
		return !(lhs < rhs);
	}

	[[nodiscard]] auto max_size() const noexcept -> size_type {
		return std::numeric_limits<size_type>::max();
	}

	[[nodiscard]] auto capacity() const noexcept -> size_type {
		return m_capacity;
	}

	[[nodiscard]] auto size() const noexcept -> size_type {
		return m_size;
	}

	[[nodiscard]] auto empty() const noexcept -> bool {
		return m_size == 0;
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return iterator{this->data()};
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return const_iterator{this->data()};
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return iterator{this->data() + this->size()};
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return const_iterator{this->data() + this->size()};
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return this->begin();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return this->end();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->end());
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->end());
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->begin());
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->begin());
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->cend());
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		return std::make_reverse_iterator(this->cbegin());
	}

	[[nodiscard]] auto data() noexcept -> pointer {
		return (m_storage) ? std::launder(reinterpret_cast<pointer>(m_storage[0].data.data())) : nullptr;
	}

	[[nodiscard]] auto data() const noexcept -> const_pointer {
		return (m_storage) ? std::launder(reinterpret_cast<const_pointer>(m_storage[0].data.data())) : nullptr;
	}

	[[nodiscard]] auto front() -> reference {
		assert(!this->empty());
		return *this->data();
	}

	[[nodiscard]] auto front() const -> const_reference {
		assert(!this->empty());
		return *this->data();
	}

	[[nodiscard]] auto back() -> reference {
		assert(!this->empty());
		return (*this)[this->size() - 1];
	}

	[[nodiscard]] auto back() const -> const_reference {
		assert(!this->empty());
		return (*this)[this->size() - 1];
	}

	[[nodiscard]] auto operator[](size_type i) -> reference {
		assert(i < this->size());
		return *(this->data() + i);
	}

	[[nodiscard]] auto operator[](size_type i) const -> const_reference {
		assert(i < this->size());
		return *(this->data() + i);
	}

	[[nodiscard]] auto at(size_type i) -> reference {
		if (i < m_size) {
			return (*this)[i];
		}
		throw std::out_of_range{"MultiHash::at() index out of range."};
	}

	[[nodiscard]] auto at(size_type i) const -> const_reference {
		if (i < m_size) {
			return (*this)[i];
		}
		throw std::out_of_range{"MultiHash::at() index out of range."};
	}

	auto clear() noexcept -> void {
		std::destroy_n(this->data(), this->size());
		m_storage.reset();
		m_size = 0;
		m_capacity = 0;
	}

	auto shrink_to_fit() -> void {
		this->reserve(this->size());
	}

	auto reserve(size_type newCapacity) -> void {
		newCapacity = std::max(newCapacity, this->size());
		if (newCapacity != m_capacity) {
			if (newCapacity == 0) {
				this->clear();
			} else {
				auto* const end = this->data() + this->size();

				auto newStorage = std::make_unique<AlignedStorage[]>(newCapacity);

				auto* const newBegin = std::launder(reinterpret_cast<pointer>(newStorage[0].data.data()));

				auto* newPtr = newBegin;
				for (auto* ptr = this->data(); ptr != end; ++ptr) {
					if constexpr (std::is_nothrow_move_constructible_v<value_type>) {
						new (newPtr) value_type(std::move(*ptr));
					} else {
						try {
							new (newPtr) value_type(std::move(*ptr));
						} catch (...) {
							std::destroy(newBegin, newPtr);
							throw;
						}
					}
					++newPtr;
				}
				std::destroy(this->data(), end);
				m_storage = std::move(newStorage);
				m_capacity = newCapacity;
			}
		}
	}

	template <typename M>
	auto push_back(M&& obj) -> void {
		this->emplace_back(std::forward<M>(obj));
	}

	template <typename... Args>
	auto emplace_back(Args&&... args) -> reference {
		if (this->size() == this->capacity()) {
			this->reserve(std::max(this->capacity() * 2, util::detail::MULTI_HASH_MIN_CAPACITY));
		}

		auto* const ptr = this->data() + this->size();
		new (ptr) value_type(std::forward<Args>(args)...);
		++m_size;
		return *ptr;
	}

	auto pop_back() noexcept -> void {
		assert(!this->empty());
		--m_size;
		std::destroy_at(this->data() + m_size);
	}

	auto erase(const_iterator pos) -> iterator {
		assert(!this->empty());
		const auto i = pos - this->cbegin();
		const auto end = this->end();

		auto it = this->begin() + i;
		for (auto next = std::next(it); next != end; ++next) {
			*it++ = std::move(*next);
		}
		this->pop_back();
		return this->begin() + i;
	}

	auto erase(const_iterator first, const_iterator last) -> iterator {
		assert(this->size() >= last - first);
		const auto i = first - this->cbegin();
		const auto n = last - first;
		const auto end = this->end();

		auto it = this->begin() + i;
		for (auto next = std::next(it, n); next != end; ++next) {
			*it++ = std::move(*next);
		}
		while (this->end() != it) {
			this->pop_back();
		}
		return this->begin() + i;
	}

private:
	std::unique_ptr<AlignedStorage[]> m_storage = nullptr;
	size_type m_size = 0;
	size_type m_capacity = 0;
};

template <typename T, typename... Keys>
class MultiHashElement final {
public:
	MultiHashElement() = default;

	template <typename Arg, typename... Args, typename = std::enable_if_t<sizeof...(Args) != 0 || !std::is_same_v<std::remove_reference_t<Arg>, MultiHashElement>>>
	MultiHashElement(Arg&& arg, Args&&... args)
		: m_tuple(std::forward<Arg>(arg), std::forward<Args>(args)...) {}

	[[nodiscard]] constexpr auto operator*() noexcept -> T& {
		return this->get<0>();
	}

	[[nodiscard]] constexpr auto operator*() const noexcept -> const T& {
		return this->get<0>();
	}

	[[nodiscard]] constexpr auto operator->() noexcept -> T* {
		return std::addressof(**this);
	}

	[[nodiscard]] constexpr auto operator->() const noexcept -> const T* {
		return std::addressof(**this);
	}

	template <std::size_t INDEX>
	[[nodiscard]] auto get() -> decltype(auto) {
		if constexpr (INDEX == 0) {
			return std::get<INDEX>(m_tuple);
		} else {
			return std::get<INDEX>(static_cast<const decltype(m_tuple)&>(m_tuple));
		}
	}

	template <std::size_t INDEX>
	[[nodiscard]] auto get() const -> decltype(auto) {
		if constexpr (INDEX == 0) {
			return std::get<INDEX>(m_tuple);
		} else {
			return std::get<INDEX>(static_cast<const decltype(m_tuple)&>(m_tuple));
		}
	}

	template <std::size_t INDEX>
	[[nodiscard]] friend auto get(MultiHashElement& elem) -> decltype(auto) {
		return elem.get<INDEX>();
	}

	template <std::size_t INDEX>
	[[nodiscard]] friend auto get(const MultiHashElement& elem) -> decltype(auto) {
		return elem.get<INDEX>();
	}

private:
	friend util::MultiHash<T, Keys...>;

	std::tuple<T, Keys...> m_tuple{};
};

} // namespace detail
} // namespace util

template <typename T, typename... Keys>
struct std::tuple_size<util::detail::MultiHashElement<T, Keys...>> : std::tuple_size<std::tuple<T, Keys...>> {};

template <std::size_t INDEX, typename T, typename... Keys>
struct std::tuple_element<INDEX, util::detail::MultiHashElement<T, Keys...>>
	: std::conditional_t<INDEX == 0, std::tuple_element<INDEX, std::tuple<T, Keys...>>, std::tuple_element<INDEX, const std::tuple<T, Keys...>>> {};

namespace util {

template <typename T, typename... Keys>
class MultiHash final {
public:
	using size_type = std::size_t;
	using value_type = detail::MultiHashElement<T, Keys...>;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

private:
	using Array = detail::StorageArray<value_type>;
	using Bucket = std::vector<std::ptrdiff_t>;
	using Buckets = std::vector<Bucket>;

	template <bool CONST_VALUE>
	class MapIterator final {
	private:
		using Values = std::conditional_t<CONST_VALUE, util::Span<const MultiHash::value_type>, util::Span<MultiHash::value_type>>;
		using BucketIterator = std::conditional_t<CONST_VALUE, typename Bucket::const_iterator, typename Bucket::iterator>;
		friend MapIterator<true>;

	public:
		using difference_type = typename BucketIterator::difference_type;
		using value_type = std::conditional_t<CONST_VALUE, const typename MultiHash::value_type, typename MultiHash::value_type>;
		using reference = std::conditional_t<CONST_VALUE, typename MultiHash::const_reference, typename MultiHash::reference>;
		using pointer = std::conditional_t<CONST_VALUE, typename MultiHash::const_pointer, typename MultiHash::pointer>;
		using iterator_category = typename BucketIterator::iterator_category;

		constexpr MapIterator() noexcept = default;

		constexpr MapIterator(Values values, BucketIterator it) noexcept
			: m_values(values)
			, m_it(it) {}

		~MapIterator() = default;

		constexpr MapIterator(const MapIterator&) noexcept = default;
		constexpr MapIterator(MapIterator&&) noexcept = default;

		constexpr auto operator=(const MapIterator&) noexcept -> MapIterator& = default;
		constexpr auto operator=(MapIterator&&) noexcept -> MapIterator& = default;

		template <bool OTHER_CONST, typename = std::enable_if_t<!OTHER_CONST || CONST_VALUE>>
		constexpr MapIterator(const MapIterator<OTHER_CONST>& other) noexcept
			: m_values(other.m_values)
			, m_it(other.m_it) {}

		[[nodiscard]] constexpr auto operator==(const MapIterator& other) const noexcept -> bool {
			return m_it == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const MapIterator& other) const noexcept -> bool {
			return m_it != other.m_it;
		}

		[[nodiscard]] constexpr auto operator<(const MapIterator& other) const noexcept -> bool {
			return m_it < other.m_it;
		}

		[[nodiscard]] constexpr auto operator>(const MapIterator& other) const noexcept -> bool {
			return m_it > other.m_it;
		}

		[[nodiscard]] constexpr auto operator<=(const MapIterator& other) const noexcept -> bool {
			return m_it <= other.m_it;
		}

		[[nodiscard]] constexpr auto operator>=(const MapIterator& other) const noexcept -> bool {
			return m_it >= other.m_it;
		}

		[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
			return m_values[*m_it];
		}

		[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
			return std::addressof(**this);
		}

		constexpr auto operator++(int) noexcept -> MapIterator {
			auto old = *this;
			++*this;
			return old;
		}

		constexpr auto operator--(int) noexcept -> MapIterator {
			auto old = *this;
			--*this;
			return old;
		}

		constexpr auto operator++() noexcept -> MapIterator& {
			++m_it;
			return *this;
		}

		constexpr auto operator--() noexcept -> MapIterator& {
			--m_it;
			return *this;
		}

		constexpr auto operator+=(difference_type n) noexcept -> MapIterator& {
			m_it += n;
			return *this;
		}

		constexpr auto operator-=(difference_type n) noexcept -> MapIterator& {
			m_it -= n;
			return *this;
		}

		[[nodiscard]] constexpr auto operator[](difference_type n) noexcept -> reference {
			return *m_values[m_it[n]].get();
		}

		[[nodiscard]] constexpr friend auto operator+(const MapIterator& lhs, difference_type rhs) noexcept -> MapIterator {
			return MapIterator{lhs.m_values, lhs.m_it + rhs};
		}

		[[nodiscard]] constexpr friend auto operator+(difference_type lhs, const MapIterator& rhs) noexcept -> MapIterator {
			return MapIterator{rhs.m_values, lhs + rhs.m_it};
		}

		[[nodiscard]] constexpr friend auto operator-(const MapIterator& lhs, difference_type rhs) noexcept -> MapIterator {
			return MapIterator{lhs.m_values, lhs.m_it - rhs};
		}

		[[nodiscard]] constexpr friend auto operator-(const MapIterator& lhs, const MapIterator& rhs) noexcept -> difference_type {
			lhs.m_it - rhs.m_it;
		}

	private:
		Values m_values{};
		BucketIterator m_it{};
	};

public:
	using iterator = typename Array::iterator;
	using reverse_iterator = typename Array::reverse_iterator;
	using const_iterator = typename Array::const_iterator;
	using const_reverse_iterator = typename Array::const_reverse_iterator;
	using difference_type = typename Array::difference_type;

	using map_iterator = MapIterator<false>;
	using reverse_map_iterator = std::reverse_iterator<map_iterator>;
	using const_map_iterator = MapIterator<true>;
	using const_reverse_map_iterator = std::reverse_iterator<const_map_iterator>;
	using map_difference_type = typename map_iterator::difference_type;

	[[nodiscard]] friend auto operator==(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr == rhs.m_arr;
	}

	[[nodiscard]] friend auto operator!=(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr != rhs.m_arr;
	}

	[[nodiscard]] friend auto operator<(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr < rhs.m_arr;
	}

	[[nodiscard]] friend auto operator<=(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr <= rhs.m_arr;
	}

	[[nodiscard]] friend auto operator>(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr > rhs.m_arr;
	}

	[[nodiscard]] friend auto operator>=(const MultiHash& lhs, const MultiHash& rhs) -> bool {
		return lhs.m_arr >= rhs.m_arr;
	}

	[[nodiscard]] auto max_size() const noexcept -> size_type {
		return m_arr.max_size();
	}

	[[nodiscard]] auto capacity() const noexcept -> size_type {
		return m_arr.capacity();
	}

	[[nodiscard]] auto size() const noexcept -> size_type {
		return m_arr.size();
	}

	[[nodiscard]] auto empty() const noexcept -> bool {
		return m_arr.empty();
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return m_arr.begin();
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return m_arr.begin();
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return m_arr.end();
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return m_arr.end();
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return m_arr.cbegin();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return m_arr.cend();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		return m_arr.rbegin();
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		return m_arr.rbegin();
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		return m_arr.rend();
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		return m_arr.rend();
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		return m_arr.crbegin();
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		return m_arr.crend();
	}

	[[nodiscard]] auto front() -> reference {
		return m_arr.front();
	}

	[[nodiscard]] auto front() const -> const_reference {
		return m_arr.front();
	}

	[[nodiscard]] auto back() -> reference {
		return m_arr.back();
	}

	[[nodiscard]] auto back() const -> const_reference {
		return m_arr.back();
	}

	[[nodiscard]] auto operator[](size_type i) -> reference {
		return m_arr[i];
	}

	[[nodiscard]] auto operator[](size_type i) const -> const_reference {
		return m_arr[i];
	}

	[[nodiscard]] auto at(size_type i) -> reference {
		return m_arr.at(i);
	}

	[[nodiscard]] auto at(size_type i) const -> const_reference {
		return m_arr.at(i);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto find(const Key& key) -> iterator {
		return m_maps.template find<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto find(const Key& key) const -> const_iterator {
		return m_maps.template find<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto equal_range(const Key& key) -> std::pair<map_iterator, map_iterator> {
		return m_maps.template equal_range<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto equal_range(const Key& key) const -> std::pair<const_map_iterator, const_map_iterator> {
		return m_maps.template equal_range<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto count(const Key& key) const -> size_type {
		return m_maps.template count<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Key>
	[[nodiscard]] auto contains(const Key& key) const -> bool {
		return m_maps.template contains<INDEX>(*this, key);
	}

	template <std::size_t INDEX, typename Value>
	auto set(iterator pos, Value&& value) -> void {
		if constexpr (INDEX != 0) {
			m_maps.template set<INDEX>(*this, pos, value);
		}
		std::get<INDEX>(pos->m_tuple) = std::forward<Value>(value);
	}

	template <std::size_t INDEX, typename Key>
	auto erase(const Key& key) -> size_type {
		auto count = size_type{0};
		for (auto it = this->find<INDEX>(key); it != this->end(); it = this->find<INDEX>(key)) {
			this->erase(it);
			++count;
		}
		return count;
	}

	auto swap(MultiHash& other) -> void {
		auto temp = other;
		other = std::move(*this);
		*this = std::move(temp);
	}

	auto clear() noexcept -> void {
		m_maps.clear();
		m_arr.clear();
	}

	auto shrink_to_fit() -> void {
		this->reserve(this->size());
	}

	auto reserve(size_type newCapacity) -> void {
		newCapacity = std::max(newCapacity, this->size());
		if (newCapacity != m_arr.capacity()) {
			m_arr.reserve(newCapacity);
			m_maps.rehash(*this, std::max(newCapacity, detail::MULTI_HASH_MIN_CAPACITY));
		}
	}

	template <typename M>
	auto push_back(M&& obj) -> void {
		this->emplace_back(std::forward<M>(obj));
	}

	template <typename... Args>
	auto emplace_back(Args&&... args) -> reference {
		const auto i = static_cast<std::ptrdiff_t>(this->size());
		if (this->size() == this->capacity()) {
			this->reserve(std::max(this->capacity() * 2, detail::MULTI_HASH_MIN_CAPACITY));
		}

		auto& ref = m_arr.emplace_back(std::forward<Args>(args)...);
		try {
			m_maps.insert(*this, i);
		} catch (...) {
			m_arr.pop_back();
			throw;
		}
		return ref;
	}

	auto pop_back() -> void {
		assert(!this->empty());
		m_maps.pop_back(*this);
		m_arr.pop_back();
	}

	auto erase(const_iterator pos) -> iterator {
		m_maps.erase(*this, pos);
		return m_arr.erase(pos);
	}

	auto erase(const_iterator first, const_iterator last) -> iterator {
		m_maps.erase(*this, first, last);
		return m_arr.erase(first, last);
	}

private:
	template <std::size_t INDEX, typename Key>
	struct Map {
	private:
		[[nodiscard]] static auto bucket(Buckets& buckets, std::size_t hash) -> Bucket& {
			return buckets[hash % buckets.size()];
		}

		[[nodiscard]] static auto bucket(const Buckets& buckets, std::size_t hash) -> const Bucket& {
			return buckets[hash % buckets.size()];
		}

		auto eraseIndex(Bucket& bucket, std::ptrdiff_t i) -> void {
			const auto end = bucket.end();
			for (auto it = bucket.begin(); it != end; ++it) {
				if (*it == i) {
					bucket.erase(it);
					break;
				}
			}
		}

		template <typename DesiredKey>
		[[nodiscard]] auto upperBound(const MultiHash& container, const Bucket& bucket, const DesiredKey& key) const ->
			typename Bucket::const_iterator {
			const auto end = bucket.end();
			for (auto it = bucket.begin(); it != end; ++it) {
				if (container[*it].template get<INDEX>() == key) {
					for (++it; it != end; ++it) {
						if (!(container[*it].template get<INDEX>() == key)) {
							break;
						}
					}
					return it;
				}
			}
			return end;
		}

	public:
		auto setBuckets(Buckets&& buckets) -> void {
			m_buckets = std::move(buckets);
		}

		template <typename DesiredKey>
		[[nodiscard]] auto find(MultiHash& container, const DesiredKey& key) -> iterator {
			for (const auto& i : Map::bucket(m_buckets, m_hasher(key))) {
				if (container[i].template get<INDEX>() == key) {
					return container.begin() + i;
				}
			}
			return container.end();
		}

		template <typename DesiredKey>
		[[nodiscard]] auto find(const MultiHash& container, const DesiredKey& key) const -> const_iterator {
			for (const auto& i : Map::bucket(m_buckets, m_hasher(key))) {
				if (container[i].template get<INDEX>() == key) {
					return container.begin() + i;
				}
			}
			return container.end();
		}

		template <typename DesiredKey>
		[[nodiscard]] auto equal_range(MultiHash& container, const DesiredKey& key) -> std::pair<map_iterator, map_iterator> {
			auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			const auto end = bucket.end();
			for (auto it = bucket.begin(); it != end; ++it) {
				if (container[*it].template get<INDEX>() == key) {
					const auto begin = it;
					for (++it; it != end; ++it) {
						if (!(container[*it].template get<INDEX>() == key)) {
							break;
						}
					}
					return {map_iterator{container.storage(), begin}, map_iterator{container.storage(), it}};
				}
			}
			return {map_iterator{container.storage(), end}, map_iterator{container.storage(), end}};
		}

		template <typename DesiredKey>
		[[nodiscard]] auto equal_range(const MultiHash& container, const DesiredKey& key) const
			-> std::pair<const_map_iterator, const_map_iterator> {
			const auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			const auto end = bucket.end();
			for (auto it = bucket.begin(); it != end; ++it) {
				if (container[*it].template get<INDEX>() == key) {
					const auto begin = it;
					for (++it; it != end; ++it) {
						if (!(container[*it].template get<INDEX>() == key)) {
							break;
						}
					}
					return {const_map_iterator{container.storage(), begin}, const_map_iterator{container.storage(), it}};
				}
			}
			return {const_map_iterator{container.storage(), end}, const_map_iterator{container.storage(), end}};
		}

		template <typename DesiredKey>
		[[nodiscard]] auto count(const MultiHash& container, const DesiredKey& key) const -> size_type {
			const auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			const auto end = bucket.end();
			for (auto it = bucket.begin(); it != end; ++it) {
				if (container[*it].template get<INDEX>() == key) {
					auto count = size_type{1};
					for (++it; it != end; ++it) {
						if (!(container[*it].template get<INDEX>() == key)) {
							break;
						}
						++count;
					}
					return count;
				}
			}
			return size_type{0};
		}

		template <typename DesiredKey>
		[[nodiscard]] auto contains(const MultiHash& container, const DesiredKey& key) const -> bool {
			const auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			return util::anyOf(bucket, [&](const auto& i) { return container[i].template get<INDEX>() == key; });
		}

		template <typename DesiredKey>
		auto set(const MultiHash& container, const_iterator pos, const DesiredKey& key) -> void {
			const auto i = pos - container.begin();
			auto& oldBucket = Map::bucket(m_buckets, m_hasher(pos->template get<INDEX>()));
			this->eraseIndex(oldBucket, i);
			auto& newBucket = Map::bucket(m_buckets, m_hasher(key));
			newBucket.insert(this->upperBound(container, newBucket, key), i);
		}

		auto swap(Map& other) -> void {
			using std::swap;
			m_buckets.swap(other.m_buckets);
			swap(m_hasher, other.m_hasher);
		}

		auto clear() noexcept -> void {
			m_buckets.clear();
		}

		auto rehash(const MultiHash& container, std::size_t newBucketCount) -> Buckets {
			auto newBuckets = Buckets(newBucketCount);
			for (const auto& value : container) {
				const auto& key = value.template get<INDEX>();
				const auto hash = m_hasher(key);
				auto& bucket = Map::bucket(newBuckets, hash);
				for (const auto& i : Map::bucket(m_buckets, hash)) {
					if (container[i].template get<INDEX>() == key) {
						bucket.push_back(i);
						break;
					}
				}
			}
			auto oldBuckets = std::move(m_buckets);
			m_buckets = std::move(newBuckets);
			return oldBuckets;
		}

		auto insert(const MultiHash& container, std::ptrdiff_t i) -> void {
			const auto& key = container[i].template get<INDEX>();

			auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			bucket.insert(this->upperBound(container, bucket, key), i);
		}

		auto pop_back(const MultiHash& container) -> void {
			const auto i = static_cast<std::ptrdiff_t>(container.size() - 1);
			const auto& key = container.back().template get<INDEX>();

			auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			this->eraseIndex(bucket, i);
		}

		auto erase(const MultiHash& container, const_iterator pos) -> void {
			const auto i = pos - container.begin();
			const auto& key = pos->template get<INDEX>();

			auto& bucket = Map::bucket(m_buckets, m_hasher(key));
			this->eraseIndex(bucket, i);

			for (auto& bucket : m_buckets) {
				for (auto& index : bucket) {
					if (index > i) {
						--index;
					}
				}
			}
		}

		auto erase(const MultiHash& container, const_iterator first, const_iterator last) -> void {
			const auto n = last - first;
			while (first != last) {
				const auto i = first - container.begin();
				const auto& key = first->template get<INDEX>();

				auto& bucket = Map::bucket(m_buckets, m_hasher(key));

				const auto end = bucket.end();
				this->eraseIndex(bucket, i);
				++first;
			}

			const auto i = last - container.begin();
			for (auto& bucket : m_buckets) {
				for (auto& index : bucket) {
					if (index > i) {
						index -= n;
					}
				}
			}
		}

	private:
		Buckets m_buckets = Buckets(detail::MULTI_HASH_MIN_CAPACITY);
		std::hash<Key> m_hasher{};
	};

	template <std::size_t, typename...>
	struct Maps;

	template <std::size_t INDEX>
	struct Maps<INDEX> {
		auto swap(Maps<INDEX>&) -> void {}
		auto clear() noexcept -> void {}
		auto rehash(const MultiHash&, std::size_t) -> void {}
		auto insert(const MultiHash&, std::ptrdiff_t) -> void {}
		auto pop_back(const MultiHash&) -> void {}
		auto erase(const MultiHash&, const_iterator) -> void {}
		auto erase(const MultiHash&, const_iterator, const_iterator) -> void {}
	};

	template <std::size_t INDEX, typename Key, typename... Rest>
	struct Maps<INDEX, Key, Rest...>
		: Maps<INDEX + 1, Rest...>
		, Map<INDEX, Key> {
		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto find(MultiHash& container, const DesiredKey& key) -> iterator {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::find(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template find<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto find(const MultiHash& container, const DesiredKey& key) const -> const_iterator {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::find(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template find<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto equal_range(MultiHash& container, const DesiredKey& key) -> std::pair<map_iterator, map_iterator> {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::equal_range(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template equal_range<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto equal_range(const MultiHash& container, const DesiredKey& key) const
			-> std::pair<const_map_iterator, const_map_iterator> {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::equal_range(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template equal_range<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto count(const MultiHash& container, const DesiredKey& key) const -> size_type {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::count(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template count<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		[[nodiscard]] auto contains(const MultiHash& container, const DesiredKey& key) const -> bool {
			if constexpr (DESIRED_INDEX == INDEX) {
				return this->Map<INDEX, Key>::contains(container, key);
			} else {
				return this->Maps<INDEX + 1, Rest...>::template contains<DESIRED_INDEX>(container, key);
			}
		}

		template <std::size_t DESIRED_INDEX, typename DesiredKey>
		auto set(const MultiHash& container, const_iterator pos, const DesiredKey& key) -> void {
			if constexpr (DESIRED_INDEX == INDEX) {
				this->Map<INDEX, Key>::set(container, pos, key);
			} else {
				this->Maps<INDEX + 1, Rest...>::template set<DESIRED_INDEX>(container, pos, key);
			}
		}

		auto swap(Maps<INDEX, Key, Rest...>& other) -> void {
			this->Map<INDEX, Key>::swap(static_cast<Map<INDEX, Key>&>(other));
			this->Maps<INDEX + 1, Rest...>::swap(static_cast<Maps<INDEX + 1, Rest...>&>(other));
		}

		auto clear() noexcept -> void {
			this->Map<INDEX, Key>::clear();
			this->Maps<INDEX + 1, Rest...>::clear();
		}

		auto rehash(const MultiHash& container, std::size_t newBucketCount) -> void {
			auto oldBuckets = this->Map<INDEX, Key>::rehash(container, newBucketCount);
			try {
				this->Maps<INDEX + 1, Rest...>::rehash(container, newBucketCount);
			} catch (...) {
				this->Map<INDEX, Key>::setBuckets(std::move(oldBuckets));
				throw;
			}
		}

		auto insert(const MultiHash& container, std::ptrdiff_t i) -> void {
			this->Map<INDEX, Key>::insert(container, i);
			try {
				this->Maps<INDEX + 1, Rest...>::insert(container, i);
			} catch (...) {
				this->Map<INDEX, Key>::erase(container, container.begin() + i);
				throw;
			}
		}

		auto pop_back(const MultiHash& container) -> void {
			this->Map<INDEX, Key>::pop_back(container);
			this->Maps<INDEX + 1, Rest...>::pop_back(container);
		}

		auto erase(const MultiHash& container, const_iterator pos) -> void {
			this->Map<INDEX, Key>::erase(container, pos);
			this->Maps<INDEX + 1, Rest...>::erase(container, pos);
		}

		auto erase(const MultiHash& container, const_iterator first, const_iterator last) -> void {
			this->Map<INDEX, Key>::erase(container, first, last);
			this->Maps<INDEX + 1, Rest...>::erase(container, first, last);
		}
	};

	Array m_arr{};
	Maps<1, Keys...> m_maps{};
};

} // namespace util

#endif
