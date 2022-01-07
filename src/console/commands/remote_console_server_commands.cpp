#include "remote_console_server_commands.hpp"

#include "../../game/game.hpp"               // Game
#include "../../game/server/game_server.hpp" // GameServer
#include "../../network/crypto.hpp"          // crypto::...
#include "../../utilities/span.hpp"          // util::Span, util::asBytes
#include "../call_frame_handle.hpp"          // CallFrameHandle
#include "../command.hpp"                    // cmd::...
#include "../command_options.hpp"            // cmd::...
#include "../process.hpp"                    // Process
#include "../suggestions.hpp"                // Suggestions, SUGGESTIONS

#include <any>         // std::any, std::any_cast
#include <cstddef>     // std::byte
#include <optional>    // std::optional, std::nullopt
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::move

// clang-format off
ConVarBool			sv_rcon_enable{			"sv_rcon_enable",			true,	ConVar::SERVER_SETTING,	"Whether or not to allow remote console sessions to connect to the server."};
ConVarFloatMinMax	sv_rcon_session_timeout{"sv_rcon_session_timeout",	60.0f,	ConVar::SERVER_SETTING,	"How many seconds to wait before ending inactive remote console sessions.", 0.0f, -1.0f};
// clang-format on

namespace {

SUGGESTIONS(suggestRconUsername) {
	if (i == 1 && server) {
		return Suggestions{server->getRconUsernames()};
	}
	return Suggestions{};
}

} // namespace

CON_COMMAND(sv_rcon_add_user, "[options...] <username> [password]", ConCommand::SERVER | ConCommand::ADMIN_ONLY,
			"Add a remote console user account.",
			cmd::opts(cmd::opt('a', "admin", "This user should have admin privileges."),
					  cmd::opt('h', "hashtype",
							   fmt::format("Type of hash function to use ({0}/{1}/{2}). Slower is stronger. Default is \"{0}\".",
										   getHashTypeString(crypto::pw::HashType::FAST), getHashTypeString(crypto::pw::HashType::MEDIUM),
										   getHashTypeString(crypto::pw::HashType::SLOW)),
							   cmd::OptionType::ARGUMENT_REQUIRED)),
			nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 1 && args.size() != 2) {
		return cmd::error(self.getUsage());
	}

	if (args.size() == 2 && (frame.process()->getUserFlags() & Process::CONSOLE) != 0) {
		game.warning(
			fmt::format("{0}: Warning: The password you just typed may have been logged to the console. Check any log files if this was a "
						"mistake. Console users are advised to use {0} <username> to avoid this.",
						self.getName()));
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	if (frame.progress() == 0) {
		if (args.size() == 2) {
			frame.data().emplace<std::string>(args[1]);
		} else {
			game.println(fmt::format("{}: Enter new password.", self.getName()));
			game.setConsoleModePassword([frame](std::string_view password) { frame.data().emplace<std::string>(password); });
			return cmd::deferToNextFrame(1);
		}
	}

	if (!data.has_value()) {
		return cmd::deferToNextFrame(1);
	}

	const auto& username = args[0];
	const auto password = std::any_cast<std::string>(std::move(data));
	data.reset();

	const auto admin = options['a'].isSet();

	auto hashType = crypto::pw::HashType::FAST;
	if (const auto& hashTypeStr = options['h']) {
		if (const auto type = crypto::pw::getHashType(*hashTypeStr)) {
			hashType = *type;
		} else {
			return cmd::error("{}: Invalid hash type \"{}\".", self.getName(), *hashTypeStr);
		}
	}

	auto salt = crypto::pw::Salt{};
	crypto::pw::generateSalt(salt);

	auto key = crypto::pw::Key{};
	if (!crypto::pw::deriveKey(key, salt, password, hashType)) {
		return cmd::error("{}: Failed to derive password key for user \"{}\"!", self.getName(), username);
	}

	auto keyHash = crypto::FastHash{};
	if (!crypto::fastHash(keyHash, util::asBytes(util::Span{key}))) {
		return cmd::error("{}: Failed to hash password key for user \"{}\"!", self.getName(), username);
	}

	assert(server);
	if (!server->addRconUser(std::string{username}, keyHash, salt, hashType, admin)) {
		return cmd::error("{}: Failed to add user \"{}\"!", self.getName(), username);
	}

	return cmd::done("Successfully added remote console user \"{}\"", username);
}

