#ifndef AF2_GUI_CMD_CHECKBOX_HPP
#define AF2_GUI_CMD_CHECKBOX_HPP

#include "checkbox.hpp"

#include <memory> // std::shared_ptr, std::make_shared

class Game;
class VirtualMachine;
class Process;
struct Environment;

namespace gui {

class CmdCheckbox final : public Checkbox {
public:
	CmdCheckbox(Vec2 position, Vec2 size, Color color, bool value, Game& game, VirtualMachine& vm, std::shared_ptr<Environment> env,
	            std::shared_ptr<Process> process, std::string_view command);
};

} // namespace gui

#endif
