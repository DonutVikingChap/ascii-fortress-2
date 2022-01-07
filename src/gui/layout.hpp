#ifndef AF2_GUI_LAYOUT_HPP
#define AF2_GUI_LAYOUT_HPP

#include "../game/data/vector.hpp" // Vec2

namespace gui {

static constexpr auto GRID_SIZE_X = Vec2::Length{55};
static constexpr auto GRID_SIZE_Y = Vec2::Length{43};
static constexpr auto VIEWPORT_X = Vec2::Length{1};
static constexpr auto VIEWPORT_Y = Vec2::Length{1};
static constexpr auto VIEWPORT_W = Vec2::Length{GRID_SIZE_X - 2};
static constexpr auto VIEWPORT_H = Vec2::Length{28};
static constexpr auto CONSOLE_INPUT_W = Vec2::Length{GRID_SIZE_X};
static constexpr auto CONSOLE_INPUT_H = Vec2::Length{3};
static constexpr auto CONSOLE_INPUT_X = Vec2::Length{0};
static constexpr auto CONSOLE_INPUT_Y = Vec2::Length{GRID_SIZE_Y - CONSOLE_INPUT_H};
static constexpr auto CONSOLE_X = Vec2::Length{0};
static constexpr auto CONSOLE_Y = Vec2::Length{VIEWPORT_Y + VIEWPORT_H};
static constexpr auto CONSOLE_W = Vec2::Length{GRID_SIZE_X};
static constexpr auto CONSOLE_H = Vec2::Length{GRID_SIZE_Y - VIEWPORT_Y - VIEWPORT_H - CONSOLE_INPUT_H + 1};

} // namespace gui

#endif
