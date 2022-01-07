#ifndef AF2_NETWORK_DELTA_HPP
#define AF2_NETWORK_DELTA_HPP

#include "../debug.hpp"             // Msg, DEBUG_MSG, DEBUG_TYPE_NAME, DEBUG_TYPE_NAME_ONLY
#include "../utilities/integer.hpp" // util::uint_t, util::setBit, util::checkBit
#include "../utilities/span.hpp"    // util::Span
#include "../utilities/tuple.hpp"   // util::forEach, util::binaryForEach
#include "message.hpp"              // net::TieInputStreamableBase, net::TieOutputStreamableBase

#include <algorithm>   // std::min
#include <cstddef>     // std::size_t
#include <cstdint>     // std::uint16_t, std::uint8_t
#include <limits>      // std::numeric_limits
#include <tuple>       // std::tuple, std::tuple_size_v
#include <type_traits> // std::remove_reference_t
#include <utility>     // std::forward
#include <vector>      // std::vector

namespace net {

template <typename Stream, typename T>
constexpr auto deltaCompress(Stream& stream, const T&, const T& b) -> void {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-compressing regular {} (full write).", DEBUG_TYPE_NAME_ONLY(T)) { //
		stream << b;
	}
}

template <typename Stream, typename T>
[[nodiscard]] constexpr auto deltaDecompress(Stream& stream, T& a) -> bool {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-decompressing regular {} (full read).", DEBUG_TYPE_NAME_ONLY(T)) { //
		return static_cast<bool>(stream >> a);
	}
}

template <typename Stream, typename T, std::size_t N>
constexpr auto deltaCompress(Stream& stream, util::Span<T, N> a, util::Span<T, N> b) -> void {
	static_assert(N != util::DYNAMIC_SIZE);
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-compressing util::Span<{}, {}>.", DEBUG_TYPE_NAME_ONLY(T), N) {
		using Mask = util::uint_t<N>;

		const auto maskIndex = stream.size();

		auto mask = Mask{};
		stream << mask;
		for (auto i = std::size_t{0}; i < N; ++i) {
			if (a[i] != b[i]) {
				DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} changed ({}).", i, DEBUG_TYPE_NAME(T));
				mask = util::setBit(mask, static_cast<Mask>(i));
				deltaCompress(stream, a[i], b[i]);
			} else {
				DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} didn't change ({}).", i, DEBUG_TYPE_NAME(T));
			}
		}
		stream.replace(maskIndex, mask);
	}
}

template <typename Stream, typename T, std::size_t N>
[[nodiscard]] constexpr auto deltaDecompress(Stream& stream, util::Span<T, N> a) -> bool {
	static_assert(N != util::DYNAMIC_SIZE);
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-decompressing util::Span<{}, {}>.", DEBUG_TYPE_NAME_ONLY(T), N) {
		using Mask = util::uint_t<N>;
		if (auto mask = Mask{}; stream >> mask) {
			if (mask == Mask{}) {
				return true;
			}

			for (auto i = std::size_t{0}; i < N; ++i) {
				if (util::checkBit(mask, static_cast<Mask>(i))) {
					DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Element #{} changed ({}).", i, DEBUG_TYPE_NAME(T)) {
						if (!deltaDecompress(stream, a[i])) {
							DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to delta-decompress tuple element #{}!", i);
							return false;
						}
					}
				} else {
					DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} didn't change ({}).", i, DEBUG_TYPE_NAME(T));
				}
			}
			return true;
		}
	}
	return false;
}

template <typename Stream, typename T>
constexpr auto deltaCompress(Stream& stream, const std::vector<T>& a, const std::vector<T>& b) -> void {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-compressing std::vector<{}>.", DEBUG_TYPE_NAME_ONLY(T)) {
		stream << static_cast<std::uint16_t>(std::min(b.size(), static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max())));
		if (!b.empty()) {
			auto maskIndex = stream.size();
			auto mask = std::uint8_t{};
			stream << mask;
			auto i = std::size_t{0};
			auto maskBit = std::uint8_t{0};
			while (i < b.size()) {
				if (maskBit == 8) {
					maskBit = 0;
					stream.replace(maskIndex, mask);
					mask = 0;
					maskIndex = stream.size();
					stream << mask;
				}

				const auto& y = b[i];
				if (i < a.size()) {
					const auto& x = a[i];
					if (x != y) {
						mask = util::setBit(mask, maskBit);
						deltaCompress(stream, x, y);
					}
				} else {
					mask = util::setBit(mask, maskBit);
					stream << y;
				}

				++i;
				++maskBit;
			}
			stream.replace(maskIndex, mask);
		}
	}
}