CON_COMMAND(sv_rcon_add_user_hashed, "[options...] <username> <hashtype> <keyhash> <salt>", ConCommand::SERVER | ConCommand::ADMIN_ONLY,
			"Add a remote console user account with a pre-hashed password.",
			cmd::opts(cmd::opt('a', "admin", "This user should have admin privileges.")), nullptr) {
	const auto [args, options] = cmd::parse(argv, self.getOptions());
	if (args.size() != 4) {
		return cmd::error(self.getUsage());
	}

	if (const auto& error = options.error()) {
		return cmd::error("{}: {}", self.getName(), *error);
	}

	const auto& username = args[0];
	const auto& hashTypeStr = args[1];
	const auto& keyHashStr = args[2];
	const auto& saltStr = args[3];

	const auto admin = options['a'].isSet();

	auto keyHash = crypto::FastHash{};
	if (keyHashStr.size() != keyHash.size()) {
		return cmd::error("{}: Invalid key hash size ({}/{}).", self.getName(), keyHashStr.size(), keyHash.size());
	}
	util::copy(keyHashStr, keyHash.begin());

	auto salt = crypto::pw::Salt{};
	if (saltStr.size() != salt.size()) {
		return cmd::error("{}: Invalid salt size ({}/{}).", self.getName(), saltStr.size(), salt.size());
	}
	util::copy(saltStr, salt.begin());

	const auto hashType = crypto::pw::getHashType(hashTypeStr);
	if (!hashType) {
		return cmd::error("{}: Invalid hash type \"{}\".", self.getName(), hashTypeStr);
	}

	assert(server);
	if (!server->addRconUser(std::string{username}, keyHash, salt, *hashType, admin)) {
		return cmd::error("{}: Failed to add user \"{}\"!", self.getName(), username);
	}
	return cmd::done();
}

CON_COMMAND(sv_rcon_remove_user, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Remove a remote console user account.", {}, suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	if (!server->removeRconUser(argv[1])) {
		return cmd::error("{}: User \"{}\" not found.", self.getName(), argv[1]);
	}
	return cmd::done();
}

CON_COMMAND(sv_rcon_userlist, "", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Get the usernames of all added rcon users.", {}, nullptr) {
	if (argv.size() != 1) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->getRconUserList());
}

CON_COMMAND(sv_rcon_has_user, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Check if a certain remote console user exists.",
			{}, suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->isRconUser(argv[1]));
}

CON_COMMAND(sv_rcon_logged_in, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY,
			"Check if a certain user has an active remote console session.", {}, suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->isRconLoggedIn(argv[1]));
}

CON_COMMAND(sv_rcon_running, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY,
			"Check if a certain user has a running remote console process.", {}, suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	return cmd::done(server->isRconProcessRunning(argv[1]));
}

CON_COMMAND(sv_rcon_end, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "End the remote console session of a certain user.", {},
			suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	if (!server->endRconSession(argv[1])) {
		return cmd::error("{}: Session not found.", self.getName());
	}
	return cmd::done();
}

CON_COMMAND(sv_rcon_kill, "<username>", ConCommand::SERVER | ConCommand::ADMIN_ONLY, "Stop the remote console session of a certain user.",
			{}, suggestRconUsername) {
	if (argv.size() != 2) {
		return cmd::error(self.getUsage());
	}
	assert(server);
	if (!server->killRconProcess(argv[1])) {
		return cmd::error("{}: Not running a process.", self.getName());
	}
	return cmd::done();
}
