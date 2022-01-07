#include "math_commands.hpp"

#include "../command.hpp"           // cmd::...
#include "../command_utilities.hpp" // cmd::...

#include <algorithm> // std::min, std::max, std::clamp
#include <cmath> // std::pow, std::sqrt, std::sin, std::cos, std::tan, std::asin, std::acos, std::atan, std::atan2, std::abs, std::round, std::floor, std::ceil, std::hypot
#include <cstddef>     // std::size_t
#include <cstdint>     // std::int64_t
#include <optional>    // std::nullopt
#include <string>      // std::string
#include <type_traits> // std::is_integral_v
#include <utility>     // std::move

// clang-format off
ConVarString cvar_e{	"e",	"2.71828182845904523536028747135266249775724709369995",	ConVar::READ_ONLY,	"Constant Euler's number e."};
ConVarString cvar_pi{	"pi",	"3.14159265358979323846264338327950288419716939937510",	ConVar::READ_ONLY,	"Constant number pi."};
// clang-format on

namespace {

template <bool TRY_INTEGER, bool ALLOW_ZERO, typename T, typename Func>
auto unaryOp(ConCommand& self, cmd::CommandView argv, Func func) -> cmd::Result {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	constexpr auto CONSTRAINT = (ALLOW_ZERO) ? cmd::NumberConstraint::NONE : cmd::NumberConstraint::NON_ZERO;

	if constexpr (TRY_INTEGER) {
		auto parseError = cmd::ParseError{};

		const auto x = cmd::parseNumber<std::int64_t, CONSTRAINT>(parseError, argv[1], "right hand operand");
		if (!parseError) {
			return cmd::done(func(x));
		}
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<T, CONSTRAINT>(parseError, argv[1], "right hand operand");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(func(x));
}

template <bool TRY_INTEGER, bool ALLOW_LHS_ZERO, bool ALLOW_RHS_ZERO, typename T, typename Func>
auto binaryOp(ConCommand& self, cmd::CommandView argv, Func func) -> cmd::Result {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	constexpr auto LHS_CONSTRAINT = (ALLOW_LHS_ZERO) ? cmd::NumberConstraint::NONE : cmd::NumberConstraint::NON_ZERO;
	constexpr auto RHS_CONSTRAINT = (ALLOW_RHS_ZERO) ? cmd::NumberConstraint::NONE : cmd::NumberConstraint::NON_ZERO;

	if constexpr (TRY_INTEGER) {
		auto parseError = cmd::ParseError{};

		const auto x = cmd::parseNumber<std::int64_t, LHS_CONSTRAINT>(parseError, argv[1], "left hand operand");
		if (!parseError) {
			const auto y = cmd::parseNumber<std::int64_t, RHS_CONSTRAINT>(parseError, argv[2], "right hand operand");
			if (!parseError) {
				return cmd::done(func(x, y));
			}
		}
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<T, LHS_CONSTRAINT>(parseError, argv[1], "left hand operand");
	const auto y = cmd::parseNumber<T, RHS_CONSTRAINT>(parseError, argv[2], "right hand operand");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return cmd::done(func(x, y));
}

template <bool TRY_INTEGER, bool ALLOW_ZERO, typename T, typename Func>
auto foldOp(ConCommand& self, cmd::CommandView argv, Func func) -> cmd::Result {
	if (argv.size() < 2) {
		return cmd::error(self.getUsage());
	}

	constexpr auto CONSTRAINT = (ALLOW_ZERO) ? cmd::NumberConstraint::NONE : cmd::NumberConstraint::NON_ZERO;

	auto value = T{};
	[[maybe_unused]] auto integerValue = std::int64_t{};
	[[maybe_unused]] auto validInteger = true;

	if constexpr (TRY_INTEGER) {
		auto parseError = cmd::ParseError{};
		const auto x = cmd::parseNumber<std::int64_t, CONSTRAINT>(parseError, argv[1], "argument");
		if (parseError) {
			parseError = cmd::ParseError{};
			const auto x = cmd::parseNumber<T, CONSTRAINT>(parseError, argv[1], "argument");
			if (parseError) {
				return cmd::error("{}: {}", self.getName(), *parseError);
			}
			value = x;
			validInteger = false;
		} else {
			integerValue = x;
			value = static_cast<T>(x);
		}
	} else {
		auto parseError = cmd::ParseError{};
		const auto x = cmd::parseNumber<T, CONSTRAINT>(parseError, argv[1], "argument");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}
		value = x;
	}

	for (const auto& arg : argv.subCommand(2)) {
		if constexpr (TRY_INTEGER) {
			auto parseError = cmd::ParseError{};
			if (validInteger) {
				const auto x = cmd::parseNumber<std::int64_t, CONSTRAINT>(parseError, arg, "argument");
				if (!parseError) {
					integerValue = func(integerValue, x);
					value = func(value, static_cast<T>(x));
					continue;
				}
				validInteger = false;
			}
		}
		auto parseError = cmd::ParseError{};
		const auto x = cmd::parseNumber<T, CONSTRAINT>(parseError, arg, "argument");
		if (parseError) {
			return cmd::error("{}: {}", self.getName(), *parseError);
		}
		value = func(value, x);
	}

	if constexpr (TRY_INTEGER) {
		if (validInteger) {
			return cmd::done(integerValue);
		}
	}
	return cmd::done(value);
}

} // namespace

CON_COMMAND(approx_eq, "<x> <y> <epsilon>", ConCommand::NO_FLAGS, "Is x approximately equal to y?", {}, nullptr) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto epsilon = cmd::parseNumber<double>(parseError, argv[3], "epsilon");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	return binaryOp<false, true, true, double>(self, argv.subCommand(0, argv.size() - 1), [epsilon](auto x, auto y) {
		if (x == 0.0f) {
			return std::abs(y) <= epsilon;
		}
		if (y == 0.0f) {
			return std::abs(x) <= epsilon;
		}
		const auto absX = std::abs(x);
		const auto absY = std::abs(y);
		return std::abs(x - y) <= ((absX < absY) ? absY : absX) * epsilon;
	});
}

