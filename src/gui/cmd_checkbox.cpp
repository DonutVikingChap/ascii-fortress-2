#include "cmd_checkbox.hpp"

#include "../console/process.hpp"         // Process
#include "../console/script.hpp"          // Script
#include "../console/virtual_machine.hpp" // VirtualMachine
#include "../game/game.hpp"               // Game

#include <utility> // std::move

namespace gui {

CmdCheckbox::CmdCheckbox(Vec2 position, Vec2 size, Color color, bool value, Game& game, VirtualMachine& vm,
						 std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command)
	: Checkbox(position, size, color, value,
			   [&game, &vm, env = std::move(env), process = std::move(process), script = Script::parse(command)](Checkbox&) {
				   if (const auto frame = process->call(std::make_shared<Environment>(env), script)) {
					   vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
				   }
			   }) {}

} // namespace gui
