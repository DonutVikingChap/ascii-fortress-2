#ifndef AF2_UTILITIES_REGISTRY_HPP
#define AF2_UTILITIES_REGISTRY_HPP

#include "arrow_proxy.hpp" // util::ArrowProxy

#include <algorithm>   // std::lower_bound
#include <cassert>     // assert
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdint>     // std::uint64_t
#include <iterator>    // std::forward_iterator_tag, std::distance
#include <memory>      // std::addressof
#include <optional>    // std::optional, std::nullopt
#include <type_traits> // std::conditional_t, std::enable_if_t, std::is_unsigned_v
#include <utility>     // std::pair, std::move, std::forward
#include <vector>      // std::vector

namespace util {

template <typename T, typename Identifier = std::uint64_t>
class Registry final {
public:
	using key_type = Identifier;
	using mapped_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	static constexpr auto INVALID_KEY = key_type{};

private:
	struct Element final {
		size_type skip = 0;
		key_type id = INVALID_KEY;
		std::optional<mapped_type> storage{};
	};

	using Container = std::vector<Element>;

	template <bool CONST_VALUE, bool REVERSE>
	class Iterator final {
	public:
		using difference_type = typename Registry::difference_type;
		using value_type = std::conditional_t<CONST_VALUE, const typename Registry::mapped_type, typename Registry::mapped_type>;
		using reference = std::pair<const key_type, value_type&>;
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::forward_iterator_tag;

	private:
		using ElementPointer = std::conditional_t<CONST_VALUE, const Element*, Element*>;

	public:
		constexpr Iterator() = default;

		constexpr explicit Iterator(ElementPointer ptr) noexcept
			: m_pointer(ptr) {}

		~Iterator() = default;

		constexpr Iterator(const Iterator&) noexcept = default;
		constexpr Iterator(Iterator&&) noexcept = default;

		constexpr auto operator=(const Iterator&) noexcept -> Iterator& = default;
		constexpr auto operator=(Iterator&&) noexcept -> Iterator& = default;

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr Iterator(const Iterator<OTHER_CONST, REVERSE>& other) noexcept
			: m_pointer(other.m_pointer) {}

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr Iterator(Iterator<OTHER_CONST, REVERSE>&& other) noexcept
			: m_pointer(other.m_pointer) {}

		[[nodiscard]] constexpr auto operator==(const Iterator& other) const noexcept -> bool {
			return m_pointer == other.m_pointer;
		}

		[[nodiscard]] constexpr auto operator!=(const Iterator& other) const noexcept -> bool {
			return m_pointer != other.m_pointer;
		}

		[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
			assert(m_pointer);
			return reference{m_pointer->id, *m_pointer->storage};
		}

		constexpr auto operator++() noexcept -> Iterator& {
			assert(m_pointer);
			if constexpr (REVERSE) {
				--m_pointer;
				m_pointer -= m_pointer->skip;
			} else {
				++m_pointer;
				m_pointer += m_pointer->skip;
			}
			return *this;
		}

		[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++(int) noexcept -> Iterator {
			auto old = *this;
			++*this;
			return old;
		}

	private:
		friend Registry;
		friend Iterator<true, REVERSE>;

		ElementPointer m_pointer = nullptr;
	};

	template <bool CONST_VALUE, bool REVERSE>
	class StableIterator final {
	public:
		using difference_type = typename Registry::difference_type;
		using value_type = std::conditional_t<CONST_VALUE, const typename Registry::mapped_type, typename Registry::mapped_type>;
		using reference = std::pair<const key_type, value_type*>;
		using pointer = util::ArrowProxy<reference>;
		using iterator_category = std::forward_iterator_tag;
		using unstable_iterator = Iterator<CONST_VALUE, REVERSE>;
		using const_unstable_iterator = Iterator<true, REVERSE>;

	private:
		using ContainerPointer = std::conditional_t<CONST_VALUE, const Container*, Container*>;

