#include "environment_commands.hpp"

#include "../../utilities/algorithm.hpp" // util::find, util::contains, util::trasnform
#include "../../utilities/match.hpp"     // util::match
#include "../../utilities/string.hpp"    // util::join, util::toString, util::stringTo
#include "../command.hpp"                // cmd::...
#include "../command_options.hpp"        // cmd::...
#include "../command_utilities.hpp"      // cmd::...
#include "../environment.hpp"            // Environment
#include "../process.hpp"                // CallFrameHandle
#include "../script.hpp"                 // Script
#include "../suggestions.hpp"            // Suggestions

#include <algorithm>  // std::sort
#include <any>        // std::any, std::any_cast
#include <array>      // std::array
#include <cstddef>    // std::size_t
#include <cstdint>    // std::int64_t
#include <functional> // std::greater
#include <variant>    // std::get_if
#include <vector>     // std::vector

CON_COMMAND(exists, "<name>", ConCommand::NO_FLAGS, "Check if a local object exists.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	return cmd::done(frame.env()->objects.count(argv[1]) != 0);
}

CON_COMMAND(defined, "<name>", ConCommand::NO_FLAGS, "Check if a name is defined.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	return cmd::done(frame.process()->defined(frame.env(), argv[1]));
}

CON_COMMAND(type, "<name>", ConCommand::NO_FLAGS, "Get the type of an object (var/function/table).", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		return util::match(*obj)([&](const Environment::Variable&) { return cmd::done("var"); },
								 [&](const Environment::Constant&) { return cmd::done("const"); },
								 [&](const Environment::Function&) { return cmd::done("function"); },
								 [&](const Environment::Array&) { return cmd::done("array"); },
								 [&](const Environment::Table&) { return cmd::done("table"); });
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(var, "<name> [value]", ConCommand::NO_FLAGS, "Create a local variable.", {}, nullptr) {
	if (argv.size() < 2 || argv.size() > 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[1], Environment::Variable{(argv.size() == 3) ? argv[2] : std::string{}});
		!inserted) {
		return util::match(it->second)(
			[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Function&) { return cmd::error("{}: A function named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), argv[1]); });
	}
	return cmd::done();
}

CON_COMMAND(const, "<name> [value]", ConCommand::NO_FLAGS, "Create a local constant.", {}, nullptr) {
	if (argv.size() < 2 || argv.size() > 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[1], Environment::Constant{(argv.size() == 3) ? argv[2] : std::string{}});
		!inserted) {
		return util::match(it->second)(
			[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Function&) { return cmd::error("{}: A function named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), argv[1]); });
	}
	return cmd::done();
}

CON_COMMAND(enum, "<names>", ConCommand::NO_FLAGS,
			"Create an enumeration of local constants with values starting at 0 and increasing by 1 for each variable.", {}, nullptr) {
	struct State final {
		Script commands;
		int value = 0;
		std::size_t i = 0;
	};

	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	State* state = nullptr;
	if (frame.progress() == 0) {
		state = &data.emplace<State>();
		state->commands = Script::parse(argv[1]);
		if (state->commands.empty()) {
			return cmd::error("{}: Empty enum!", self.getName());
		}
	} else {
		state = &std::any_cast<State&>(data);
		util::stringTo<int>(state->value, argv[1]);
	}

	assert(state);
	while (state->i < state->commands.size()) {
		auto& command = state->commands[state->i];
		assert(!command.empty());
		if (command.size() > 2) {
			return cmd::error("{}: Invalid enum syntax \"{}\".", self.getName(), Script::commandString(command));
		}
		if (command.size() == 2) {
			if ((command[1].flags & Script::Argument::EXEC) != 0) {
				frame.arguments()[1].reset();
				if (!frame.call(1, frame.env(), std::move(command[1].value))) {
					return cmd::error("{}: Stack overflow.", self.getName());
				}
				command.pop_back();
				return cmd::notDone(1);
			}

			const auto str = Script::argumentString(command[1]);

			auto parseError = cmd::ParseError{};

			state->value = cmd::parseNumber<int>(parseError, str, "enum value");
			if (parseError) {
				return cmd::error("{}: {}", self.getName(), *parseError);
			}
		}

		assert(!command.empty());
		if (command.front().flags != Script::Argument::NO_FLAGS) {
			return cmd::error("{}: Invalid enum syntax \"{}\".", self.getName(), Script::argumentString(command.front()));
		}

		const auto& name = command.front().value;
		if (const auto [it, inserted] = frame.env()->objects.try_emplace(name, Environment::Constant{util::toString(state->value)}); !inserted) {
			return util::match(it->second)(
				[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), name); },
				[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), name); },
				[&](const Environment::Function&) { return cmd::error("{}: A function named {} already exists.", self.getName(), name); },
				[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), name); },
				[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), name); });
		}
		++state->value;
		++state->i;
	}
	return cmd::done();
}

