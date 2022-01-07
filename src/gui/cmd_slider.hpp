#ifndef AF2_GUI_CMD_SLIDER_HPP
#define AF2_GUI_CMD_SLIDER_HPP

#include "slider.hpp"

#include <memory> // std::shared_ptr, std::make_shared

class Game;
class VirtualMachine;
class Process;
struct Environment;

namespace gui {

class CmdSlider final : public Slider {
public:
	CmdSlider(Vec2 position, Vec2 size, Color color, float value, float delta, Game& game, VirtualMachine& vm,
			  std::shared_ptr<Environment> env, std::shared_ptr<Process> process, std::string_view command);
};

} // namespace gui

#endif