	public:
		constexpr StableIterator() = default;

		constexpr StableIterator(ContainerPointer cont, size_type i) noexcept
			: m_container(cont)
			, m_index(i) {}

		~StableIterator() = default;

		constexpr StableIterator(StableIterator&&) noexcept = default;
		constexpr StableIterator(const StableIterator&) noexcept = default;

		constexpr auto operator=(StableIterator&&) noexcept -> StableIterator& = default;
		constexpr auto operator=(const StableIterator&) noexcept -> StableIterator& = default;

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr StableIterator(const StableIterator<OTHER_CONST, REVERSE>& other) noexcept
			: m_container(other.m_container)
			, m_index(other.m_index) {}

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr StableIterator(StableIterator<OTHER_CONST, REVERSE>&& other) noexcept
			: m_container(other.m_container)
			, m_index(other.m_index) {}

		template <bool OTHER_CONST, typename = std::enable_if_t<OTHER_CONST || !CONST_VALUE>>
		constexpr operator Iterator<OTHER_CONST, REVERSE>() const noexcept {
			return (m_container) ? Iterator<OTHER_CONST, REVERSE>{&(*m_container)[m_index]} : Iterator<OTHER_CONST, REVERSE>{};
		}

		[[nodiscard]] friend constexpr auto operator==(const StableIterator& lhs, const StableIterator& rhs) noexcept -> bool {
			assert(lhs.m_container == rhs.m_container);
			assert((lhs.m_index == 0 && lhs.m_container == nullptr) ||
				   (lhs.m_container != nullptr && lhs.m_index > 0 && lhs.m_index < lhs.m_container->size()));
			assert((rhs.m_index == 0 && rhs.m_container == nullptr) ||
				   (rhs.m_container != nullptr && rhs.m_index > 0 && rhs.m_index < rhs.m_container->size()));
			return lhs.m_index == rhs.m_index;
		}

		[[nodiscard]] friend constexpr auto operator!=(const StableIterator& lhs, const StableIterator& rhs) noexcept -> bool {
			return !(lhs == rhs);
		}

		[[nodiscard]] constexpr auto operator*() const noexcept -> reference {
			assert(m_container);
			assert(m_index > 0);
			assert(m_index + 1 < m_container->size());
			auto&& element = (*m_container)[m_index];
			return reference{element.id, (element.storage) ? &*element.storage : nullptr};
		}

		constexpr auto operator++() noexcept -> StableIterator& {
			assert(m_container);
			assert(m_index > 0);
			assert(m_index + 1 < m_container->size());
			if constexpr (REVERSE) {
				static_assert(std::is_unsigned_v<size_type>);
				if ((*m_container)[m_index].storage.has_value()) {
					--m_index;
					m_index -= (*m_container)[m_index].skip;
				} else {
					do {
						--m_index;
					} while (m_index != size_type{0} && !(*m_container)[m_index].storage.has_value());
				}
			} else {
				if ((*m_container)[m_index].storage.has_value()) {
					++m_index;
					m_index += (*m_container)[m_index].skip;
				} else {
					do {
						++m_index;
					} while (m_index != m_container->size() - 1 && !(*m_container)[m_index].storage.has_value());
				}
			}
			return *this;
		}

		[[nodiscard]] constexpr auto operator->() const noexcept -> pointer {
			return pointer{**this};
		}

		constexpr auto operator++(int) noexcept -> StableIterator {
			auto old = *this;
			++*this;
			return old;
		}

	private:
		friend Registry;
		friend StableIterator<true, REVERSE>;

		ContainerPointer m_container = nullptr;
		size_type m_index = 0;
	};

	template <bool CONST_VALUE>
	class StableView final {
	public:
		using size_type = typename Registry::size_type;
		using difference_type = typename Registry::difference_type;
		using iterator = StableIterator<CONST_VALUE, false>;
		using const_iterator = StableIterator<true, false>;
		using reverse_iterator = StableIterator<CONST_VALUE, true>;
		using const_reverse_iterator = StableIterator<true, true>;

