#ifndef AF2_UTILITIES_TUPLE_HPP
#define AF2_UTILITIES_TUPLE_HPP

#include <cstddef>     // std::size_t
#include <tuple>       // std::get, std::tuple_size_v, std::apply, std::tie
#include <type_traits> // std::remove_reference_t
#include <utility>     // std::forward, std::index_sequence, std::make_index_sequence

namespace util {
namespace detail {

template <typename Tuple, typename Func, std::size_t... INDICES>
constexpr auto forEachImpl(Tuple&& a, Func&& func, std::index_sequence<INDICES...>) -> void {
	(static_cast<void>(func(std::get<INDICES>(std::forward<Tuple>(a)), INDICES)), ...);
}

template <typename Tuple, typename Func>
constexpr auto forEachImpl(Tuple&&, Func&&, std::index_sequence<>) -> void {}

template <typename Tuple, typename Func, std::size_t... INDICES>
constexpr auto binaryForEachImpl(Tuple&& a, Tuple&& b, Func&& func, std::index_sequence<INDICES...>) -> void {
	(static_cast<void>(func(std::get<INDICES>(std::forward<Tuple>(a)), std::get<INDICES>(std::forward<Tuple>(b)), INDICES)), ...);
}

template <typename Tuple, typename Func>
constexpr auto binaryForEachImpl(Tuple&&, Tuple&&, Func&&, std::index_sequence<>) -> void {}

} // namespace detail

template <typename... Args>
[[nodiscard]] constexpr auto ctie(const Args&... args) noexcept {
	return std::tie(args...);
}

template <typename Tuple>
[[nodiscard]] constexpr auto constTuple(const Tuple& t) noexcept {
	return std::apply([](const auto&... args) { return std::tie(args...); }, t);
}

template <typename Tuple, typename Func>
constexpr auto forEach(Tuple&& a, Func&& func) -> void {
	using Sequence = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>;
	detail::forEachImpl(std::forward<Tuple>(a), std::forward<Func>(func), Sequence{});
}

template <typename Tuple, typename Func>
constexpr auto binaryForEach(Tuple&& a, Tuple&& b, Func&& func) -> void {
	using Sequence = std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<Tuple>>>;
	detail::binaryForEachImpl(std::forward<Tuple>(a), std::forward<Tuple>(b), std::forward<Func>(func), Sequence{});
}

} // namespace util

#endif
