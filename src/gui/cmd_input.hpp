#ifndef AF2_GUI_CMD_INPUT_HPP
#define AF2_GUI_CMD_INPUT_HPP

#include "text_input.hpp"

#include <memory> // std::shared_ptr, std::make_shared

class Game;
class VirtualMachine;
class Process;
struct Environment;

namespace gui {

class CmdInput final : public TextInput {
public:
	CmdInput(Vec2 position, Vec2 size, Color color, std::string text, Game& game, VirtualMachine& vm, std::shared_ptr<Environment> env,
			 std::shared_ptr<Process> process, std::string_view command, std::size_t maxLength = std::numeric_limits<std::size_t>::max(),
			 bool isPrivate = false, bool replaceMode = false);
};

} // namespace gui

#endif
