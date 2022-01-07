#ifndef AF2_UTILITIES_SCOPE_GUARD_HPP
#define AF2_UTILITIES_SCOPE_GUARD_HPP

#include <utility> // std::forward, std::move

namespace util {

template <typename Function>
class ScopeGuard final {
public:
	template <typename Func>
	[[nodiscard]] constexpr explicit ScopeGuard(Func&& func)
		: m_func(std::forward<Func>(func)) {}

	~ScopeGuard() {
		if (m_active) {
			m_func();
		}
	}

	constexpr ScopeGuard(const ScopeGuard&) = delete;

	constexpr ScopeGuard(ScopeGuard&& other) noexcept
		: m_func(std::move(other.m_func))
		, m_active(other.m_active) {
		other.m_active = false;
	}

	constexpr auto operator=(const ScopeGuard&) -> ScopeGuard& = delete;

	constexpr auto operator=(ScopeGuard&& other) noexcept -> ScopeGuard& {
		m_func = std::move(other.m_func);
		m_active = other.m_active;
		other.m_active = false;
	}

	constexpr auto release() noexcept -> void {
		m_active = false;
	}

	[[nodiscard]] constexpr auto active() const noexcept -> bool {
		return m_active;
	}

private:
	Function m_func;
	bool m_active = true;
};

template <typename Func>
ScopeGuard(Func) -> ScopeGuard<Func>;

} // namespace util

#endif
