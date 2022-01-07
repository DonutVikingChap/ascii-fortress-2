#include "game_client_state.hpp"

#include "../client/game_client.hpp" // GameClient
#include "../game.hpp"               // Game

GameClientState::GameClientState(Game& game)
	: m_game(game) {}

auto GameClientState::init() -> bool {
	return m_game.startGameClient();
}

auto GameClientState::handleEvent(const SDL_Event& e, const CharWindow&) -> void {
	m_game.gameClient()->handleEvent(e);
}

auto GameClientState::update(float deltaTime) -> void {
	if (!m_game.gameClient()->update(deltaTime)) {
		m_game.reset();
	}
}

auto GameClientState::draw(CharWindow&) -> void {
	m_game.gameClient()->draw();
}