	private:
		using RegistryPointer = std::conditional_t<CONST_VALUE, const Registry*, Registry*>;

	public:
		constexpr StableView() = default;

		constexpr explicit StableView(RegistryPointer reg) noexcept
			: m_registry(reg) {}

		~StableView() = default;

		constexpr StableView(StableView&&) noexcept = default;
		constexpr StableView(const StableView&) noexcept = default;

		constexpr auto operator=(StableView&&) noexcept -> StableView& = default;
		constexpr auto operator=(const StableView&) noexcept -> StableView& = default;

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr StableView(const StableView<OTHER_CONST>& other) noexcept
			: m_registry(other.m_registry) {}

		template <bool OTHER_CONST, typename = std::enable_if_t<CONST_VALUE && !OTHER_CONST>>
		constexpr StableView(StableView<OTHER_CONST>&& other) noexcept
			: m_registry(other.m_registry) {}

		[[nodiscard]] auto begin() noexcept -> iterator {
			assert(m_registry);
			return m_registry->stable_begin();
		}

		[[nodiscard]] auto begin() const noexcept -> const_iterator {
			assert(m_registry);
			return m_registry->stable_begin();
		}

		[[nodiscard]] auto end() noexcept -> iterator {
			assert(m_registry);
			return m_registry->stable_end();
		}

		[[nodiscard]] auto end() const noexcept -> const_iterator {
			assert(m_registry);
			return m_registry->stable_end();
		}

		[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
			assert(m_registry);
			return m_registry->stable_cbegin();
		}

		[[nodiscard]] auto cend() const noexcept -> const_iterator {
			assert(m_registry);
			return m_registry->stable_cend();
		}

		[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
			assert(m_registry);
			return m_registry->stable_rbegin();
		}

		[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
			assert(m_registry);
			return m_registry->stable_rbegin();
		}

		[[nodiscard]] auto rend() noexcept -> reverse_iterator {
			assert(m_registry);
			return m_registry->stable_rend();
		}

		[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
			assert(m_registry);
			return m_registry->stable_rend();
		}

		[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
			assert(m_registry);
			return m_registry->stable_crbegin();
		}

		[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
			assert(m_registry);
			return m_registry->stable_crend();
		}

	private:
		RegistryPointer m_registry = nullptr;
	};

public:
	using iterator = Iterator<false, false>;
	using const_iterator = Iterator<true, false>;
	using reverse_iterator = Iterator<false, true>;
	using const_reverse_iterator = Iterator<true, true>;
	using stable_iterator = StableIterator<false, false>;
	using const_stable_iterator = StableIterator<true, false>;
	using reverse_stable_iterator = StableIterator<false, true>;
	using const_reverse_stable_iterator = StableIterator<true, true>;
	using stable_view = StableView<false>;
	using const_stable_view = StableView<true>;

	Registry() = default;

	~Registry() = default;

	Registry(const Registry&) = default;

	Registry(Registry&& other) noexcept {
		// Use swap instead of move to guarantee that the allocated memory
		// of the container doesn't get relocated.
		this->swap(other);
	}

	auto operator=(const Registry&) -> Registry& = default;

	auto operator=(Registry&& other) noexcept -> Registry& {
		if (&other == this) {
			return *this;
		}
		// Don't call the regular clear() here since that would try to keep iterators valid for no reason.
		m_container = {Element{}, Element{}};
		m_begin = 0;
		m_rbegin = 0;
		m_size = 0;
		m_id = INVALID_KEY;
		// Use swap instead of move to guarantee that the allocated memory
		// of the container doesn't get relocated.
		this->swap(other);
		return *this;
	}

