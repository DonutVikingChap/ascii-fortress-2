#include "listen_server_state_commands.hpp"

#include "../../game/game.hpp"                      // Game
#include "../../game/state/listen_server_state.hpp" // ListenServerState
#include "../../network/connection.hpp"             // net::IpAddress
#include "../call_frame_handle.hpp"                 // CallFrameHandle
#include "../command.hpp"                           // cmd::...
#include "../suggestions.hpp"                       // Suggestions
#include "file_commands.hpp"                        // data_dir, data_subdir_maps
#include "game_client_commands.hpp"                 // address, port, username, password
#include "game_commands.hpp"                        // maplist
#include "game_server_commands.hpp"                 // sv_port, sv_password, sv_map

#include <filesystem> // std::filesystem::is_regular_file
#include <fmt/core.h> // fmt::format
#include <memory>     // std::make_unique
#include <string>     // std::string

CON_COMMAND(start, "[map]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Start a listen server running the specified map.", {},
			Suggestions::suggestMap<1>) {
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}

	if (argv.size() > 2) {
		return cmd::error(self.getUsage());
	}

	if (game.gameServer()) {
		return cmd::error("{}: Cannot create a server while one is running. Use \"changelevel\" to switch maps.", self.getName());
	}

	if (game.gameClient()) {
		return cmd::error("{}: Cannot create a server while connected.", self.getName());
	}

	if (game.metaServer()) {
		return cmd::error("{}: Cannot create a server while running a meta server.", self.getName());
	}

	if (username.empty()) {
		return cmd::error("Please choose a username!");
	}

	if (argv.size() > 1) {
		if (auto result = sv_map.set(argv[1], game, server, client, metaServer, metaClient); result.status == cmd::Status::ERROR_MSG) {
			return result;
		}
	}

	const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_maps, sv_map);
	if (!std::filesystem::is_regular_file(filepath)) {
		return cmd::error("{}: Map \"{}\" not found. Try \"{}\".", self.getName(), sv_map, GET_COMMAND(maplist).getName());
	}

	if (auto result = sv_password.set(password, game, server, client, metaServer, metaClient); result.status == cmd::Status::ERROR_MSG) {
		return result;
	}

	const auto oldAddress = std::string{address.cvar().getRaw()};
	const auto oldPort = std::string{port.cvar().getRaw()};

	if (auto result = address.setSilent(std::string{net::IpAddress::localhost()}); result.status == cmd::Status::ERROR_MSG) {
		return result;
	}

	if (auto result = port.setSilent(sv_port.cvar().getRaw()); result.status == cmd::Status::ERROR_MSG) {
		address.setSilent(oldAddress);
		return result;
	}

	if (!game.setState(std::make_unique<ListenServerState>(game))) {
		address.setSilent(oldAddress);
		port.setSilent(oldPort);
		return cmd::error("{}: Initialization failed.", self.getName());
	}

	address.setSilent(oldAddress);
	port.setSilent(oldPort);
	return cmd::done();
}
