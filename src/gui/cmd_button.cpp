#include "cmd_button.hpp"

#include "../console/process.hpp"         // Process
#include "../console/script.hpp"          // Script
#include "../console/virtual_machine.hpp" // VirtualMachine
#include "../game/game.hpp"               // Game

#include <utility> // std::move

namespace gui {

CmdButton::CmdButton(Vec2 position, Vec2 size, Color color, std::string text, Game& game, VirtualMachine& vm,
                     std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command)
	: Button(position, size, color, std::move(text),
             [&game, &vm, env = std::move(env), process = std::move(process), script = Script::parse(command)](Button&) {
				 if (const auto frame = process->call(std::make_shared<Environment>(env), script)) {
					 vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
				 }
			 }) {}

} // namespace gui
