#include "cmd_slider.hpp"

#include "../console/process.hpp"         // Process
#include "../console/script.hpp"          // Script
#include "../console/virtual_machine.hpp" // VirtualMachine
#include "../game/game.hpp"               // Game

#include <utility> // std::move

namespace gui {

CmdSlider::CmdSlider(Vec2 position, Vec2 size, Color color, float value, float delta, Game& game, VirtualMachine& vm,
                     std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command)
	: Slider(position, size, color, value, delta,
             [&game, &vm, env = std::move(env), process = std::move(process), script = Script::parse(command)](Slider&) {
				 if (const auto frame = process->call(std::make_shared<Environment>(env), script)) {
					 vm.output(frame->run(game, game.gameServer(), game.gameClient(), game.metaServer(), game.metaClient()));
				 }
			 }) {}

} // namespace gui
