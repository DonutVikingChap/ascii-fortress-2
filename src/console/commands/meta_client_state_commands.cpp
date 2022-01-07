#include "meta_client_state_commands.hpp"

#include "../../game/game.hpp"                    // Game
#include "../../game/state/meta_client_state.hpp" // MetaClientState
#include "../call_frame_handle.hpp"               // CallFrameHandle
#include "../command.hpp"                         // cmd::...
#include "meta_client_commands.hpp"               // meta_address, meta_port

#include <memory> // std::make_unique
#include <string> // std::string

CON_COMMAND(meta_start, "[address[:port]]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Start a meta client.", {}, nullptr) {
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}

	if (argv.size() > 2) {
		return cmd::error(self.getUsage());
	}

	if (game.gameClient() || game.gameServer()) {
		return cmd::error("{}: Cannot start a meta client while in-game.", self.getName());
	}

	if (game.metaClient()) {
		return cmd::error("{}: Already running a meta client.", self.getName());
	}

	if (game.metaServer()) {
		return cmd::error("{}: Cannot start a meta client while running a meta server.", self.getName());
	}

	const auto oldMetaAddress = std::string{meta_address.cvar().getRaw()};
	const auto oldMetaPort = std::string{meta_port.cvar().getRaw()};

	if (argv.size() > 1) {
		auto addressStr = std::string{argv[1]};
		if (const auto i = addressStr.rfind(':'); i != std::string::npos) {
			if (auto result = meta_port.setSilent(addressStr.substr(i + 1)); result.status == cmd::Status::ERROR_MSG) {
				return result;
			}
			addressStr.erase(i, std::string::npos);
		}
		if (auto result = meta_address.setSilent(addressStr); result.status == cmd::Status::ERROR_MSG) {
			meta_port.setSilent(oldMetaPort);
			return result;
		}
	}

	if (meta_address.empty()) {
		meta_address.setSilent(oldMetaAddress);
		meta_port.setSilent(oldMetaPort);
		return cmd::error("Please enter a meta server address!");
	}

	if (!game.setState(std::make_unique<MetaClientState>(game))) {
		meta_address.setSilent(oldMetaAddress);
		meta_port.setSilent(oldMetaPort);
		return cmd::error("{}: Initialization failed.", self.getName());
	}

	meta_address.setSilent(oldMetaAddress);
	meta_port.setSilent(oldMetaPort);
	return cmd::done();
}
