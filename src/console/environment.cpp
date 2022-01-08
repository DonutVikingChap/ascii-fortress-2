#include "environment.hpp"

#include "../utilities/algorithm.hpp" // util::erase, util::transform
#include "../utilities/string.hpp"    // util::join

#include <cassert>    // assert
#include <fmt/core.h> // fmt::format
#include <utility>    // std::move

auto Environment::arrayString(const Array& arr) -> std::string {
	return arr | util::transform(Script::escapedString) | util::join('\n');
}

auto Environment::tableString(const Table& table) -> std::string {
	static constexpr auto formatTableElement = [](const auto& kv) {
		return fmt::format("{} {}", Script::escapedString(kv.first), Script::escapedString(kv.second));
	};

	return table | util::transform(formatTableElement) | util::join('\n');
}

auto Environment::appendToArray(Array& arr, std::string_view script) -> cmd::Result {
	for (const auto& command : Script::parse(script)) {
		arr.push_back(command | util::transform([](const auto& arg) { return arg.value; }) | util::join(' '));
	}
	return cmd::done();
}

auto Environment::appendToTable(Table& table, std::string_view script) -> cmd::Result {
	for (const auto& command : Script::parse(script)) {
		if (command.size() == 1) {
			const auto& key = command.front().value;
			if (!table.try_emplace(key).second) {
				return cmd::error("Multiple initialization of table key \"{}\".", key);
			}
		} else if (command.size() == 2) {
			const auto& key = command.front().value;
			if (!table.try_emplace(key, command[1].value).second) {
				return cmd::error("Multiple initialization of table key \"{}\".", key);
			}
		} else {
			return cmd::error("Invalid table initialization syntax \"{}\". Correct syntax is \"<key>\" or \"<key> <value>\".",
			                  Script::commandString(command));
		}
	}
	return cmd::done();
}

Environment::Environment(std::shared_ptr<Environment> parent)
	: parent(std::move(parent)) {}

Environment::Environment(ObjectMap objects, AliasMap aliases)
	: objects(std::move(objects))
	, aliases(std::move(aliases)) {}

Environment::Environment(std::shared_ptr<Environment> parent, ObjectMap objects, AliasMap aliases)
	: parent(std::move(parent))
	, objects(std::move(objects))
	, aliases(std::move(aliases)) {}

auto Environment::reset() noexcept -> void {
	aliases.clear();
	objects.clear();
}