	/**
	 * Erase all elements from the container.
	 * Previously acquired stable iterators remain valid, but will point to empty elements.
	 */
	auto clear() noexcept -> void {
		// Leave the elements allocated in the container so that stable iterators are not invalidated.
		const auto end = m_container.size() - 1;
		for (auto i = size_type{1}; i < end; ++i) {
			m_container[i].storage.reset();
		}
		m_begin = m_container.size() - 2;
		m_rbegin = 0;
		m_size = 0;
	}

	/**
	 * Get the maximum number of elements that the container can hold.
	 *
	 * @return Maximum size of the container.
	 */
	[[nodiscard]] auto max_size() const noexcept -> size_type {
		return m_container.max_size() - 2;
	}

	/**
	 * Get the current capacity of the container, i.e. how many elements
	 * can be held before a re-allocation is required.
	 *
	 * @return Current capacity of the container.
	 */
	[[nodiscard]] auto capacity() const noexcept -> size_type {
		return m_container.capacity() - 2;
	}

	/**
	 * Get the current number of elements in the container.
	 *
	 * @return Current element count of the container.
	 */
	[[nodiscard]] auto size() const noexcept -> size_type {
		return m_size;
	}

	/**
	 * Check if the container is empty.
	 *
	 * @return True if the container contains no elements.
	 */
	[[nodiscard]] auto empty() const noexcept -> bool {
		return m_size == 0;
	}

	/**
	 * Swap the contents of this container with another.
	 * Pointers remain valid after the swap and point to the same elements as they did before.
	 * However, iterators are invalidated.
	 *
	 * @warning This function invalidates all previously acquired iterators into
	 *          the container.
	 *
	 * @param other Container to swap with.
	 */
	auto swap(Registry& other) noexcept -> void {
		using std::swap;
		m_container.swap(other.m_container);
		swap(m_begin, other.m_begin);
		swap(m_rbegin, other.m_rbegin);
		swap(m_size, other.m_size);
		swap(m_id, other.m_id);
	}

	/**
	 * Swap the contents of two containers.
	 * Pointers remain valid after the swap and point to the same elements as they did before.
	 * However, iterators are invalidated.
	 *
	 * @warning This function invalidates all previously acquired iterators into the container.
	 *
	 * @param a First container to swap.
	 * @param b Second container to swap.
	 */
	friend auto swap(Registry& a, Registry& b) noexcept -> void {
		a.swap(b);
	}

	/**
	 * Reclaim the space left by elements that have been erased from the container.
	 * This function should be called occasionally between insertions to avoid a memory leak.
	 * To also de-allocate the memory that is left over, use shrink_to_fit() instead.
	 *
	 * @warning This function invalidates all previously acquired iterators and pointers into the container.
	 */
	auto commit() -> void {
		if (m_rbegin == m_size) {
			return; // The elements are already packed.
		}

		// Find the first skip. This is done by smultaneously incrementing an
		// iterator and a regular index until the iterator's internal index
		// differs from the regular index.
		const auto end = this->end();

		auto i = size_type{1};
		auto it = this->begin();
		while (it != end && i == static_cast<size_type>(it.m_pointer - m_container.data())) {
			++i;
			++it;
		}

		// Go through all the elements after the first skip and move them to
		// their appropriate indices. Make sure to destroy moved-from elements
		// so that we can safely erase them from the container afterwards.
		while (it != end) {
			auto& lhsElement = m_container[i];
			auto& rhsElement = *it.m_pointer;
			lhsElement.storage.emplace(std::move(*rhsElement.storage));
			rhsElement.storage.reset();
			lhsElement.skip = 0;
			lhsElement.id = rhsElement.id;
			++i;
			++it;
		}

		// Make the last remaining element a dummy element.
		m_container[m_size + 1] = Element{};

		// Remove the slack at the end of the container.
		m_container.erase(m_container.begin() + m_size + 2, m_container.end());

		// Update iterators. (Size remains the same.)
		m_begin = 0;
		m_rbegin = m_size;
	}

