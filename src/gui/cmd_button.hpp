#ifndef AF2_GUI_CMD_BUTTON_HPP
#define AF2_GUI_CMD_BUTTON_HPP

#include "button.hpp"

#include <memory> // std::shared_ptr, std::make_shared

class Game;
class VirtualMachine;
class Process;
struct Environment;

namespace gui {

class CmdButton final : public Button {
public:
	CmdButton(Vec2 position, Vec2 size, Color color, std::string text, Game& game, VirtualMachine& vm, std::shared_ptr<Environment> env,
			  std::shared_ptr<Process> process, std::string_view command);
};

} // namespace gui

#endif