CON_COMMAND(eq, "<x> <y>", ConCommand::NO_FLAGS, "Is x equal to y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x == y; });
}

CON_COMMAND(ne, "<x> <y>", ConCommand::NO_FLAGS, "Is x not equal to y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x != y; });
}

CON_COMMAND(lt, "<x> <y>", ConCommand::NO_FLAGS, "Is x less than y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x < y; });
}

CON_COMMAND(gt, "<x> <y>", ConCommand::NO_FLAGS, "Is x greater than y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x > y; });
}

CON_COMMAND(le, "<x> <y>", ConCommand::NO_FLAGS, "Is x less than or equal to y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x <= y; });
}

CON_COMMAND(ge, "<x> <y>", ConCommand::NO_FLAGS, "Is x greater than or equal to y?", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x >= y; });
}

CON_COMMAND(abs, "<x>", ConCommand::NO_FLAGS, "Return the absolute value of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::abs(x); });
}

CON_COMMAND(round, "<x>", ConCommand::NO_FLAGS, "Return x rounded to the nearest integer.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) {
		if constexpr (std::is_integral_v<decltype(x)>) {
			return x;
		} else {
			return std::round(x);
		}
	});
}

CON_COMMAND(floor, "<x>", ConCommand::NO_FLAGS, "Return x rounded down.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) {
		if constexpr (std::is_integral_v<decltype(x)>) {
			return x;
		} else {
			return std::floor(x);
		}
	});
}

CON_COMMAND(ceil, "<x>", ConCommand::NO_FLAGS, "Return x rounded up.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) {
		if constexpr (std::is_integral_v<decltype(x)>) {
			return x;
		} else {
			return std::ceil(x);
		}
	});
}