	/**
	 * Reclaim the space left by elements that have been erased from the
	 * container and de-allocate the memory that is left over.
	 *
	 * @warning This function invalidates all previously acquired iterators and
	 *          pointers into the container.
	 */
	auto shrink_to_fit() -> void {
		this->commit();
		m_container.shrink_to_fit();
	}

	/**
	 * Reclaim the space left by elements that have been erased from the
	 * container and then allocate enough space to fit a given number of elements.
	 *
	 * @warning This function invalidates all previously acquired iterators and
	 *          pointers into the container.
	 *
	 * @param new_capacity Number of elements to allocate space for.
	 */
	auto reserve(size_type newCapacity) -> void {
		this->commit();
		m_container.reserve(newCapacity + 2);
	}

	/**
	 * Add a new element to the end of the container.
	 * The element is constructed in-place using the arguments supplied to this function through perfect forwarding.
	 * This function does not invalidate stable iterators. However, pointers and non-stable iterators may be invalidated.
	 *
	 * @warning If a reallocation is required, this function invalidates all previously acquired pointers and non-stable iterators into the container.
	 *
	 * @param args Constructor arguments for the new element.
	 *
	 * @return Iterator to the newly added element.
	 */
	template <typename... Args>
	[[nodiscard]] auto stable_emplace_back(Args&&... args) -> stable_iterator {
		// Don't try to reclaim space here, since that could invalidate stable iterators.
		// Users of this container are expected to call commit() manually when it is appropriate.
		m_container.emplace_back();
		auto& element = m_container[m_container.size() - 2];
		element.skip = 0;
		element.id = ++m_id;
		element.storage.emplace(std::forward<Args>(args)...);
		m_rbegin = m_container.size() - 2;
		++m_size;
		return stable_iterator{&m_container, m_rbegin};
	}

	/**
	 * Add a new element to the end of the container.
	 * The element is constructed in-place using the arguments supplied to this function through perfect forwarding.
	 *
	 * @warning If a reallocation is required, this function invalidates all previously acquired pointers and iterators into the container.
	 *
	 * @param args Constructor arguments for the new element.
	 *
	 * @return Iterator to the newly added element.
	 */
	template <typename... Args>
	[[nodiscard]] auto emplace_back(Args&&... args) -> iterator {
		if (m_size == m_container.size() - 2) {
			this->commit(); // Try to reclaim space so we don't have to reallocate.
		}
		return iterator{this->stable_emplace_back(std::forward<Args>(args)...)};
	}

	/**
	 * Add a new element to the end of the container.
	 * The element is copied into place.
	 * This function does not invalidate stable iterators. However, pointers and non-stable iterators may be invalidated.
	 *
	 * @warning If a reallocation is required, this function invalidates all previously acquired pointers and non-stable iterators into the container.
	 *
	 * @param element The element to copy.
	 *
	 * @return Iterator to the newly added element.
	 */
	[[nodiscard]] auto stable_push_back(const T& element) -> stable_iterator {
		return this->stable_emplace_back(element);
	}

	/**
	 * Add a new element to the end of the container.
	 * The element is copied into place.
	 *
	 * @warning If a reallocation is required, this function invalidates all previously acquired pointers and iterators into the container.
	 *
	 * @param element The element to copy.
	 *
	 * @return Iterator to the newly added element.
	 */
	[[nodiscard]] auto push_back(const T& element) -> iterator {
		if (m_size == m_container.size() - 2) {
			this->commit(); // Try to reclaim space so we don't have to reallocate.
		}
		return iterator{this->stable_push_back(element)};
	}