CON_COMMAND(set, "<name> <value>", ConCommand::NO_FLAGS,
			"Create a local variable or overwrite if a local object with the same name already exists.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	auto var = Environment::Variable{argv[2]};
	frame.env()->objects.insert_or_assign(argv[1], std::move(var));
	return cmd::done();
}

CON_COMMAND(function, "<name> [arguments...] <script>", ConCommand::NO_FLAGS,
			"Create a local function. Overwrites if a local function with the same name already exists.", {}, nullptr) {
	if (argv.size() < 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	auto function = Environment::Function{};
	function.parameters.insert(function.parameters.end(), argv.begin() + 2, argv.end() - 1);
	function.body = Script::parse(argv[argv.size() - 1]);

	if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[1], std::move(function)); !inserted) {
		return util::match(it->second)(
			[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), argv[1]); },
			[&](Environment::Function& func) {
				func = std::move(function);
				return cmd::done();
			},
			[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), argv[1]); });
	}
	return cmd::done();
}

CON_COMMAND(array, "<name> [value]", ConCommand::NO_FLAGS, "Create a local array.", {}, nullptr) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[1], Environment::Array{}); inserted) {
		if (argv.size() == 3) {
			return Environment::appendToArray(*std::get_if<Environment::Array>(&it->second), argv[2]);
		}
	} else {
		return util::match(it->second)(
			[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Function&) { return cmd::error("{}: A function named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), argv[1]); });
	}
	return cmd::done();
}

CON_COMMAND(table, "<name> [value]", ConCommand::NO_FLAGS, "Create a local table.", {}, nullptr) {
	if (argv.size() != 2 && argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[1], Environment::Table{}); inserted) {
		if (argv.size() == 3) {
			return Environment::appendToTable(*std::get_if<Environment::Table>(&it->second), argv[2]);
		}
	} else {
		return util::match(it->second)(
			[&](const Environment::Variable&) { return cmd::error("{}: A variable named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Constant&) { return cmd::error("{}: A constant named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Function&) { return cmd::error("{}: A function named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Array&) { return cmd::error("{}: An array named {} already exists.", self.getName(), argv[1]); },
			[&](const Environment::Table&) { return cmd::error("{}: A table named {} already exists.", self.getName(), argv[1]); });
	}
	return cmd::done();
}

CON_COMMAND(args, "<name>", ConCommand::NO_FLAGS, "Get the values in an array as a string of space-separated arguments.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
			return cmd::done(*arr | util::transform(Script::escapedString) | util::join(' '));
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(invoke, "<name> <args>", ConCommand::NO_FLAGS, "Invoke a command using arguments in an array.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[2])) {
		if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
			auto command = Script::Command{};
			command.reserve(arr->size() + 1);
			command.emplace_back(argv[1]);
			for (const auto& elem : *arr) {
				command.emplace_back(elem);
			}
			if (!frame.tailCall(frame.env(), std::move(command))) {
				return cmd::error("{}: Stack overflow.", self.getName());
			}
			return cmd::done();
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[2]);
	}
	return cmd::error("{}: Couldn't find object \"{}\".", self.getName(), argv[2]);
}

CON_COMMAND(sort, "<name>", ConCommand::NO_FLAGS, "Sort an array.",
			cmd::opts(cmd::opt('a', "ascending", "Sort in ascending order."),
					  cmd::opt('n', "numeric", "Sort numerically rather than alphabetically.")),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), std::string{args[0]})) {
		if (auto* const arr = std::get_if<Environment::Array>(obj)) {
			if (options['n']) {
				static constexpr auto compare = [](const auto& lhs, const auto& rhs) {
					const auto a = util::stringTo<std::int64_t>(lhs);
					const auto b = util::stringTo<std::int64_t>(rhs);
					if (a && b) {
						return *a < *b;
					}

					const auto af = util::stringTo<double>(lhs);
					const auto bf = util::stringTo<double>(rhs);
					if (af && bf) {
						return *af < *bf;
					}

					return lhs < rhs;
				};

				if (options['a']) {
					std::sort(arr->begin(), arr->end(), [&](const auto& lhs, const auto& rhs) { return compare(rhs, lhs); });
				} else {
					std::sort(arr->begin(), arr->end(), compare);
				}
			} else {
				if (options['a']) {
					std::sort(arr->begin(), arr->end(), std::greater{});
				} else {
					std::sort(arr->begin(), arr->end());
				}
			}
			return cmd::done();
		}
		return cmd::error("{}: {} is not an array.", self.getName(), args[0]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), args[0]);
}

