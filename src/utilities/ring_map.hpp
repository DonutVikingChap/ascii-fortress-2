#ifndef AF2_UTILITIES_RING_MAP_HPP
#define AF2_UTILITIES_RING_MAP_HPP

#include "integer.hpp" // util::ceil2

#include <algorithm>   // std::copy, std::min, std::move
#include <cassert>     // assert
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <iterator>    // std::forward_iterator_tag, std::next
#include <limits>      // std::numeric_limits
#include <memory>      // std::addressof, std::unique_ptr
#include <optional>    // std::optional
#include <stdexcept>   // std::length_error, std::out_of_range
#include <tuple>       // std::forward_as_tuple
#include <type_traits> // std::conditional_t, std::is_..._v, std::make_signed_t
#include <utility>     // std::move, std::forward, std::pair, std::piecewise_construct

namespace util {

template <typename Key, typename T, typename IndexType = std::conditional_t<std::is_unsigned_v<Key>, Key, std::size_t>>
class RingMap final {
public:
	using key_type = Key;
	using mapped_type = T;
	using size_type = IndexType;
	using value_type = std::pair<const key_type, mapped_type>;
	using reference = value_type&;
	using const_reference = const value_type&;
	using pointer = value_type*;
	using const_pointer = const value_type*;

	static_assert(std::is_convertible_v<key_type, size_type>, "Ring key type must be convertible to index type."); // Needed to get the index from a key.
	static_assert(std::is_unsigned_v<size_type>, "Ring index type must be unsigned."); // Needed for defined overflow.

private:
	template <bool CONST_VALUE>
	class Iterator final {
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = std::conditional_t<CONST_VALUE, const typename RingMap::value_type, typename RingMap::value_type>;
		using reference = value_type&;
		using pointer = value_type*;
		using iterator_category = std::forward_iterator_tag;

	private:
		friend Iterator<true>;
		friend RingMap;
		using RingPointer = std::conditional_t<CONST_VALUE, const RingMap*, RingMap*>;

		constexpr Iterator(RingPointer ring, size_type index) noexcept
			: m_ring(ring)
			, m_index(index) {
			assert(m_ring);
			assert(m_index == m_ring->m_end || m_ring->get(m_index));
		}

	public:
		constexpr Iterator() noexcept = default;

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr Iterator(const Iterator<OTHER_CONST>& other) noexcept
			: m_ring(other.m_ring)
			, m_index(other.m_index) {}

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr Iterator(Iterator<OTHER_CONST>&& other) noexcept
			: m_ring(other.m_ring)
			, m_index(other.m_index) {}

		~Iterator() = default;

		constexpr Iterator(const Iterator&) noexcept = default;
		constexpr Iterator(Iterator&&) noexcept = default;

		constexpr auto operator=(const Iterator&) noexcept -> Iterator& = default;
		constexpr auto operator=(Iterator&&) noexcept -> Iterator& = default;

		[[nodiscard]] constexpr auto operator==(const Iterator& other) const noexcept -> bool {
			assert(m_ring == other.m_ring);
			return m_index == other.m_index;
		}

		[[nodiscard]] constexpr auto operator!=(const Iterator& other) const noexcept -> bool {
			assert(m_ring == other.m_ring);
			return m_index != other.m_index;
		}

		[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
			assert(m_ring);
			assert(m_ring->get(m_index));
			return *m_ring->get(m_index);
		}

		[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
			return std::addressof(**this);
		}

		constexpr auto operator++(int) noexcept -> Iterator {
			auto old = *this;
			++*this;
			return old;
		}

		constexpr auto operator++() noexcept -> Iterator& {
			assert(m_ring);
			do {
				++m_index;
			} while (m_index != m_ring->m_end && !m_ring->get(m_index));
			return *this;
		}

	private:
		RingPointer m_ring = nullptr;
		size_type m_index = 0;
	};

public:
	using iterator = Iterator<false>;
	using const_iterator = Iterator<true>;

	RingMap() noexcept = default;
	~RingMap() = default;

	explicit RingMap(size_type capacity) {
		reserve(capacity);
	}

	RingMap(const RingMap& other) {
		*this = other;
	}

	RingMap(RingMap&& other) noexcept = default;

	auto operator=(const RingMap& other) -> RingMap& {
		if (&other == this) {
			return *this;
		}

		if (other.m_capacity == 0) {
			m_values.reset();
			m_capacity = 0;
			m_size = 0;
			m_begin = 0;
			m_end = 0;
		} else {
			if (other.m_capacity != m_capacity) {
				m_values = std::make_unique<std::optional<value_type>[]>(other.m_capacity);
				m_capacity = other.m_capacity;
			}
			std::copy(other.m_values.get(), other.m_values.get() + m_capacity, m_values.get());
			m_size = other.m_size;
			m_begin = other.m_begin;
			m_end = other.m_end;
		}
		return *this;
	}

