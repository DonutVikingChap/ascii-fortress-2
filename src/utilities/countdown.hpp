#ifndef AF2_UTILITIES_COUNTDOWN_HPP
#define AF2_UTILITIES_COUNTDOWN_HPP

#include <algorithm>   // std::min
#include <type_traits> // std::enable_if_t, std::is_..._v
#include <utility>     // std::forward, std::pair, std::make_pair

namespace util {

template <typename Duration>
class Stopwatch final {
public:
	constexpr Stopwatch() noexcept = default;

	constexpr explicit Stopwatch(Duration timeElapsed) noexcept
		: m_timeElapsed(timeElapsed) {}

	[[nodiscard]] constexpr auto getElapsedTime() const noexcept -> Duration {
		return m_timeElapsed;
	}

	constexpr auto addElapsedTime(Duration time) noexcept -> void {
		m_timeElapsed += time;
	}

	constexpr auto setElapsedTime(Duration timeElapsed) noexcept -> void {
		m_timeElapsed = timeElapsed;
	}

	constexpr auto reset() noexcept -> void {
		m_timeElapsed = Duration{};
	}

	constexpr auto advance(Duration deltaTime) noexcept -> void {
		m_timeElapsed += deltaTime;
	}

private:
	Duration m_timeElapsed{};
};

template <typename Duration>
class Countup final {
public:
	constexpr Countup() noexcept = default;

	constexpr explicit Countup(Duration timeElapsed) noexcept
		: m_timeElapsed(timeElapsed) {}

	[[nodiscard]] constexpr auto getElapsedTime() const noexcept -> Duration {
		return m_timeElapsed;
	}

	constexpr auto addElapsedTime(Duration time) noexcept -> void {
		m_timeElapsed += time;
	}

	constexpr auto setElapsedTime(Duration timeElapsed) noexcept -> void {
		m_timeElapsed = timeElapsed;
	}

	constexpr auto reset() noexcept -> void {
		m_timeElapsed = Duration{};
	}

	constexpr auto advance(Duration deltaTime, Duration duration) noexcept -> std::pair<bool, Duration> {
		if (m_timeElapsed < duration) {
			m_timeElapsed += deltaTime;
			return std::make_pair(!(m_timeElapsed < duration), duration - m_timeElapsed);
		}
		m_timeElapsed += deltaTime;
		return std::make_pair(false, duration - m_timeElapsed);
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration duration) const noexcept -> std::pair<bool, Duration> {
		auto t = m_timeElapsed;
		if (t < duration) {
			t += deltaTime;
			return std::make_pair(!(t < duration), duration - t);
		}
		t += deltaTime;
		return std::make_pair(false, duration - t);
	}

private:
	Duration m_timeElapsed{};
};

template <typename Duration>
class CountupLoop final {
public:
	constexpr CountupLoop() noexcept = default;

	constexpr explicit CountupLoop(Duration timeElapsed) noexcept
		: m_timeElapsed(timeElapsed) {}

	[[nodiscard]] constexpr auto getElapsedTime() const noexcept -> Duration {
		return m_timeElapsed;
	}

	constexpr auto addElapsedTime(Duration time) noexcept -> void {
		m_timeElapsed += time;
	}

	constexpr auto setElapsedTime(Duration timeElapsed) noexcept -> void {
		m_timeElapsed = timeElapsed;
	}

	constexpr auto reset() noexcept -> void {
		m_timeElapsed = Duration{};
	}

	constexpr auto advance(Duration deltaTime, Duration interval) noexcept -> int {
		return this->advance(deltaTime, interval, [] { return true; });
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval) const noexcept -> int {
		return this->ticks(deltaTime, interval, [] { return true; });
	}

	constexpr auto advance(Duration deltaTime, Duration interval, bool active) noexcept -> int {
		if (active) {
			return this->advance(deltaTime, interval);
		}
		this->reset();
		return 0;
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active) const noexcept -> int {
		return (active) ? this->ticks(deltaTime, interval) : 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	constexpr auto advance(Duration deltaTime, Duration interval, Condition&& cond) noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		if (interval <= Duration{}) {
			return (cond()) ? 1 : 0;
		}

		auto loops = 0;
		m_timeElapsed += deltaTime;
		while (m_timeElapsed >= interval && cond()) {
			m_timeElapsed -= interval;
			++loops;
		}
		return loops;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, Condition&& cond) const
		noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		if (interval <= Duration{}) {
			return (cond()) ? 1 : 0;
		}

		auto loops = 0;

		auto t = m_timeElapsed;
		t += deltaTime;
		while (t >= interval && cond()) {
			t -= interval;
			++loops;
		}
		return loops;
	}

	constexpr auto advance(Duration deltaTime, Duration interval, int maxLoops) noexcept -> int {
		auto loops = 0;
		return this->advance(deltaTime, interval, [&] { return loops++ < maxLoops; });
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, int maxLoops) const noexcept -> int {
		auto loops = 0;
		return this->ticks(deltaTime, interval, [&] { return loops++ < maxLoops; });
	}

	constexpr auto advance(Duration deltaTime, Duration interval, bool active, int maxLoops) noexcept -> int {
		if (active) {
			return this->advance(deltaTime, interval, maxLoops);
		}
		this->reset();
		return 0;
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active, int maxLoops) const noexcept -> int {
		return (active) ? this->ticks(deltaTime, interval, maxLoops) : 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	constexpr auto advance(Duration deltaTime, Duration interval, bool active, Condition&& cond) noexcept(std::is_nothrow_invocable_v<Condition>)
		-> int {
		if (active) {
			return this->advance(deltaTime, interval, std::forward<Condition>(cond));
		}
		this->reset();
		return 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active, Condition&& cond) const
		noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		return (active) ? this->ticks(deltaTime, interval, std::forward<Condition>(cond)) : 0;
	}

private:
	Duration m_timeElapsed{};
};

