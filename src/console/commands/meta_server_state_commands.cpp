#include "meta_server_state_commands.hpp"

#include "../../game/game.hpp"                    // Game
#include "../../game/state/meta_server_state.hpp" // MetaServerState
#include "../call_frame_handle.hpp"               // CallFrameHandle
#include "../command.hpp"                         // cmd::...

#include <memory> // std::make_unique

CON_COMMAND(meta_start_server, "", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Start a dedicated meta server.", {}, nullptr) {
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}

	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	if (game.gameClient() || game.gameServer()) {
		return cmd::error("{}: Cannot start a meta server while in-game.", self.getName());
	}

	if (game.metaClient()) {
		return cmd::error("{}: Cannot start a meta server while running a meta client.", self.getName());
	}

	if (game.metaServer()) {
		return cmd::error("{}: Already running a meta server.", self.getName());
	}

	if (!game.setState(std::make_unique<MetaServerState>(game))) {
		return cmd::error("{}: Initialization failed.", self.getName());
	}
	return cmd::done();
}