	auto operator=(RingMap&& other) noexcept -> RingMap& = default;

	auto clear() noexcept -> void {
		while (m_begin != m_end) {
			this->get(m_begin).reset();
			++m_begin;
		}
		m_size = 0;
	}

	auto reserve(size_type newCapacity) -> void {
		if (newCapacity > m_capacity) {
			// Round capacity up to the next power of 2.
			newCapacity = util::ceil2(newCapacity);
			if (newCapacity == 0 || newCapacity > this->max_size()) {
				throw std::length_error{"Ring capacity is larger than the maximum size."};
			}

			auto newValues = std::make_unique<std::optional<value_type>[]>(newCapacity);
			for (auto i = m_begin; i != m_end; ++i) {
				const auto oldIndex = i & (m_capacity - 1);
				if (auto& oldValue = m_values[oldIndex]) {
					const auto newIndex = i & (newCapacity - 1);
					newValues[newIndex].emplace(oldValue->first, std::move(oldValue->second));
				}
			}
			m_values = std::move(newValues);
			m_capacity = newCapacity;
		}
	}

	[[nodiscard]] auto max_size() const noexcept -> size_type {
		return std::numeric_limits<size_type>::max() / 2 + 1;
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

	[[nodiscard]] auto contains(const key_type& key) const noexcept -> bool {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			const auto& value = this->get(i);
			return value && value->first == key;
		}
		return false;
	}

	[[nodiscard]] auto count(const key_type& key) const noexcept -> size_type {
		return (this->contains(key)) ? size_type{1} : size_type{0};
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return iterator{this, m_begin};
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return const_iterator{this, m_begin};
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return iterator{this, m_end};
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return const_iterator{this, m_end};
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return this->begin();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return this->end();
	}

	[[nodiscard]] auto front() noexcept -> reference {
		assert(!this->empty());
		return *this->get(this->first());
	}

	[[nodiscard]] auto front() const noexcept -> const_reference {
		assert(!this->empty());
		return *this->get(this->first());
	}

	[[nodiscard]] auto back() noexcept -> reference {
		assert(!this->empty());
		return *this->get(this->last());
	}

	[[nodiscard]] auto back() const noexcept -> const_reference {
		assert(!this->empty());
		return *this->get(this->last());
	}

	[[nodiscard]] auto find(const key_type& key) noexcept -> iterator {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			if (const auto& value = this->get(i); value && value->first == key) {
				return iterator{this, i};
			}
		}
		return this->end();
	}

