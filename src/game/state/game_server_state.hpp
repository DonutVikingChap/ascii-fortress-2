#ifndef AF2_STATE_GAME_SERVER_STATE_HPP
#define AF2_STATE_GAME_SERVER_STATE_HPP

#include "game_state.hpp"

class Game;

class GameServerState final : public GameState {
public:
	explicit GameServerState(Game& game);

	[[nodiscard]] auto init() -> bool override;

	auto handleEvent(const SDL_Event& e, const CharWindow& charWindow) -> void override;

	auto update(float deltaTime) -> void override;

	auto draw(CharWindow& charWindow) -> void override;

private:
	Game& m_game;
};

#endif
