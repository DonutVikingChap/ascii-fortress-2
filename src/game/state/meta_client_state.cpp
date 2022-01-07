#include "meta_client_state.hpp"

#include "../game.hpp"             // Game
#include "../meta/meta_client.hpp" // MetaClient

MetaClientState::MetaClientState(Game& game)
	: m_game(game) {}

auto MetaClientState::init() -> bool {
	return m_game.startMetaClient();
}

auto MetaClientState::handleEvent(const SDL_Event&, const CharWindow&) -> void {}

auto MetaClientState::update(float deltaTime) -> void {
	if (!m_game.metaClient()->update(deltaTime)) {
		m_game.reset();
	}
}

auto MetaClientState::draw(CharWindow&) -> void {}