	[[nodiscard]] auto find(const key_type& key) const noexcept -> const_iterator {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			if (const auto& value = this->get(i); value && value->first == key) {
				return const_iterator{this, i};
			}
		}
		return this->end();
	}

	[[nodiscard]] auto equal_range(const key_type& key) -> std::pair<iterator, iterator> {
		const auto end_it = this->end();
		if (const auto it = this->find(key); it != end_it) {
			return {it, std::next(it)};
		}
		return {end_it, end_it};
	}

	[[nodiscard]] auto equal_range(const key_type& key) const -> std::pair<const_iterator, const_iterator> {
		const auto end_it = this->end();
		if (const auto it = this->find(key); it != end_it) {
			return {it, std::next(it)};
		}
		return {end_it, end_it};
	}

	[[nodiscard]] auto at(const key_type& key) -> mapped_type& {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			if (auto& value = this->get(i); value && value->first == key) {
				return value->second;
			}
		}
		throw std::out_of_range{"Ring key not found."};
	}

	[[nodiscard]] auto at(const key_type& key) const -> const mapped_type& {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			if (const auto& value = this->get(i); value && value->first == key) {
				return value->second;
			}
		}
		throw std::out_of_range{"Ring key not found."};
	}

	[[nodiscard]] auto operator[](const key_type& key) noexcept(std::is_nothrow_copy_constructible_v<key_type>&& std::is_nothrow_default_constructible_v<mapped_type>)
		-> mapped_type& {
		return this->try_emplace(key).first->second;
	}

	[[nodiscard]] auto operator[](key_type&& key) noexcept(std::is_nothrow_move_constructible_v<key_type>&& std::is_nothrow_default_constructible_v<mapped_type>)
		-> mapped_type& {
		return this->try_emplace(std::move(key)).first->second;
	}

	template <typename... Args>
	auto try_emplace(const key_type& key,
					 Args&&... args) noexcept(std::is_nothrow_copy_constructible_v<key_type>&& std::is_nothrow_constructible_v<mapped_type, Args...>)
		-> std::pair<iterator, bool> {
		return this->try_emplace_impl(key, std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto try_emplace(key_type&& key,
					 Args&&... args) noexcept(std::is_nothrow_move_constructible_v<key_type>&& std::is_nothrow_constructible_v<mapped_type, Args...>)
		-> std::pair<iterator, bool> {
		return this->try_emplace_impl(std::move(key), std::forward<Args>(args)...);
	}

	template <typename M>
	auto insert_or_assign(const key_type& key, M&& obj) -> std::pair<iterator, bool> {
		return this->insert_or_assign_impl(key, std::forward<M>(obj));
	}

	template <typename M>
	auto insert_or_assign(key_type&& key, M&& obj) -> std::pair<iterator, bool> {
		return this->insert_or_assign_impl(std::move(key), std::forward<M>(obj));
	}

	auto pop_front() noexcept -> void {
		assert(!empty());
		--m_size;
		this->get(this->first()).reset();
		do {
			++m_begin;
		} while (m_begin != m_end && !this->get(this->first()));
	}

	auto pop_back() noexcept -> void {
		assert(!empty());
		--m_size;
		this->get(this->last()).reset();
		do {
			--m_end;
		} while (m_end != m_begin && !this->get(this->last()));
	}

	auto erase(const key_type& key) noexcept -> size_type {
		if (!this->empty()) {
			const auto i = static_cast<size_type>(key);
			if (auto& value = this->get(i); value && value->first == key) {
				if (i == this->first()) {
					this->pop_front();
				} else if (i == this->last()) {
					this->pop_back();
				} else {
					--m_size;
					value.reset();
				}
				return size_type{1};
			}
		}
		return size_type{0};
	}

	auto erase(const_iterator it) noexcept -> iterator {
		assert(!this->empty());
		assert(this->get(it.m_index).has_value());

		if (it.m_index == this->first()) {
			this->pop_front();
			return this->begin();
		}

		if (it.m_index == this->last()) {
			this->pop_back();
			return this->end();
		}

		--m_size;
		this->get(it.m_index).reset();
		++it;
		return iterator{this, it.m_index};
	}

private:
	[[nodiscard]] auto first() const noexcept -> size_type {
		return m_begin;
	}

	[[nodiscard]] auto last() const noexcept -> size_type {
		return static_cast<size_type>(m_end - 1);
	}

	[[nodiscard]] auto get(size_type i) noexcept -> std::optional<value_type>& {
		assert(m_capacity > 0);
		return m_values[i & (m_capacity - 1)];
	}

	[[nodiscard]] auto get(size_type i) const noexcept -> const std::optional<value_type>& {
		assert(m_capacity > 0);
		return m_values[i & (m_capacity - 1)];
	}

	template <typename K, typename... Args>
	[[nodiscard]] auto try_emplace_impl(K&& key, Args&&... args) -> std::pair<iterator, bool> {
		const auto i = static_cast<size_type>(key);
		if (empty()) {
			this->reserve(1);
			this->get(i).emplace(std::piecewise_construct, std::forward_as_tuple(std::forward<K>(key)), std::forward_as_tuple(std::forward<Args>(args)...));
			m_begin = i;
			m_end = static_cast<size_type>(i + 1);
		} else {
			const auto relativeIndex = static_cast<size_type>(i - m_begin);
			if (relativeIndex >= m_capacity) {
				this->reserve(static_cast<size_type>(relativeIndex + 1));
			}
			const auto inRange = relativeIndex < static_cast<size_type>(m_end - m_begin);
			auto& value = this->get(i);
			if (inRange && value) {
				return {iterator{this, i}, false};
			}
			value.emplace(std::piecewise_construct, std::forward_as_tuple(std::forward<K>(key)), std::forward_as_tuple(std::forward<Args>(args)...));
			if (!inRange) {
				m_end = static_cast<size_type>(i + 1);
			}
		}
		++m_size;
		return {iterator{this, i}, true};
	}

	template <typename K, typename M>
	[[nodiscard]] auto insert_or_assign_impl(K&& key, M&& obj) -> std::pair<iterator, bool> {
		static_assert(std::is_convertible_v<M, mapped_type>);
		auto result = this->try_emplace(key, std::forward<M>(obj));
		if (!result.second) {
			result.first->second = std::forward<M>(obj);
		}
		return result;
	}

	std::unique_ptr<std::optional<value_type>[]> m_values = nullptr;
	size_type m_capacity = 0;
	size_type m_size = 0;
	size_type m_begin = 0;
	size_type m_end = 0;
};

} // namespace util

#endif
