#ifndef AF2_UTILITIES_OVERLOADED_HPP
#define AF2_UTILITIES_OVERLOADED_HPP

namespace util {

template <typename... Functors>
struct Overloaded : Functors... {
	using Functors::operator()...;
};

template <typename... Functors>
Overloaded(Functors...) -> Overloaded<Functors...>;

} // namespace util

#endif
