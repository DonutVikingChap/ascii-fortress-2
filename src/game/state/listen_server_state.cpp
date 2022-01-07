#include "listen_server_state.hpp"

#include "../client/game_client.hpp" // GameClient
#include "../game.hpp"               // Game
#include "../server/game_server.hpp" // GameServer

ListenServerState::ListenServerState(Game& game)
	: m_game(game) {}

auto ListenServerState::init() -> bool {
	return m_game.startGameServer() && m_game.startGameClient();
}

auto ListenServerState::handleEvent(const SDL_Event& e, const CharWindow&) -> void {
	m_game.gameClient()->handleEvent(e);
}

auto ListenServerState::update(float deltaTime) -> void {
	if (!m_game.gameServer()->update(deltaTime) || !m_game.gameClient()->update(deltaTime)) {
		m_game.reset();
	}
}

auto ListenServerState::draw(CharWindow&) -> void {
	m_game.gameClient()->draw();
}
