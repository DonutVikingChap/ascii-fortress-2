#include "meta_server_state.hpp"

#include "../game.hpp"             // Game
#include "../meta/meta_server.hpp" // MetaServer

MetaServerState::MetaServerState(Game& game)
	: m_game(game) {}

auto MetaServerState::init() -> bool {
	return m_game.startMetaServer();
}

auto MetaServerState::handleEvent(const SDL_Event&, const CharWindow&) -> void {}

auto MetaServerState::update(float deltaTime) -> void {
	if (!m_game.metaServer()->update(deltaTime)) {
		m_game.reset();
	}
}

auto MetaServerState::draw(CharWindow&) -> void {}