CON_COMMAND(foreach, "<parameter> <name> <script>", ConCommand::NO_FLAGS, "Execute script for each key/value in a table/array.", {}, nullptr) {
	struct State final {
		std::vector<std::string> values;
		Script body;
		std::size_t i = 0;
	};

	switch (frame.progress()) {
		case 0: {
			if (argv.size() != 4) {
				return cmd::error(self.getUsage());
			}

			if (const auto* const obj = frame.process()->findObject(frame.env(), argv[2])) {
				if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
					auto& state = data.emplace<State>();
					state.values = *arr;
					state.body = Script::parse(argv[3]);
					assert(frame.arguments().size() == 4);
					frame.arguments().pop_back();
					frame.arguments()[2].reset();
					return cmd::notDone(1);
				}

				if (const auto* const table = std::get_if<Environment::Table>(obj)) {
					auto& state = data.emplace<State>();
					state.values.reserve(table->size());
					for (const auto& kv : *table) {
						state.values.push_back(kv.first);
					}
					state.body = Script::parse(argv[3]);
					assert(frame.arguments().size() == 4);
					frame.arguments().pop_back();
					frame.arguments()[2].reset();
					return cmd::notDone(1);
				}
				return cmd::error("{}: {} is not a table/array.", self.getName(), argv[2]);
			}
			return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[2]);
		}
		case 1: {
			assert(frame.arguments().size() == 3);
			if (frame.arguments()[2].status == cmd::Status::BREAK) {
				return cmd::done();
			}

			if (frame.arguments()[2].status == cmd::Status::RETURN) {
				return cmd::returned();
			}

			if (frame.arguments()[2].status == cmd::Status::RETURN_VALUE) {
				return cmd::returned(std::move(frame.arguments()[2].value));
			}

			auto& state = std::any_cast<State&>(data);
			if (state.i >= state.values.size()) {
				return cmd::done();
			}

			auto env = std::make_shared<Environment>(frame.env());
			env->objects.try_emplace(argv[1], Environment::Variable{std::move(state.values[state.i])});
			frame.arguments()[2].reset();
			if (const auto bodyFrame = frame.call(2, std::move(env), state.body)) {
				bodyFrame->makeSection();
			} else {
				return cmd::error("{}: Stack overflow.", self.getName());
			}

			++state.i;
			return cmd::notDone(1);
		}
		default: break;
	}
	return cmd::done();
}

