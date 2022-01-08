#include "remote_console_client_commands.hpp"

#include "../../game/client/game_client.hpp"           // GameClient
#include "../../game/client/remote_console_client.hpp" // RemoteConsoleClient
#include "../../game/game.hpp"                         // Game
#include "../../utilities/algorithm.hpp"               // util::subview
#include "../../utilities/string.hpp"                  // util::join
#include "../call_frame_handle.hpp"                    // CallFrameHandle
#include "../command.hpp"                              // cmd::...
#include "../process.hpp"                              // Process

#include <any>         // std::any, std::any_cast
#include <cassert>     // assert
#include <fmt/core.h>  // fmt::format
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

CON_COMMAND(rcon_login, "<username> [password]", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Login to the server's remote console.", {}, nullptr) {
	assert(client);
	switch (frame.progress()) {
		case 0:
			// Stage 0: Initialize password input.
			if (argv.size() != 2 && argv.size() != 3) {
				return cmd::error(self.getUsage());
			}

			if (argv.size() == 3 && (frame.process()->getUserFlags() & Process::CONSOLE) != 0) {
				game.warning(
					fmt::format("{0}: Warning: The password you just typed may have been logged to the console. Check any log files if "
				                "this was a mistake. Console users are advised to use {0} <username> to avoid this.",
				                self.getName()));
			}

			if (client->getRconState() != RemoteConsoleClient::State::NONE) {
				return cmd::error("{}: Already logged in.", self.getName());
			}

			if (argv.size() != 3) {
				game.println(fmt::format("{}: Enter password.", self.getName()));
				game.setConsoleModePassword([frame](std::string_view password) { frame.data().emplace<std::string>(password); });
				game.activateConsole();

				return cmd::deferToNextFrame(1);
			}

			data.emplace<std::string>(argv[2]);
			frame.arguments().pop_back();
			[[fallthrough]];
		case 1:
			// Stage 1: Acquire password, send login info request.
			if (!data.has_value()) {
				return cmd::deferToNextFrame(1);
			}

			if (client->getRconState() != RemoteConsoleClient::State::NONE) {
				return cmd::error("{}: Connection error.", self.getName());
			}

			if (!client->writeRconLoginInfoRequest(frame.arguments()[1].value)) {
				return cmd::error("{}: Failed to write info request.", self.getName());
			}

			return cmd::deferToNextFrame(2);
		case 2:
			// Stage 2: Acquire login info, send login request.
			if (client->getRconState() == RemoteConsoleClient::State::LOGIN_PART1) {
				return cmd::deferToNextFrame(2);
			}

			if (client->getRconState() != RemoteConsoleClient::State::LOGIN_PART2) {
				return cmd::error("{}: Connection error.", self.getName());
			}

			if (!client->writeRconLoginRequest(argv[1], std::any_cast<std::string>(std::move(data)))) {
				return cmd::error("{}: Failed to write login request.", self.getName());
			}

			data.reset();

			return cmd::deferToNextFrame(3);
		case 3:
			// Stage 3: Acquire login reply, return result.
			if (client->getRconState() == RemoteConsoleClient::State::LOGIN_PART2) {
				return cmd::deferToNextFrame(3);
			}

			if (client->getRconState() == RemoteConsoleClient::State::NONE) {
				return cmd::error("{}: Request denied by server.", self.getName());
			}

			if (client->getRconState() != RemoteConsoleClient::State::READY) {
				return cmd::error("{}: Connection error.", self.getName());
			}

			return cmd::done("Logged in to remote console as user \"{}\".", argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(rcon_logout, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Log out from the server's remote console.", {}, nullptr) {
	assert(client);
	if (frame.progress() == 0) {
		if (argv.size() != 1) {
			return cmd::error(self.getUsage());
		}

		if (client->getRconState() == RemoteConsoleClient::State::NONE) {
			return cmd::error("{}: Not logged in.", self.getName());
		}

		if (!client->writeRconLogout()) {
			return cmd::error("{}: Failed to write command.", self.getName());
		}

		return cmd::deferToNextFrame(1);
	}

	if (client->getRconState() == RemoteConsoleClient::State::LOGOUT) {
		return cmd::deferToNextFrame(1);
	}
	return cmd::done();
}

CON_COMMAND(rcon_abort, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON, "Abort the current remote console request.", {}, nullptr) {
	assert(client);
	if (frame.progress() == 0) {
		if (argv.size() != 1) {
			return cmd::error(self.getUsage());
		}

		if (!client->writeRconAbortCommand()) {
			return cmd::error("{}: Failed to write command.", self.getName());
		}

		return cmd::deferToNextFrame(1);
	}

	if (client->getRconState() == RemoteConsoleClient::State::ABORTING) {
		return cmd::deferToNextFrame(1);
	}

	return cmd::done();
}

CON_COMMAND(rcon, "[args...]", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Execute a console command on the remote server.", {}, nullptr) {
	assert(client);
	if (frame.progress() == 0) {
		if (client->getRconState() == RemoteConsoleClient::State::NONE) {
			return cmd::error("{}: Not logged in. Use {} {}.", self.getName(), GET_COMMAND(rcon_login).getName(), GET_COMMAND(rcon_login).getParameters());
		}

		if (client->getRconState() == RemoteConsoleClient::State::WAITING) {
			return cmd::error("{}: Not ready. Current command is not finished. Use {} to cancel.", self.getName(), GET_COMMAND(rcon_abort).getName());
		}

		if (client->getRconState() != RemoteConsoleClient::State::READY) {
			return cmd::error("{}: Not ready to receive commands.", self.getName());
		}

		if (!client->writeRconCommand((argv.size() == 2) ? argv[1] : util::subview(argv, 1) | util::join(' '))) {
			return cmd::error("{}: Failed to write command.", self.getName());
		}

		frame.arguments().erase(frame.arguments().begin() + 1, frame.arguments().end());

		if (auto output = frame.process()->getOutput()) {
			if (const auto ptr = output->lock()) {
				client->setRconOutput(ptr);
			}
		}

		return cmd::deferToNextFrame(1);
	}

	if (client->getRconState() == RemoteConsoleClient::State::WAITING) {
		return cmd::deferToNextFrame(1);
	}

	if (client->getRconState() != RemoteConsoleClient::State::RESULT_RECEIVED) {
		return cmd::error("{}: Connection error.", self.getName());
	}

	return client->pullRconResult();
}

CON_COMMAND(rcon_ready, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Check if the remote console is ready to receive commands.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(client);
	return cmd::done(client->getRconState() == RemoteConsoleClient::State::READY);
}

CON_COMMAND(rcon_status, "", ConCommand::CLIENT | ConCommand::ADMIN_ONLY | ConCommand::NO_RCON,
            "Check the current status of the remote console client.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}

	assert(client);
	switch (client->getRconState()) {
		case RemoteConsoleClient::State::NONE: return cmd::done("none");
		case RemoteConsoleClient::State::LOGIN_PART1: return cmd::done("login_part1");
		case RemoteConsoleClient::State::LOGIN_PART2: return cmd::done("login_part2");
		case RemoteConsoleClient::State::READY: return cmd::done("ready");
		case RemoteConsoleClient::State::WAITING: return cmd::done("waiting");
		case RemoteConsoleClient::State::RESULT_RECEIVED: return cmd::done("result_received");
		case RemoteConsoleClient::State::ABORTING: return cmd::done("aborting");
		case RemoteConsoleClient::State::LOGOUT: return cmd::done("logout");
	}
	return cmd::error("{}: Unknown rcon state!", self.getName());
}