template <typename Stream, typename T>
[[nodiscard]] constexpr auto deltaDecompress(Stream& stream, std::vector<T>& a) -> bool {
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-decompressing std::vector<{}>.", DEBUG_TYPE_NAME_ONLY(T)) {
		if (auto count = std::uint16_t{}; stream >> count) {
			if (count == 0) {
				a.clear();
				return true;
			}

			const auto size = static_cast<std::size_t>(count);
			const auto oldSize = a.size();
			a.resize(size);
			auto i = std::size_t{0};
			auto maskBit = std::uint8_t{0};
			if (auto mask = std::uint8_t{}; stream >> mask) {
				while (i < size) {
					if (maskBit == 8) {
						maskBit = 0;
						if (!(stream >> mask)) {
							DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to read vector mask!");
							return false;
						}
					}

					if (util::checkBit(mask, maskBit)) {
						if (i < oldSize) {
							if (!deltaDecompress(stream, a[i])) {
								DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to decompress element #{}!", i);
								return false;
							}
						} else {
							if (!(stream >> a[i])) {
								DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to decompress element #{}!", i);
								return false;
							}
						}
					}

					++i;
					++maskBit;
				}
				return true;
			}
			DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to read vector mask!");
			return false;
		}
		DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to read vector size!");
		return false;
	}
}

template <typename Stream, typename... Args>
constexpr auto deltaCompress(Stream& stream, const std::tuple<const Args&...>& a, const std::tuple<const Args&...>& b) -> void {
	using Tuple = std::remove_reference_t<decltype(a)>;
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-compressing tuple of {} elements ({}).", std::tuple_size_v<Tuple>, DEBUG_TYPE_NAME(Tuple)) {
		using Mask = util::uint_t<std::tuple_size_v<Tuple>>;

		const auto maskIndex = stream.size();

		auto mask = Mask{};
		stream << mask;
		util::binaryForEach(std::forward<decltype(a)>(a), std::forward<decltype(b)>(b), [&](auto& x, auto& y, std::size_t i) {
			using T [[maybe_unused]] = std::remove_reference_t<decltype(x)>;
			if (x != y) {
				DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} changed ({}).", i, DEBUG_TYPE_NAME(T));
				mask = util::setBit(mask, static_cast<Mask>(i));
				deltaCompress(stream, x, y);
			} else {
				DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} didn't change ({}).", i, DEBUG_TYPE_NAME(T));
			}
		});
		stream.replace(maskIndex, mask);
	}
}

template <typename Stream, typename... Args>
[[nodiscard]] constexpr auto deltaDecompress(Stream& stream, std::tuple<Args&...> a) -> bool {
	using Tuple = std::remove_reference_t<decltype(a)>;
	DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-decompressing tuple of {} elements ({}).", std::tuple_size_v<Tuple>, DEBUG_TYPE_NAME(Tuple)) {
		using Mask = util::uint_t<std::tuple_size_v<Tuple>>;
		if (auto mask = Mask{}; stream >> mask) {
			if (mask == Mask{}) {
				return true;
			}

			auto success = true;
			util::forEach(std::forward<decltype(a)>(a), [&](auto& x, std::size_t i) {
				if (success) {
					using T [[maybe_unused]] = std::remove_reference_t<decltype(x)>;
					if (util::checkBit(mask, static_cast<Mask>(i))) {
						DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Element #{} changed ({}).", i, DEBUG_TYPE_NAME(T)) {
							if (!deltaDecompress(stream, x)) {
								DEBUG_MSG(Msg::CONNECTION_DELTA, "Failed to delta-decompress tuple element #{}!", i);
								success = false;
							}
						}
					} else {
						DEBUG_MSG(Msg::CONNECTION_DELTA, "Element #{} didn't change ({}).", i, DEBUG_TYPE_NAME(T));
					}
				}
			});
			return success;
		}
	}
	return false;
}

template <typename T>
struct TieEqualityComparableBase {
	[[nodiscard]] friend constexpr auto operator==(const T& a, const T& b) -> bool {
		return a.tie() == b.tie();
	}

	[[nodiscard]] friend constexpr auto operator!=(const T& a, const T& b) -> bool {
		return a.tie() != b.tie();
	}
};

template <typename T>
struct TieDeltaCompressableBase
	: TieOutputStreamableBase<T>
	, TieEqualityComparableBase<T> {
	template <typename Stream>
	friend constexpr auto deltaCompress(Stream& stream, const T& a, const T& b) -> void {
		DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-compressing {}.", DEBUG_TYPE_NAME_ONLY(T)) { //
			deltaCompress(stream, a.tie(), b.tie());
		}
	}
};

template <typename T>
struct TieDeltaDecompressableBase
	: TieInputStreamableBase<T>
	, TieEqualityComparableBase<T> {
	template <typename Stream>
	[[nodiscard]] friend constexpr auto deltaDecompress(Stream& stream, T& a) -> bool {
		DEBUG_MSG_INDENT(Msg::CONNECTION_DELTA, "Delta-decompressing {}.", DEBUG_TYPE_NAME_ONLY(T)) { //
			return deltaDecompress(stream, a.tie());
		}
	}
};

template <typename T>
struct TieDeltaCompressableDecompressableBase
	: TieDeltaCompressableBase<T>
	, TieDeltaDecompressableBase<T> {};

} // namespace net

#endif