CON_COMMAND(find_index, "<name> <x>", ConCommand::NO_FLAGS,
			"Get the array index of x. Returns the size of the array if the value is not found.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
			return cmd::done(util::find(*arr, argv[2]) - arr->begin());
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(contains, "<name> <x>", ConCommand::NO_FLAGS, "Check if a table/array contains x.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
			return cmd::done(util::contains(*arr, argv[2]));
		}
		if (const auto* const table = std::get_if<Environment::Table>(obj)) {
			return cmd::done(table->count(argv[2]) != 0);
		}
		return cmd::error("{}: {} is not a table/array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(size, "<name>", ConCommand::NO_FLAGS, "Get the number of elements in a table/array.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (const auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (const auto* const arr = std::get_if<Environment::Array>(obj)) {
			return cmd::done(arr->size());
		}
		if (const auto* const table = std::get_if<Environment::Table>(obj)) {
			return cmd::done(table->size());
		}
		return cmd::error("{}: {} is not a table/array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(push, "<name> <value>", ConCommand::NO_FLAGS, "Add a value to the end of an array.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (auto* const arr = std::get_if<Environment::Array>(obj)) {
			arr->push_back(argv[2]);
			return cmd::done();
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(pop, "<name>", ConCommand::NO_FLAGS, "Pop the value at the end of an array.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (auto* const arr = std::get_if<Environment::Array>(obj)) {
			if (arr->empty()) {
				return cmd::error("{}: {} is empty.", self.getName(), argv[1]);
			}
			arr->pop_back();
			return cmd::done();
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(insert, "<name> <index> <value>", ConCommand::NO_FLAGS, "Insert a value into an array at a certain index.", {}, nullptr) {
	if (argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		if (auto* const arr = std::get_if<Environment::Array>(obj)) {
			auto parseError = cmd::ParseError{};

			auto index = cmd::parseNumber<int>(parseError, argv[2], "array index");
			if (parseError) {
				return cmd::error("{}: {}", self.getName(), *parseError);
			}

			if (index < 0) {
				index += static_cast<int>(arr->size());
			}

			const auto i = static_cast<std::size_t>(index);
			if (i <= arr->size()) {
				arr->insert(arr->begin() + static_cast<std::ptrdiff_t>(i), argv[3]);
				return cmd::done();
			}
			return cmd::error("{}: Array index out of range ({}/{}).", self.getName(), i, arr->size());
		}
		return cmd::error("{}: {} is not an array.", self.getName(), argv[1]);
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(toggle, "<name> [key]", ConCommand::NO_FLAGS, "Toggle the value of a var/cvar between 0 and 1.", {}, Suggestions::suggestCvar<1>) {
	if (frame.progress() == 0) {
		if (argv.size() != 2 && argv.size() != 3) {
			return cmd::error(self.getUsage());
		}

		if (argv.size() == 3) {
			if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
				if (auto* const arr = std::get_if<Environment::Array>(obj)) {
					if (const auto elemIt = util::find(*arr, argv[2]); elemIt != arr->end()) {
						*elemIt = (*elemIt == "0") ? "1" : "0";
						return cmd::done();
					}
					return cmd::error("{}: {} does not contain \"{}\"", self.getName(), argv[1], argv[2]);
				}

				if (auto* const table = std::get_if<Environment::Table>(obj)) {
					if (const auto kvIt = table->find(argv[2]); kvIt != table->end()) {
						kvIt->second = (kvIt->second == "0") ? "1" : "0";
						return cmd::done();
					}
					return cmd::error("{}: {} does not contain \"{}\".", self.getName(), argv[1], argv[2]);
				}
				return cmd::error("{}: {} is not a table/array.", self.getName(), argv[1]);
			}
			return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
		}

		frame.arguments().push_back(cmd::done());
		if (!frame.call(2, frame.env(), frame.arguments()[1].value)) {
			return cmd::error("{}: Stack overflow.", self.getName());
		}
		return cmd::notDone(1);
	}

	if (!frame.tailCall(frame.env(), Script::command({argv[1], (argv[2] == "0") ? "1" : "0"}))) {
		return cmd::error("{}: Stack overflow.", self.getName());
	}

	return cmd::done();
}

CON_COMMAND(delete, "<name> [key]", ConCommand::NO_FLAGS, "Remove a local object, or an entry from an array/table.", {}, nullptr) {
	if (argv.size() == 2) {
		if (!frame.env()) {
			return cmd::error("{}: No environment!", self.getName());
		}

		if (frame.env()->objects.erase(argv[1]) == 0) {
			return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
		}

		return cmd::done();
	}

	if (argv.size() == 3) {
		if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
			if (auto* const arr = std::get_if<Environment::Array>(obj)) {
				auto parseError = cmd::ParseError{};

				auto index = cmd::parseNumber<int>(parseError, argv[2], "array index");
				if (parseError) {
					return cmd::error("{}: {}", self.getName(), *parseError);
				}

				if (index < 0) {
					index += static_cast<int>(arr->size());
				}

				const auto i = static_cast<std::size_t>(index);
				if (i <= arr->size()) {
					arr->erase(arr->begin() + static_cast<std::ptrdiff_t>(i));
					return cmd::done();
				}
				return cmd::error("{}: Array index out of range ({}/{}).", self.getName(), i, arr->size());
			}

			if (auto* const table = std::get_if<Environment::Table>(obj)) {
				if (table->erase(argv[2]) == 0) {
					return cmd::error("{}: {} does not contain \"{}\".", self.getName(), argv[1], argv[2]);
				}
				return cmd::done();
			}
			return cmd::error("{}: {} is not a table/array.", self.getName(), argv[1]);
		}
		return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
	}
	return cmd::error(self.getUsage());
}

CON_COMMAND(clear, "<name>", ConCommand::NO_FLAGS, "Erase all content from an existing variable, array or table.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		return util::match(*obj)(
			[&](Environment::Variable& var) {
				var.value.clear();
				return cmd::done();
			},
			[&](Environment::Constant&) { return cmd::error("{}: Cannot change the value of a constant.", self.getName()); },
			[&](Environment::Function&) { return cmd::error("{}: Cannot clear a function.", self.getName()); },
			[&](Environment::Array& arr) {
				arr.clear();
				return cmd::done();
			},
			[&](Environment::Table& table) {
				table.clear();
				return cmd::done();
			});
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(assign, "<name> <value>", ConCommand::NO_FLAGS, "Re-initialize an existing variable, array or table.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		return util::match(*obj)(
			[&](Environment::Variable& var) {
				var.value = argv[2];
				return cmd::done();
			},
			[&](Environment::Constant&) { return cmd::error("{}: Cannot change the value of a constant.", self.getName()); },
			[&](Environment::Function&) { return cmd::error("{}: Cannot assign to a function.", self.getName()); },
			[&](Environment::Array& arr) {
				arr.clear();
				return Environment::appendToArray(arr, argv[2]);
			},
			[&](Environment::Table& table) {
				table.clear();
				return Environment::appendToTable(table, argv[2]);
			});
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(append, "<name> <value>", ConCommand::NO_FLAGS, "Append to an existing variable, array or table.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (auto* const obj = frame.process()->findObject(frame.env(), argv[1])) {
		return util::match(*obj)(
			[&](Environment::Variable& var) {
				var.value.append(argv[2]);
				return cmd::done();
			},
			[&](Environment::Constant&) { return cmd::error("{}: Cannot change the value of a constant.", self.getName()); },
			[&](Environment::Function&) { return cmd::error("{}: Cannot append to a function.", self.getName()); },
			[&](Environment::Array& arr) { return Environment::appendToArray(arr, argv[2]); },
			[&](Environment::Table& table) { return Environment::appendToTable(table, argv[2]); });
	}
	return cmd::error("{}: Couldn't find \"{}\".", self.getName(), argv[1]);
}

CON_COMMAND(inline, "<type> <name> [value]", ConCommand::NO_FLAGS, "Create an object if it doesn't already exist.", {}, nullptr) {
	if (argv.size() != 3 && argv.size() != 4) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (argv[1] == "var") {
		frame.env()->objects.try_emplace(argv[2], Environment::Variable{(argv.size() == 4) ? argv[3] : std::string{}});
	} else if (argv[1] == "const") {
		frame.env()->objects.try_emplace(argv[2], Environment::Constant{(argv.size() == 4) ? argv[3] : std::string{}});
	} else if (argv[1] == "array") {
		if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[2], Environment::Array{}); inserted && argv.size() == 4) {
			return Environment::appendToArray(*std::get_if<Environment::Array>(&it->second), argv[3]);
		}
	} else if (argv[1] == "table") {
		if (const auto [it, inserted] = frame.env()->objects.try_emplace(argv[2], Environment::Table{}); inserted && argv.size() == 4) {
			return Environment::appendToTable(*std::get_if<Environment::Table>(&it->second), argv[3]);
		}
	} else {
		return cmd::error("{}: Invalid type \"{}\".", self.getName(), argv[1]);
	}

	return cmd::done();
}

CON_COMMAND(alias, "<name> <command>", ConCommand::NO_FLAGS, "Create a local alias.", {}, nullptr) {
	if (argv.size() != 3) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	auto commands = Script::parse(argv[2]);
	if (commands.empty()) {
		return cmd::error("{}: Empty command!", self.getName());
	}

	if (commands.size() > 1) {
		return cmd::error("{}: An alias may only contain one command.", self.getName());
	}

	frame.env()->aliases[argv[1]] = std::move(commands.front());
	return cmd::done();
}

CON_COMMAND(unalias, "<name>", ConCommand::NO_FLAGS, "Remove a local alias.", {}, nullptr) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (!frame.env()) {
		return cmd::error("{}: No environment!", self.getName());
	}

	if (frame.env()->aliases.erase(argv[1]) == 0) {
		return cmd::error("{}: Couldn't find alias \"{}\".", self.getName(), argv[1]);
	}

	return cmd::done();
}
