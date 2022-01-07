#ifndef AF2_CONSOLE_COMMAND_HPP
#define AF2_CONSOLE_COMMAND_HPP

#include "../utilities/span.hpp"   // util::Span
#include "../utilities/string.hpp" // util::toString

#include <cstddef>    // std::size_t
#include <cstdint>    // std::uint8_t
#include <fmt/core.h> // fmt::format
#include <iterator> // std::begin, std::end, std::distance, std::reverse_iterator, std::make_reverse_iterator, std::random_access_iterator_tag
#include <memory>   // std::addressof
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <type_traits> // std::enable_if_t, std::is_..._v
#include <utility>     // std::forward, std::move
#include <vector>      // std::vector

namespace cmd {

using Progress = std::size_t;

enum class Status : std::uint8_t {
	NONE,
	VALUE,               // The command yielded a value.
	ERROR_MSG,           // The command yielded an error message.
	NOT_DONE,            // We should run again with new progress as soon as possible.
	DEFER_TO_NEXT_FRAME, // We should run again with new progress on the next frame.
	RETURN,              // We should return from the current function.
	RETURN_VALUE,        // The command yielded a value which should be returned from the current function.
	BREAK,               // We should break from the current loop.
	CONTINUE,            // We should continue the current loop.
	CONDITION_FAILED,    // The previous condition failed.
};

using Value = std::string;

struct Result final {
	Status status;
	Value value;

	Result(Status status, Value value)
		: status(status)
		, value(std::move(value)) {}

	auto reset() noexcept -> void {
		status = Status::NONE;
		value.clear();
	}
};
using CommandArguments = std::vector<Result>;

class CommandView final {
private:
	using Container = util::Span<const Result>;

public:
	using size_type = typename Container::size_type;
	using difference_type = typename Container::difference_type;
	using value_type = const Value;
	using reference = const Value&;
	using pointer = const Value*;

	class iterator final {
	private:
		using ContainerIterator = typename CommandView::Container::iterator;

	public:
		using difference_type = typename CommandView::difference_type;
		using value_type = typename CommandView::value_type;
		using reference = typename CommandView::reference;
		using pointer = typename CommandView::pointer;
		using iterator_category = std::random_access_iterator_tag;

	private:
		friend CommandView;
		constexpr explicit iterator(ContainerIterator it)
			: m_it(it) {}

	public:
		iterator() noexcept = default;

		[[nodiscard]] constexpr auto operator==(const iterator& other) const -> bool {
			return m_it == other.m_it;
		}

		[[nodiscard]] constexpr auto operator!=(const iterator& other) const -> bool {
			return m_it != other.m_it;
		}

		[[nodiscard]] constexpr auto operator<(const iterator& other) const -> bool {
			return m_it < other.m_it;
		}

		[[nodiscard]] constexpr auto operator>(const iterator& other) const -> bool {
			return m_it > other.m_it;
		}

		[[nodiscard]] constexpr auto operator<=(const iterator& other) const -> bool {
			return m_it <= other.m_it;
		}

		[[nodiscard]] constexpr auto operator>=(const iterator& other) const -> bool {
			return m_it >= other.m_it;
		}

		[[nodiscard]] constexpr auto operator*() const -> reference {
			return m_it->value;
		}

		[[nodiscard]] constexpr auto operator->() const -> pointer {
			return std::addressof(**this);
		}

		constexpr auto operator++() noexcept -> iterator& {
			++m_it;
			return *this;
		}

		constexpr auto operator++(int) -> iterator {
			return iterator{m_it++};
		}

		constexpr auto operator--() noexcept -> iterator& {
			--m_it;
			return *this;
		}

		constexpr auto operator--(int) -> iterator {
			return iterator{m_it--};
		}

		constexpr auto operator+=(difference_type n) noexcept -> iterator& {
			m_it += n;
			return *this;
		}

		constexpr auto operator-=(difference_type n) noexcept -> iterator& {
			m_it -= n;
			return *this;
		}

		[[nodiscard]] constexpr auto operator[](difference_type n) -> reference {
			return m_it[n].value;
		}

		[[nodiscard]] constexpr friend auto operator+(const iterator& lhs, difference_type rhs) -> iterator {
			return iterator{lhs.m_it + rhs};
		}

		[[nodiscard]] constexpr friend auto operator+(difference_type lhs, const iterator& rhs) -> iterator {
			return iterator{lhs + rhs.m_it};
		}

		[[nodiscard]] constexpr friend auto operator-(const iterator& lhs, difference_type rhs) -> iterator {
			return iterator{lhs.m_it - rhs};
		}

		[[nodiscard]] constexpr friend auto operator-(const iterator& lhs, const iterator& rhs) noexcept -> difference_type {
			return lhs.m_it - rhs.m_it;
		}

	private:
		ContainerIterator m_it{};
	};

	using reverse_iterator = std::reverse_iterator<iterator>;

	constexpr explicit CommandView(util::Span<const Result> arguments) noexcept
		: m_arguments(arguments) {}