	/**
	 * Add a new element to the end of the container.
	 * The element is moved into place.
	 * This function does not invalidate stable iterators. However, pointers and non-stable iterators may be invalidated.
	 *
	 * @warning If a reallocation is required, this function invalidates all previously acquired pointers and non-stable iterators into the container.
	 *
	 * @param element The element to copy.
	 *
	 * @return Iterator to the newly added element.
	 */
	[[nodiscard]] auto stable_push_back(T&& element) -> stable_iterator {
		return this->stable_emplace_back(std::move(element));
	}

	/**
	 * @see stable_push_back(T&&)
	 */
	[[nodiscard]] auto push_back(T&& element) -> iterator {
		return iterator{this->stable_push_back(std::move(element))};
	}

	/**
	 * Get an iterator to the element with the given identifier.
	 * This performs a binary search through the container to find the element.
	 *
	 * @param id Identifier of the element to search for.
	 *
	 * @return Iterator to the element, or an iterator equal to end() if the element is not found.
	 */
	[[nodiscard]] auto stable_find(const key_type& id) -> stable_iterator {
		if (!this->empty()) {
			const auto begin = m_container.begin() + 1 + m_begin;
			const auto end = m_container.begin() + 1 + m_rbegin;
			if (const auto it = std::lower_bound(begin, end, id, [](const auto& lhs, const auto& rhs) { return lhs.id < rhs; });
				it != end && it->id == id && it->storage.has_value()) {
				return stable_iterator{&m_container, static_cast<size_type>(std::distance(m_container.begin(), it))};
			}
		}
		return this->stable_end();
	}

	/**
	 * @see stable_find(const key_type&)
	 */
	[[nodiscard]] auto find(const key_type& id) -> iterator {
		return iterator{this->stable_find(id)};
	}

	/**
	 * @see stable_find(const key_type&)
	 */
	[[nodiscard]] auto stable_find(const key_type& id) const -> const_stable_iterator {
		if (!this->empty()) {
			const auto begin = m_container.begin() + 1 + m_begin;
			const auto end = m_container.begin() + 1 + m_rbegin;
			if (const auto it = std::lower_bound(begin, end, id, [](const auto& lhs, const auto& rhs) { return lhs.id < rhs; });
				it != end && it->id == id && it->storage.has_value()) {
				return const_stable_iterator{&m_container, static_cast<size_type>(std::distance(m_container.begin(), it))};
			}
		}
		return this->stable_end();
	}

	/**
	 * @see find(const key_type&)
	 */
	[[nodiscard]] auto find(const key_type& id) const -> const_iterator {
		return const_iterator{this->stable_find(id)};
	}

	/**
	 * Check if the container contains an element with the given identifier.
	 * This performs a binary search through the container to find the element.
	 *
	 * @param id Identifier of the element to search for.
	 *
	 * @return True if the element is found.
	 */
	[[nodiscard]] auto contains(const key_type& id) const -> bool {
		return this->find(id) != this->end();
	}

	/**
	 * Count the number of elements in the container with the given identifier.
	 * This performs a binary search through the container to find the element.
	 * The result will always be equal to 0 or 1.
	 *
	 * @param id Identifier of the element to search for.
	 *
	 * @return 1 if the element is found, 0 otherwise.
	 */
	[[nodiscard]] auto count(const key_type& id) const -> size_type {
		return (this->contains(id)) ? size_type{1} : size_type{0};
	}

	/**
	 * Remove an element from the container.
	 * Use commit() or shrink_to_fit() to reclaim the space left by the element after being erased.
	 *
	 * @param pos Iterator to the element to remove.
	 *
	 * @return Iterator to the next valid element in the container,
	 *         or an iterator equal to end() if there are no more elements remaining.
	 */
	auto stable_erase(const_iterator pos) -> stable_iterator {
		assert(!this->empty());
		assert(pos.m_pointer);
		const auto i = static_cast<size_type>(pos.m_pointer - m_container.data());
		assert(i < m_container.size() - 1);
		auto& element = m_container[i];
		assert(element.storage.has_value());

		// Destroy the element.
		element.storage.reset();

		// Update size.
		--m_size;

		// Update skipfield and iterators.
		// NOTE: Do not touch the id! It is important that it remains intact in
		// order for the binary search to work when using find() to look for other elements.
		const auto leftSkip = m_container[i - 1].skip;
		const auto rightSkip = m_container[i + 1].skip;
		const auto skip = leftSkip + rightSkip + 1;

		m_container[i - leftSkip].skip = skip;
		m_container[i + rightSkip].skip = skip;

		const auto next = i + rightSkip + 1;
		if (i == m_begin + 1) {
			m_begin = next - 1;
		}
		if (i == m_rbegin) {
			m_rbegin = i - leftSkip - 1;
		}
		return stable_iterator{&m_container, next};
	}

