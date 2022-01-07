#ifndef AF2_UTILITIES_TYPE_LIST_HPP
#define AF2_UTILITIES_TYPE_LIST_HPP

#include <cstddef>     // std::size_t
#include <type_traits> // std::conditional_t, std::is_same_v, std::false_type, std::true_type, std::integral_constant, std::bool_constant

namespace util {

template <typename... Ts>
struct TypeList final {};

template <typename... Ts>
struct is_typelist : std::false_type {};

template <typename... Ts>
struct is_typelist<TypeList<Ts...>> : std::true_type {};

template <typename... Ts>
inline constexpr auto is_typelist_v = is_typelist<Ts...>::value;

template <typename... Ts>
struct typelist_size : std::integral_constant<std::size_t, sizeof...(Ts)> {};

template <typename... Ts>
struct typelist_size<TypeList<Ts...>> : typelist_size<Ts...> {};

template <typename... Ts>
inline constexpr auto typelist_size_v = typelist_size<Ts...>::value;

namespace detail {
template <std::size_t INDEX, typename This, typename... Rest>
struct TypeImpl : TypeImpl<INDEX - 1, Rest...> {};

template <typename This, typename... Rest>
struct TypeImpl<0, This, Rest...> {
	using type = This;
};
} // namespace detail

template <std::size_t INDEX, typename... Ts>
struct typelist_type : detail::TypeImpl<INDEX, Ts...> {};

template <std::size_t INDEX, typename... Ts>
struct typelist_type<INDEX, TypeList<Ts...>> : typelist_type<INDEX, Ts...> {};

template <std::size_t INDEX, typename... Ts>
using typelist_type_t = typename typelist_type<INDEX, Ts...>::type;

namespace detail {
template <typename T, typename This, typename... Rest>
struct ContainsImpl : std::conditional_t<std::is_same_v<T, This>, std::true_type, ContainsImpl<T, Rest...>> {};

template <typename T, typename This>
struct ContainsImpl<T, This> : std::bool_constant<std::is_same_v<T, This>> {};
} // namespace detail

template <typename T, typename... Ts>
struct typelist_contains : detail::ContainsImpl<T, Ts...> {};

template <typename T, typename... Ts>
struct typelist_contains<T, TypeList<Ts...>> : typelist_contains<T, Ts...> {};

template <typename T, typename... Ts>
inline constexpr auto typelist_contains_v = typelist_contains<T, Ts...>::value;

namespace detail {
template <std::size_t INDEX, typename T, typename... Ts>
[[nodiscard]] constexpr auto indexImpl() noexcept -> std::size_t {
	static_assert(typelist_contains_v<T, Ts...>, "TypeList does not contain the given type.");
	if constexpr (std::is_same_v<T, typelist_type_t<INDEX, Ts...>>) {
		return INDEX;
	} else if constexpr (INDEX + 1 < typelist_size_v<Ts...>) {
		return indexImpl<INDEX + 1, T, Ts...>();
	}
}

template <std::size_t INDEX, typename T, typename... Ts>
inline constexpr std::size_t indexImplV = indexImpl<INDEX, T, Ts...>();
} // namespace detail

template <typename T, typename... Ts>
struct typelist_index : std::integral_constant<std::size_t, detail::indexImplV<0, T, Ts...>> {};

template <typename T, typename... Ts>
inline constexpr auto typelist_index_v = typelist_index<T, Ts...>::value;

namespace detail {
template <std::size_t INDEX, typename T, typename... Ts>
[[nodiscard]] constexpr auto rindexImpl() noexcept -> std::size_t {
	static_assert(typelist_contains_v<T, Ts...>, "TypeList does not contain the given type.");
	if constexpr (std::is_same_v<T, typelist_type_t<INDEX, Ts...>>) {
		return INDEX;
	} else if constexpr (INDEX > 0) {
		return rindexImpl<INDEX - 1, T, Ts...>();
	}
}

template <std::size_t INDEX, typename T, typename... Ts>
inline constexpr auto rindexImplV = rindexImpl<INDEX, T, Ts...>();
} // namespace detail

template <typename T, typename... Ts>
struct typelist_rindex : std::integral_constant<std::size_t, detail::rindexImplV<typelist_size_v<Ts...> - 1, T, Ts...>> {};

template <typename T, typename... Ts>
inline constexpr auto typelist_rindex_v = typelist_rindex<T, Ts...>::value;

namespace detail {
template <std::size_t INDEX, typename T, typename... Ts>
[[nodiscard]] constexpr auto countImpl() noexcept -> std::size_t {
	if constexpr (INDEX >= typelist_size_v<Ts...>) {
		return 0;
	} else if constexpr (std::is_same_v<T, typelist_type_t<INDEX, Ts...>>) {
		return 1 + countImpl<INDEX + 1, T, Ts...>();
	} else {
		return countImpl<INDEX + 1, T, Ts...>();
	}
}
} // namespace detail

template <typename T, typename... Ts>
struct typelist_count : std::integral_constant<std::size_t, detail::countImpl<0, T, Ts...>()> {};

template <typename T, typename... Ts>
struct typelist_count<T, TypeList<Ts...>> : typelist_count<T, Ts...> {};

template <typename T, typename... Ts>
inline constexpr auto typelist_count_v = typelist_count<T, Ts...>::value;

template <typename... Ts>
struct typelist_concat {
	using type = TypeList<Ts...>;
};

template <typename... Ts>
struct typelist_concat<TypeList<Ts...>> : typelist_concat<Ts...> {};

template <typename... Ts, typename... Us, typename... Rest>
struct typelist_concat<TypeList<Ts...>, TypeList<Us...>, Rest...> : typelist_concat<TypeList<Ts..., Us...>, Rest...> {};

template <typename... Ts>
using typelist_concat_t = typename typelist_concat<Ts...>::type;

} // namespace util

namespace test {

using List1 = util::TypeList<int, float, bool, char, unsigned>;
using List2 = util::TypeList<int, float, bool>;
using List3 = util::TypeList<int, float, bool, float>;
using List4 = util::TypeList<char, double, unsigned>;
using List5 = util::typelist_concat_t<List2, List4>;
using List6 = util::typelist_concat_t<List1, List2, List4>;

static_assert(std::is_same_v<util::typelist_type_t<3, List1>, char>);
static_assert(util::typelist_size_v<List2> == 3);
static_assert(util::typelist_size_v<List3> == 4);
static_assert(util::typelist_contains_v<float, List2>);
static_assert(!util::typelist_contains_v<double, List2>);
static_assert(util::typelist_index_v<float, List2> == 1);
static_assert(util::typelist_index_v<float, List3> == 1);
static_assert(util::typelist_rindex_v<float, List3> == 3);
static_assert(std::is_same_v<util::typelist_type_t<2, List2>, bool>);
static_assert(std::is_same_v<util::typelist_type_t<4, List5>, double>);
static_assert(std::is_same_v<util::typelist_type_t<4, List6>, unsigned>);
static_assert(std::is_same_v<util::typelist_type_t<6, List6>, float>);
static_assert(std::is_same_v<util::typelist_type_t<9, List6>, double>);
static_assert(util::typelist_count_v<short, List6> == 0);
static_assert(util::typelist_count_v<float, List6> == 2);
static_assert(util::typelist_count_v<double, List6> == 1);
static_assert(util::typelist_count_v<unsigned, List6> == 2);

} // namespace test

#endif
