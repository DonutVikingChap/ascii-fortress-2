#include "game_server_state.hpp"

#include "../game.hpp"               // Game
#include "../server/game_server.hpp" // GameServer

GameServerState::GameServerState(Game& game)
	: m_game(game) {}

auto GameServerState::init() -> bool {
	return m_game.startGameServer();
}

auto GameServerState::handleEvent(const SDL_Event&, const CharWindow&) -> void {}

auto GameServerState::update(float deltaTime) -> void {
	if (!m_game.gameServer()->update(deltaTime)) {
		m_game.reset();
	}
}

auto GameServerState::draw(CharWindow&) -> void {}