	/**
	 * @see stable_erase(const_iterator)
	 */
	auto erase(const_iterator pos) -> iterator {
		return iterator{this->stable_erase(pos)};
	}

	/**
	 * Remove a range of elements from the container.
	 * Use commit() or shrink_to_fit() to reclaim the space left by the elements after being erased.
	 *
	 * @param it Iterator to the first element to remove.
	 * @param end_it Iterator one past the last element to remove.
	 *
	 * @return Iterator to the next valid element in the container,
	 *         or an iterator equal to end() if there are no more elements remaining.
	 */
	auto stable_erase(const_iterator it, const_iterator end_it) -> stable_iterator {
		while (it != end_it) {
			it = this->erase(it);
		}
		return stable_iterator{&m_container, static_cast<size_type>(it.m_pointer - m_container.data())};
	}

	/**
	 * @see stable_erase(const_iterator, const_iterator)
	 */
	auto erase(const_iterator it, const_iterator end_it) -> iterator {
		return iterator{this->stable_erase(it, end_it)};
	}

	/**
	 * Remove the element with the given identifier from the container.
	 * This performs a binary search through the container to find the element.
	 * Use commit() or shrink_to_fit() to reclaim the space left by the element after being erased.
	 *
	 * @param id Identifier of the element to remove.
	 *
	 * @return Number of elements that were successfully removed, i.e. 1 if the element was found, 0 otherwise.
	 */
	auto erase(const key_type& id) -> size_type {
		if (const auto it = this->find(id); it != this->end()) {
			this->erase(it);
			return size_type{1};
		}
		return size_type{0};
	}

	/**
	 * Remove a range of elements from the container given an inclusive range of identifiers.
	 * This performs a binary search through the container to find the first element.
	 * Use commit() or shrink_to_fit() to reclaim the space left by the elements after being erased.
	 *
	 * @param first Identifier of the first element to remove.
	 * @param last Identifier of the last element to remove.
	 *
	 * @return Number of elements that were successfully removed.
	 */
	auto erase(const key_type& first, const key_type& last) -> size_type {
		assert(first <= last);
		auto erased_count = size_type{0};

		const auto end_it = this->end();
		for (auto it = this->find(first); it != end_it;) {
			++erased_count;
			if (it->first == last) {
				this->erase(it);
				break;
			}
			it = this->erase(it);
		}
		return erased_count;
	}

	[[nodiscard]] auto begin() noexcept -> iterator {
		return iterator{m_container.data() + 1 + m_begin};
	}

	[[nodiscard]] auto begin() const noexcept -> const_iterator {
		return const_iterator{m_container.data() + 1 + m_begin};
	}

	[[nodiscard]] auto end() noexcept -> iterator {
		return iterator{m_container.data() + m_container.size() - 1};
	}

	[[nodiscard]] auto end() const noexcept -> const_iterator {
		return const_iterator{m_container.data() + m_container.size() - 1};
	}

	[[nodiscard]] auto cbegin() const noexcept -> const_iterator {
		return this->begin();
	}

	[[nodiscard]] auto cend() const noexcept -> const_iterator {
		return this->end();
	}

