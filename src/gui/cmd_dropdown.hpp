#ifndef AF2_GUI_CMD_DROPDOWN_HPP
#define AF2_GUI_CMD_DROPDOWN_HPP

#include "dropdown.hpp"

#include <memory> // std::shared_ptr, std::make_shared

class Game;
class VirtualMachine;
class Process;
struct Environment;

namespace gui {

class CmdDropdown final : public Dropdown {
public:
	CmdDropdown(Vec2 position, Vec2 size, Color color, std::vector<std::string> options, std::size_t selectedOptionIndex, Game& game,
				VirtualMachine& vm, std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command);
};

} // namespace gui

#endif
