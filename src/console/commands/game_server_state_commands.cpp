#include "game_server_state_commands.hpp"

#include "../../game/game.hpp"                    // Game
#include "../../game/state/game_server_state.hpp" // GameServerState
#include "../call_frame_handle.hpp"               // CallFrameHandle
#include "../command.hpp"                         // cmd::...
#include "../suggestions.hpp"                     // Suggestions
#include "file_commands.hpp"                      // data_dir, data_subdir_maps
#include "game_client_commands.hpp"               // password
#include "game_commands.hpp"                      // maplist
#include "game_server_commands.hpp"               // sv_password, sv_map

#include <filesystem> // std::filesystem::is_regular_file
#include <fmt/core.h> // fmt::format
#include <memory>     // std::make_unique

CON_COMMAND(start_dedicated, "[map]", ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Start a dedicated server running the specified map.",
            {}, Suggestions::suggestMap<1>) {
	if (frame.progress() == 0) {
		return cmd::deferToNextFrame(1);
	}

	if (argv.size() > 2) {
		return cmd::error(self.getUsage());
	}

	if (game.gameClient() || game.gameServer()) {
		return cmd::error("{}: Cannot create a server while in-game.", self.getName());
	}

	if (game.metaServer()) {
		return cmd::error("{}: Cannot create a server while running a meta server.", self.getName());
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

	if (auto result = password.set("", game, server, client, metaServer, metaClient); result.status == cmd::Status::ERROR_MSG) {
		return result;
	}

	if (!game.setState(std::make_unique<GameServerState>(game))) {
		return cmd::error("{}: Initialization failed.", self.getName());
	}
	return cmd::done();
}
