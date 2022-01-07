#include "game_client_state_commands.hpp"

#include "../../game/game.hpp"                    // Game
#include "../../game/state/game_client_state.hpp" // GameClientState
#include "../call_frame_handle.hpp"               // CallFrameHandle
#include "../command.hpp"                         // cmd::...
#include "game_client_commands.hpp"               // address, port, username

#include <memory> // std::make_unique
#include <string> // std::string

CON_COMMAND(connect, "[address[:port]]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Join a server.", {}, nullptr) {
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}

	if (argv.size() > 2) {
		return cmd::error(self.getUsage());
	}

	if (game.gameClient() || game.gameServer()) {
		return cmd::error("{}: Cannot connect to a server while in-game.", self.getName());
	}

	if (game.metaServer()) {
		return cmd::error("{}: Cannot connect to a server while running a meta server.", self.getName());
	}

	if (username.empty()) {
		return cmd::error("Please choose a username!");
	}

	const auto oldAddress = std::string{address.cvar().getRaw()};
	const auto oldPort = std::string{port.cvar().getRaw()};

	if (argv.size() > 1) {
		auto addressStr = std::string{argv[1]};
		if (const auto i = addressStr.rfind(':'); i != std::string::npos) {
			if (auto result = port.setSilent(addressStr.substr(i + 1)); result.status == cmd::Status::ERROR_MSG) {
				return result;
			}
			addressStr.erase(i, std::string::npos);
		}
		if (auto result = address.setSilent(addressStr); result.status == cmd::Status::ERROR_MSG) {
			port.setSilent(oldPort);
			return result;
		}
	}

	if (address.empty()) {
		address.setSilent(oldAddress);
		port.setSilent(oldPort);
		return cmd::error("Please enter a server address!");
	}

	if (!game.setState(std::make_unique<GameClientState>(game))) {
		address.setSilent(oldAddress);
		port.setSilent(oldPort);
		return cmd::error("{}: Initialization failed.", self.getName());
	}

	address.setSilent(oldAddress);
	port.setSilent(oldPort);
	return cmd::done();
}
