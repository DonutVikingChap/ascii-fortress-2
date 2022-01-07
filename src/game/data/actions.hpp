#ifndef AF2_DATA_ACTIONS_HPP
#define AF2_DATA_ACTIONS_HPP

#include <cstdint> // std::uint32_t

using Actions = std::uint32_t;

struct Action final {
	Action() = delete;

	enum : Actions {
		NONE = 0,
		ALL = static_cast<Actions>(~0),

		MOVE_LEFT = 1 << 0,
		MOVE_RIGHT = 1 << 1,
		MOVE_UP = 1 << 2,
		MOVE_DOWN = 1 << 3,

		AIM_LEFT = 1 << 4,
		AIM_RIGHT = 1 << 5,
		AIM_UP = 1 << 6,
		AIM_DOWN = 1 << 7,

		ATTACK1 = 1 << 8,
		ATTACK2 = 1 << 9
	};
};

#endif