	[[nodiscard]] constexpr auto size() const noexcept -> size_type {
		return m_arguments.size();
	}

	[[nodiscard]] constexpr auto empty() const noexcept -> bool {
		return m_arguments.empty();
	}

	[[nodiscard]] constexpr auto begin() const noexcept -> iterator {
		return iterator{m_arguments.begin()};
	}

	[[nodiscard]] constexpr auto end() const noexcept -> iterator {
		return iterator{m_arguments.end()};
	}

	[[nodiscard]] constexpr auto cbegin() const noexcept -> iterator {
		return this->begin();
	}

	[[nodiscard]] constexpr auto cend() const noexcept -> iterator {
		return this->end();
	}

	[[nodiscard]] constexpr auto rbegin() const noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->end());
	}

	[[nodiscard]] constexpr auto rend() const noexcept -> reverse_iterator {
		return std::make_reverse_iterator(this->begin());
	}

	[[nodiscard]] constexpr auto crbegin() const noexcept -> reverse_iterator {
		return this->rbegin();
	}

	[[nodiscard]] constexpr auto crend() const noexcept -> reverse_iterator {
		return this->rend();
	}

	[[nodiscard]] constexpr auto front() const noexcept -> reference {
		return m_arguments.front().value;
	}

	[[nodiscard]] constexpr auto back() const noexcept -> reference {
		return m_arguments.back().value;
	}

	[[nodiscard]] constexpr auto operator[](size_type i) const noexcept -> reference {
		return m_arguments[i].value;
	}

	[[nodiscard]] constexpr auto subCommand(std::size_t offset, std::size_t count) const -> CommandView {
		return CommandView{m_arguments.subspan(offset, count)};
	}

	[[nodiscard]] constexpr auto subCommand(std::size_t offset) const -> CommandView {
		return CommandView{m_arguments.subspan(offset)};
	}

private:
	Container m_arguments;
};

template <typename... Args>
[[nodiscard]] inline auto done(std::string_view format, Args&&... args) -> Result {
	return {Status::VALUE, fmt::format(format, std::forward<Args>(args)...)};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto done(const char* value) -> Result {
	return {Status::VALUE, Value{value}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto done(std::string_view value) -> Result {
	return {Status::VALUE, Value{value}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto done(Value value) -> Result {
	return {Status::VALUE, std::move(value)};
}

[[nodiscard]] inline auto done() noexcept -> Result {
	return {Status::NONE, Value{}};
}

[[nodiscard]] inline auto done(bool value) -> Result {
	return {Status::VALUE, (value) ? Value{"1"} : Value{"0"}};
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
[[nodiscard]] inline auto done(T value) -> Result {
	return {Status::VALUE, util::toString(value)};
}

[[nodiscard]] inline auto notDone(Progress progress) -> Result {
	return {Status::NOT_DONE, util::toString(progress)};
}

[[nodiscard]] inline auto deferToNextFrame(Progress progress) -> Result {
	return {Status::DEFER_TO_NEXT_FRAME, util::toString(progress)};
}

[[nodiscard]] inline auto broke() noexcept -> Result {
	return {Status::BREAK, Value{}};
}

[[nodiscard]] inline auto continued() noexcept -> Result {
	return {Status::CONTINUE, Value{}};
}

[[nodiscard]] inline auto failedCondition() noexcept -> Result {
	return {Status::CONDITION_FAILED, Value{}};
}

template <typename... Args>
[[nodiscard]] inline auto returned(std::string_view format, Args&&... args) -> Result {
	return {Status::RETURN_VALUE, fmt::format(format, std::forward<Args>(args)...)};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto returned(const char* value) -> Result {
	return {Status::RETURN_VALUE, std::string{value}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto returned(std::string_view value) -> Result {
	return {Status::RETURN_VALUE, std::string{value}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto returned(Value value) -> Result {
	return {Status::RETURN_VALUE, std::move(value)};
}

[[nodiscard]] inline auto returned() noexcept -> Result {
	return {Status::RETURN, Value{}};
}

[[nodiscard]] inline auto returned(bool value) -> Result {
	return {Status::RETURN_VALUE, (value) ? Value{"1"} : Value{"0"}};
}

template <typename T, typename = std::enable_if_t<std::is_integral_v<T> || std::is_floating_point_v<T>>>
[[nodiscard]] inline auto returned(T value) -> Result {
	return {Status::RETURN_VALUE, util::toString(value)};
}

template <typename... Args>
[[nodiscard]] inline auto error(std::string_view format, Args&&... args) -> Result {
	return {Status::ERROR_MSG, fmt::format(format, std::forward<Args>(args)...)};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto error(const char* message) -> Result {
	return {Status::ERROR_MSG, Value{message}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto error(std::string_view message) -> Result {
	return {Status::ERROR_MSG, Value{message}};
}

// Note: This overload is important for overload resolution!
[[nodiscard]] inline auto error(Value message) -> Result {
	return {Status::ERROR_MSG, std::move(message)};
}

} // namespace cmd

#endif
