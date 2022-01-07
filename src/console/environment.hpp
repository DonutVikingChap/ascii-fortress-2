#ifndef AF2_CONSOLE_ENVIRONMENT_HPP
#define AF2_CONSOLE_ENVIRONMENT_HPP

#include "command.hpp" // cmd::...
#include "script.hpp"  // Script

#include <memory>        // std::shared_ptr
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <variant>       // std::variant
#include <vector>        // std::vector

struct Environment final {
	struct Variable final {
		std::string value;
	};

	struct Constant final {
		std::string value;
	};

	struct Function final {
		std::vector<std::string> parameters;
		Script body;
	};

	using Array = std::vector<std::string>;
	using Table = std::unordered_map<std::string, std::string>;
	using Object = std::variant<Variable, Constant, Function, Array, Table>;
	using ObjectMap = std::unordered_map<std::string, Object>;
	using AliasMap = std::unordered_map<std::string, Script::Command>;

	static auto arrayString(const Array& arr) -> std::string;
	static auto tableString(const Table& table) -> std::string;

	static auto appendToArray(Array& arr, std::string_view script) -> cmd::Result;
	static auto appendToTable(Table& table, std::string_view script) -> cmd::Result;

	explicit Environment(std::shared_ptr<Environment> parent = nullptr);
	explicit Environment(ObjectMap objects, AliasMap aliases = AliasMap{});
	Environment(std::shared_ptr<Environment> parent, ObjectMap objects, AliasMap aliases = AliasMap{});

	auto reset() noexcept -> void;

	std::shared_ptr<Environment> parent{};
	ObjectMap objects{};
	AliasMap aliases{};
};

#endif