CON_COMMAND(trunc, "<x>", ConCommand::NO_FLAGS, "Return x rounded toward 0.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) {
		if constexpr (std::is_integral_v<decltype(x)>) {
			return x;
		} else {
			return std::trunc(x);
		}
	});
}

CON_COMMAND(decimal, "<x> <n>", ConCommand::NO_FLAGS, "Round x to n decimal places.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	auto parseError = cmd::ParseError{};

	const auto n = cmd::parseNumber<std::size_t>(parseError, argv[2], "number of decimal places");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	auto str = std::string{argv[1]};
	if (const auto decimalPointPos = str.find('.'); decimalPointPos != std::string::npos) {
		if (n > 0) {
			const auto decimalPos = decimalPointPos + 1;
			if (str.size() - decimalPos > n) {
				str[decimalPointPos + n] = static_cast<char>(static_cast<int>(str[decimalPointPos + n]) + static_cast<int>(str[decimalPos + n] >= '5'));
				str.erase(decimalPos + n, std::string::npos);
				str.erase(str.find_last_not_of('0') + 1, std::string::npos);
				if (str.back() == '.') {
					str.pop_back();
				}
			}
		} else {
			str.erase(decimalPointPos, std::string::npos);
		}
	}
	return cmd::done(std::move(str));
}

CON_COMMAND(neg, "<x>", ConCommand::NO_FLAGS, "Return negative x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return -x; });
}

CON_COMMAND(add, "<x> [y...]", ConCommand::NO_FLAGS, "Return the sum of the arguments.", {}, nullptr) {
	return foldOp<true, true, double>(self, argv, [](auto x, auto y) { return x + y; });
}

CON_COMMAND(sub, "<x> <y>", ConCommand::NO_FLAGS, "Return x minus y.", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return x - y; });
}

CON_COMMAND(mul, "<x> [y...]", ConCommand::NO_FLAGS, "Return the product of the arguments.", {}, nullptr) {
	return foldOp<true, true, double>(self, argv, [](auto x, auto y) { return x * y; });
}

CON_COMMAND(div, "<x> <y>", ConCommand::NO_FLAGS, "Return x over y.", {}, nullptr) {
	return binaryOp<false, true, true, double>(self, argv, [](auto x, auto y) { return x / y; }); // Division by 0 is allowed for floating point.
}

CON_COMMAND(mod, "<x> <y>", ConCommand::NO_FLAGS, "Return x modulo y.", {}, nullptr) {
	// Disallow modulo by 0 since the operation only works on integral types.
	return binaryOp<true, true, false, std::int64_t>(self, argv, [](auto x, auto y) { return x % y; }); // NOLINT: y is checked to not be 0
}

CON_COMMAND(pow, "<x> <y>", ConCommand::NO_FLAGS, "Return x to the power of y.", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return std::pow(x, y); });
}

CON_COMMAND(sqrt, "<x>", ConCommand::NO_FLAGS, "Return the square root of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::sqrt(x); });
}

CON_COMMAND(squared, "<x>", ConCommand::NO_FLAGS, "Return x multiplied by itself.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return x * x; });
}

CON_COMMAND(hypot, "<x> <y>", ConCommand::NO_FLAGS, "Return the length of the hypotenuse of a right-angled triangle with the legs x and y.", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto x, auto y) { return std::hypot(x, y); });
}

CON_COMMAND(sin, "<x>", ConCommand::NO_FLAGS, "Return the sine of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::sin(x); });
}

CON_COMMAND(cos, "<x>", ConCommand::NO_FLAGS, "Return the cosine of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::cos(x); });
}

CON_COMMAND(tan, "<x>", ConCommand::NO_FLAGS, "Return the tangent of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::tan(x); });
}

CON_COMMAND(asin, "<x>", ConCommand::NO_FLAGS, "Return the inverse sine of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::asin(x); });
}

