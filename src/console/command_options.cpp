#include "command_options.hpp"

#include "../utilities/algorithm.hpp" // util::subview, util::countIf, util::contains

#include <cassert>    // assert
#include <fmt/core.h> // fmt::format

namespace cmd {

auto isOption(const Value& arg) noexcept -> bool {
	return !arg.empty() && arg.front() == '-';
}

auto isSpecificOption(const Value& arg, char name, std::string_view longName) noexcept -> bool {
	if (arg.size() > 1) {
		if (arg.front() == '-') {
			if (arg[1] == '-') {
				return arg.compare(2, longName.size(), longName) == 0;
			}
			if (util::contains(util::subview(arg, 1), name)) {
				return true;
			}
		}
	}
	return false;
}
auto optc(CommandView argv) noexcept -> std::size_t {
	assert(!argv.empty());
	return util::countIf(util::subview(argv, 1), cmd::isOption);
}

auto argc(CommandView argv, util::Span<const OptionSpec> optionSpecs, std::size_t offset) noexcept -> std::size_t {
	assert(!argv.empty());
	auto result = std::size_t{0};
	for (auto it = argv.begin() + static_cast<std::ptrdiff_t>(offset); it != argv.end(); ++it) {
		if (cmd::isOption(*it)) {
			for (const auto& opt : optionSpecs) {
				if (cmd::isSpecificOption(*it, opt.name, opt.longName)) {
					if (opt.type == OptionType::ARGUMENT_REQUIRED) {
						++it;
					}
					break;
				}
			}
		} else {
			++result;
		}
	}
	return result;
}

auto parse(CommandView argv, util::Span<const OptionSpec> optionSpecs, std::size_t offset) -> std::pair<std::vector<std::string_view>, cmd::Options> {
	assert(offset <= argv.size());
	auto result = std::pair<std::vector<std::string_view>, Options>{};
	for (auto it = argv.begin() + static_cast<std::ptrdiff_t>(offset); it != argv.end(); ++it) {
		if (cmd::isOption(*it)) {
			auto found = false;
			for (const auto& opt : optionSpecs) {
				if (cmd::isSpecificOption(*it, opt.name, opt.longName)) {
					if (opt.type == OptionType::ARGUMENT_REQUIRED) {
						if (++it != argv.end()) {
							result.second.set(opt.name, *it);
						} else {
							result.second.setError(fmt::format("Missing value for option \"{}\".", opt.longName));
						}
					} else {
						result.second.set(opt.name);
					}
					found = true;
					break;
				}
			}

			if (!found) {
				result.second.setError(fmt::format("Unknown option \"{}\".", *it));
			}
		} else {
			result.first.emplace_back(*it);
		}
	}
	return result;
}

} // namespace cmd
