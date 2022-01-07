#ifndef AF2_STATE_GAME_STATE_HPP
#define AF2_STATE_GAME_STATE_HPP

#include <SDL.h> // SDL_...

class CharWindow;

class GameState {
public:
	GameState() noexcept = default;

	virtual ~GameState() = default;

	GameState(const GameState&) noexcept = default;
	GameState(GameState&&) noexcept = default;

	auto operator=(const GameState&) noexcept -> GameState& = default;
	auto operator=(GameState&&) noexcept -> GameState& = default;

	[[nodiscard]] virtual auto init() -> bool = 0;

	virtual auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void = 0;

	virtual auto update(float deltaTime) -> void = 0;

	virtual auto draw(CharWindow& charWindow) -> void = 0;
};

#endif