CON_COMMAND(acos, "<x>", ConCommand::NO_FLAGS, "Return the inverse cosine of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::acos(x); });
}

CON_COMMAND(atan, "<x>", ConCommand::NO_FLAGS, "Return the inverse tangent of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::atan(x); });
}

CON_COMMAND(atan2, "<y> <x>", ConCommand::NO_FLAGS, "Return the inverse tangent of y/x with respect to their signs.", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto y, auto x) { return std::atan2(y, x); });
}

CON_COMMAND(max, "<x> [y...]", ConCommand::NO_FLAGS, "Return the largest of the arguments.", {}, nullptr) {
	return foldOp<true, true, double>(self, argv, [](auto x, auto y) { return std::max(x, y); });
}

CON_COMMAND(min, "<x> [y...]", ConCommand::NO_FLAGS, "Return the smallest of the arguments.", {}, nullptr) {
	return foldOp<true, true, double>(self, argv, [](auto x, auto y) { return std::min(x, y); });
}

CON_COMMAND(clamp, "<x> <low> <high>", ConCommand::NO_FLAGS, "Return x clamped between low and high.", {}, nullptr) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	{
		auto parseError = cmd::ParseError{};

		const auto x = cmd::parseNumber<std::int64_t>(parseError, argv[1], "value");
		if (!parseError) {
			const auto low = cmd::parseNumber<std::int64_t>(parseError, argv[2], "lower limit");
			if (!parseError) {
				const auto high = cmd::parseNumber<std::int64_t>(parseError, argv[3], "upper limit");
				if (!parseError) {
					if (low > high) {
						return cmd::error("{}: Lower limit must not be higher than the upper limit.", self.getName());
					}
					return cmd::done(std::clamp(x, low, high));
				}
			}
		}
	}

	auto parseError = cmd::ParseError{};

	const auto x = cmd::parseNumber<double>(parseError, argv[1], "value");
	const auto low = cmd::parseNumber<double>(parseError, argv[2], "lower limit");
	const auto high = cmd::parseNumber<double>(parseError, argv[3], "upper limit");
	if (parseError) {
		return cmd::error("{}: {}", self.getName(), *parseError);
	}

	if (low > high) {
		return cmd::error("{}: Lower limit must not be higher than the upper limit.", self.getName());
	}

	return cmd::done(std::clamp(x, low, high));
}

CON_COMMAND(log, "<x>", ConCommand::NO_FLAGS, "Return the base 10 logarithm of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::log10(x); });
}

CON_COMMAND(ln, "<x>", ConCommand::NO_FLAGS, "Return the base e logarithm of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::log(x); });
}

CON_COMMAND(logn, "<n> <x>", ConCommand::NO_FLAGS, "Return the base n logarithm of x.", {}, nullptr) {
	return binaryOp<true, true, true, double>(self, argv, [](auto n, auto x) { return std::log(x) / std::log(n); });
}

CON_COMMAND(exp, "<x>", ConCommand::NO_FLAGS, "Return e raised to the power of x.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) { return std::exp(x); });
}

CON_COMMAND(sgn, "<x>", ConCommand::NO_FLAGS, "Return -1, 0 or 1 depending on x's sign.", {}, nullptr) {
	return unaryOp<true, true, double>(self, argv, [](auto x) {
		using T = decltype(x);
		if (x == T{0}) {
			return T{0};
		}
		return (x < T{0}) ? T{-1} : T{1};
	});
}

CON_COMMAND(is_number, "<x>", ConCommand::NO_FLAGS, "Check if a string contains a valid number.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(util::stringTo<double>(argv[1]) != std::nullopt);
}

CON_COMMAND(is_integer, "<x>", ConCommand::NO_FLAGS, "Check if a string contains a valid integer.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	return cmd::done(util::stringTo<std::int64_t>(argv[1]) != std::nullopt);
}
