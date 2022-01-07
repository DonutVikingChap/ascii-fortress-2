#ifndef AF2_UTILITIES_REFERENCE_HPP
#define AF2_UTILITIES_REFERENCE_HPP

#include <functional>  // std::invoke
#include <memory>      // std::addressof
#include <type_traits> // std::invoke_result_t, std::is_..._v
#include <utility>     // std::forward, std::declval

namespace util {
namespace detail {

template <typename T>
auto lvalue(T& x) noexcept -> T& {
	return x;
}

template <typename T>
auto lvalue(T&&) -> void = delete;

} // namespace detail

template <typename T>
class Reference final {
public:
	using type = T;

	template <typename U, typename = decltype(detail::lvalue<T>(std::declval<U>()),
											  std::enable_if_t<!std::is_same_v<Reference, std::remove_cv_t<std::remove_reference_t<U>>>>())>
	constexpr Reference(U&& u) noexcept(noexcept(detail::lvalue<T>(std::forward<U>(u))))
		: m_ptr(std::addressof(detail::lvalue<T>(std::forward<U>(u)))) {}

	operator T&() const noexcept {
		return *m_ptr;
	}

	[[nodiscard]] constexpr auto operator*() const noexcept -> T& {
		return *m_ptr;
	}

	[[nodiscard]] constexpr auto operator->() const noexcept -> T* {
		return m_ptr;
	}

	[[nodiscard]] constexpr auto get() const noexcept -> T* {
		return m_ptr;
	}

	template <typename... Args>
	constexpr auto operator()(Args&&... args) const noexcept(std::is_nothrow_invocable_v<T&, Args...>) -> std::invoke_result_t<T&, Args...> {
		return std::invoke(**this, std::forward<Args>(args)...);
	}

private:
	T* m_ptr;
};

template <typename T>
Reference(T&) -> Reference<T>;

} // namespace util

#endif