	[[nodiscard]] auto rbegin() noexcept -> reverse_iterator {
		return reverse_iterator{m_container.data() + m_rbegin};
	}

	[[nodiscard]] auto rbegin() const noexcept -> const_reverse_iterator {
		return const_reverse_iterator{m_container.data() + m_rbegin};
	}

	[[nodiscard]] auto rend() noexcept -> reverse_iterator {
		return reverse_iterator{m_container.data() + 1};
	}

	[[nodiscard]] auto rend() const noexcept -> const_reverse_iterator {
		return const_reverse_iterator{m_container.data() + 1};
	}

	[[nodiscard]] auto crbegin() const noexcept -> const_reverse_iterator {
		return this->rbegin();
	}

	[[nodiscard]] auto crend() const noexcept -> const_reverse_iterator {
		return this->rend();
	}

	[[nodiscard]] auto stable_begin() noexcept -> stable_iterator {
		return stable_iterator{&m_container, m_begin + 1};
	}

	[[nodiscard]] auto stable_begin() const noexcept -> const_stable_iterator {
		return const_stable_iterator{&m_container, m_begin + 1};
	}

	[[nodiscard]] auto stable_end() noexcept -> stable_iterator {
		return stable_iterator{&m_container, m_container.size() - 1};
	}

	[[nodiscard]] auto stable_end() const noexcept -> const_stable_iterator {
		return const_stable_iterator{&m_container, m_container.size() - 1};
	}

	[[nodiscard]] auto stable_cbegin() const noexcept -> const_stable_iterator {
		return this->stable_begin();
	}

	[[nodiscard]] auto stable_cend() const noexcept -> const_stable_iterator {
		return this->stable_end();
	}

	[[nodiscard]] auto stable_rbegin() noexcept -> reverse_stable_iterator {
		return reverse_stable_iterator{&m_container, m_rbegin};
	}

	[[nodiscard]] auto stable_rbegin() const noexcept -> const_reverse_stable_iterator {
		return const_reverse_stable_iterator{&m_container, m_rbegin};
	}

	[[nodiscard]] auto stable_rend() noexcept -> reverse_stable_iterator {
		return reverse_stable_iterator{&m_container, size_type{1}};
	}

	[[nodiscard]] auto stable_rend() const noexcept -> const_reverse_stable_iterator {
		return const_reverse_stable_iterator{&m_container, size_type{1}};
	}

	[[nodiscard]] auto stable_crbegin() const noexcept -> const_reverse_stable_iterator {
		return this->stable_rbegin();
	}

	[[nodiscard]] auto stable_crend() const noexcept -> const_reverse_stable_iterator {
		return this->stable_rend();
	}

	[[nodiscard]] auto stable() noexcept -> stable_view {
		return stable_view{this};
	}

	[[nodiscard]] auto stable() const noexcept -> const_stable_view {
		return const_stable_view{this};
	}

	[[nodiscard]] friend auto operator==(const Registry& lhs, const Registry& rhs) -> bool {
		return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	[[nodiscard]] friend auto operator!=(const Registry& lhs, const Registry& rhs) -> bool {
		return !(lhs == rhs);
	}

	[[nodiscard]] friend auto operator<(const Registry& lhs, const Registry& rhs) -> bool {
		return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
	}

	[[nodiscard]] friend auto operator<=(const Registry& lhs, const Registry& rhs) -> bool {
		return !(rhs < lhs);
	}

	[[nodiscard]] friend auto operator>(const Registry& lhs, const Registry& rhs) -> bool {
		return rhs < lhs;
	}

	[[nodiscard]] friend auto operator>=(const Registry& lhs, const Registry& rhs) -> bool {
		return !(lhs < rhs);
	}

private:
	Container m_container{Element{}, Element{}};
	size_type m_begin = 0;
	size_type m_rbegin = 0;
	size_type m_size = 0;
	key_type m_id = INVALID_KEY;
};

} // namespace util

#endif