template <typename Duration>
class Countdown final {
public:
	constexpr Countdown() noexcept = default;

	constexpr explicit Countdown(Duration timeLeft) noexcept
		: m_timeLeft(timeLeft) {}

	[[nodiscard]] constexpr auto getTimeLeft() const noexcept -> Duration {
		return m_timeLeft;
	}

	constexpr auto addTimeLeft(Duration time) noexcept -> void {
		m_timeLeft += time;
	}

	constexpr auto start(Duration timeLeft) noexcept -> void {
		m_timeLeft = timeLeft;
	}

	constexpr auto reset() noexcept -> void {
		m_timeLeft = Duration{};
	}

	constexpr auto advance(Duration deltaTime) noexcept -> std::pair<bool, Duration> {
		m_timeLeft -= deltaTime;
		if (m_timeLeft <= Duration{}) {
			const auto timeLeft = m_timeLeft;

			m_timeLeft = Duration{};
			return std::make_pair(true, timeLeft);
		}
		return std::make_pair(false, m_timeLeft);
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime) const noexcept -> std::pair<bool, Duration> {
		auto t = m_timeLeft;
		t -= deltaTime;
		return std::make_pair(t <= Duration{}, t);
	}

	constexpr auto advance(Duration deltaTime, bool active) noexcept -> std::pair<bool, Duration> {
		if (active) {
			return this->advance(deltaTime);
		}
		m_timeLeft -= deltaTime;
		if (m_timeLeft < Duration{}) {
			m_timeLeft = Duration{};
		}
		return std::make_pair(false, m_timeLeft);
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, bool active) const noexcept -> std::pair<bool, Duration> {
		if (active) {
			return this->advance(deltaTime);
		}

		auto t = m_timeLeft;
		t -= deltaTime;
		return std::make_pair(false, (t < Duration{}) ? Duration{} : t);
	}

private:
	Duration m_timeLeft{};
};

template <typename Duration>
class CountdownLoop final {
public:
	constexpr CountdownLoop() noexcept = default;

	constexpr explicit CountdownLoop(Duration timeLeft) noexcept
		: m_timeLeft(timeLeft) {}

	[[nodiscard]] constexpr auto getTimeLeft() const noexcept -> Duration {
		return m_timeLeft;
	}

	constexpr auto addTimeLeft(Duration time) noexcept -> void {
		m_timeLeft += time;
	}

	constexpr auto setTimeLeft(Duration timeLeft) noexcept -> void {
		m_timeLeft = timeLeft;
	}

	constexpr auto reset() noexcept -> void {
		m_timeLeft = Duration{};
	}

	constexpr auto advance(Duration deltaTime, Duration interval) noexcept -> int {
		return this->advance(deltaTime, interval, [] { return true; });
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval) const noexcept -> int {
		return this->ticks(deltaTime, interval, [] { return true; });
	}

	constexpr auto advance(Duration deltaTime, Duration interval, bool active) noexcept -> int {
		if (active) {
			return this->advance(deltaTime, interval);
		}
		this->advanceInactive(deltaTime);
		return 0;
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active) const noexcept -> int {
		return (active) ? this->ticks(deltaTime, interval) : 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	constexpr auto advance(Duration deltaTime, Duration interval, Condition&& cond) noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		if (interval <= Duration{}) {
			return (cond()) ? 1 : 0;
		}

		auto loops = 0;
		m_timeLeft -= deltaTime;
		while (m_timeLeft <= Duration{} && cond()) {
			m_timeLeft += interval;
			++loops;
		}
		return loops;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, Condition&& cond) const
		noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		if (interval <= Duration{}) {
			return 1;
		}

		auto loops = 0;
		auto t = m_timeLeft;
		t -= deltaTime;
		while (t <= Duration{} && cond()) {
			t += interval;
			++loops;
		}
		return loops;
	}

	constexpr auto advance(Duration deltaTime, Duration interval, int maxLoops) noexcept -> int {
		auto loops = 0;
		return this->advance(deltaTime, interval, [&] { return loops++ < maxLoops; });
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, int maxLoops) const noexcept -> int {
		auto loops = 0;
		return this->ticks(deltaTime, interval, [&] { return loops++ < maxLoops; });
	}

	constexpr auto advance(Duration deltaTime, Duration interval, bool active, int maxLoops) noexcept -> int {
		if (active) {
			return this->advance(deltaTime, interval, maxLoops);
		}
		this->advanceInactive(deltaTime);
		return 0;
	}

	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active, int maxLoops) const noexcept -> int {
		return (active) ? this->ticks(deltaTime, interval, maxLoops) : 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	constexpr auto advance(Duration deltaTime, Duration interval, bool active, Condition&& cond) noexcept(std::is_nothrow_invocable_v<Condition>)
		-> int {
		if (active) {
			return this->advance(deltaTime, interval, std::forward<Condition>(cond));
		}
		this->advanceInactive(deltaTime);
		return 0;
	}

	template <typename Condition, typename = std::enable_if_t<std::is_invocable_r_v<bool, Condition>>>
	[[nodiscard]] constexpr auto ticks(Duration deltaTime, Duration interval, bool active, Condition&& cond) const
		noexcept(std::is_nothrow_invocable_v<Condition>) -> int {
		return (active) ? this->ticks(deltaTime, interval, std::forward<Condition>(cond)) : 0;
	}

private:
	constexpr auto advanceInactive(Duration deltaTime) noexcept -> void {
		m_timeLeft -= deltaTime;
		if (m_timeLeft < Duration{}) {
			m_timeLeft = Duration{};
		}
	}

	Duration m_timeLeft{};
};

} // namespace util

#endif
