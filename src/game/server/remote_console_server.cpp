#include "remote_console_server.hpp"

#include "../../console/commands/remote_console_client_commands.hpp" // cmd_rcon
#include "../../console/commands/remote_console_server_commands.hpp" // sv_rcon_enable, sv_rcon_session_timeout
#include "../../console/con_command.hpp"                             // ConCommand, CON_COMMAND, GET_COMMAND
#include "../../console/convar.hpp"                                  // ConVar...
#include "../../console/script.hpp"                                  // Script
#include "../../console/virtual_machine.hpp"                         // VirtualMachine
#include "../../debug.hpp"                                           // Msg, INFO_MSG, INFO_MSG_INDENT, DEBUG_MSG
#include "../../utilities/algorithm.hpp" // util::transform, util::eraseIf, util::collect, util::copy, util::take
#include "../../utilities/span.hpp"      // util::Span, util::asBytes, util::asWritableBytes
#include "../../utilities/string.hpp"    // util::join

#include <array>       // std::array
#include <cassert>     // assert
#include <fmt/core.h>  // fmt::format
#include <type_traits> // std::underlying_type_t
#include <utility>     // std::move

auto RemoteConsoleServer::initRconServer() noexcept -> bool {
	if (!crypto::init()) {
		return false;
	}
	crypto::generateSeed(m_seed);
	return true;
}

auto RemoteConsoleServer::updateRconServer(float deltaTime) -> void {
	auto& game = this->game();
	auto& server = this->gameServer();
	util::eraseIf(m_sessions, [&](auto& kv) {
		auto& [username, session] = kv;
		if (!this->updateSession(deltaTime, username, session, game, server)) {
			this->write(username, msg::cl::out::RemoteConsoleLoggedOut{{}});
			this->unregisterRconClient(username);
			return true;
		}
		return false;
	});
}

auto RemoteConsoleServer::isRconUser(const std::string& username) const -> bool {
	return m_users.count(username) != 0;
}

auto RemoteConsoleServer::isRconLoggedIn(std::string_view username) const -> bool {
	return m_sessions.count(username) != 0;
}

auto RemoteConsoleServer::isRconProcessRunning(std::string_view username) const -> bool {
	if (const auto it = m_sessions.find(username); it != m_sessions.end()) {
		return it->second.process != nullptr;
	}
	return false;
}

auto RemoteConsoleServer::endRconSession(std::string_view username) -> bool {
	if (const auto it = m_sessions.find(username); it != m_sessions.end()) {
		this->write(username, msg::cl::out::RemoteConsoleLoggedOut{{}});
		this->unregisterRconClient(username);
		m_sessions.erase(it);
		return true;
	}
	return false;
}

auto RemoteConsoleServer::killRconProcess(std::string_view username) -> bool {
	if (const auto it = m_sessions.find(username); it != m_sessions.end()) {
		if (it->second.process) {
			it->second.buffer.reset();
			it->second.process.reset();
			return true;
		}
	}
	return false;
}

auto RemoteConsoleServer::addRconUser(std::string username, const crypto::FastHash& keyHash, const crypto::pw::Salt& salt,
                                      crypto::pw::HashType hashType, bool admin) -> bool {
	return m_users.try_emplace(std::move(username), keyHash, salt, hashType, admin).second;
}

auto RemoteConsoleServer::removeRconUser(const std::string& username) -> bool {
	this->endRconSession(username);
	return m_users.erase(username) != 0;
}

auto RemoteConsoleServer::getRconUsernames() const -> std::vector<std::string> {
	static constexpr auto getUsername = [](const auto& kv) {
		return kv.first;
	};

	return m_users | util::transform(getUsername) | util::collect<std::vector<std::string>>();
}

auto RemoteConsoleServer::getRconUserList() const -> std::string {
	static constexpr auto formatUser = [](const auto& kv) {
		const auto& [username, user] = kv;
		return fmt::format("{}{}", Script::escapedString(username), (user.admin) ? " (admin)" : "");
	};

	return m_users | util::transform(formatUser) | util::join('\n');
}

auto RemoteConsoleServer::getRconConfig() const -> std::string {
	using UserRef = util::Reference<const std::pair<const std::string, User>>;
	using Refs = std::vector<UserRef>;

	static constexpr auto compareUserRefs = [](const auto& lhs, const auto& rhs) {
		return lhs->first < rhs->first;
	};

	static constexpr auto getUserCommand = [](const auto& kv) {
		const auto& [username, user] = *kv;
		return fmt::format("{}{} {} {} {} {}",
		                   GET_COMMAND(sv_rcon_add_user_hashed).getName(),
		                   (user.admin) ? " --admin" : "",
		                   Script::escapedString(username),
		                   getHashTypeString(user.hashType),
		                   Script::escapedString(std::string_view{reinterpret_cast<const char*>(user.keyHash.data()), user.keyHash.size()}),
		                   Script::escapedString(std::string_view{reinterpret_cast<const char*>(user.salt.data()), user.salt.size()}));
	};

	return m_users | util::collect<Refs>() | util::sort(compareUserRefs) | util::transform(getUserCommand) | util::join('\n');
}

auto RemoteConsoleServer::handleMessage(msg::sv::in::RemoteConsoleLoginInfoRequest&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server received login info request for user \"{}\".", msg.username);
	if (sv_rcon_enable) {
		if (const auto it = m_users.find(msg.username); it != m_users.end()) {
			this->reply(msg::cl::out::RemoteConsoleLoginInfo{{}, it->second.salt, it->second.hashType});
		} else {
			// Reply with a fake random salt and hash type to make it harder to find out which usernames exist.

			// Create a unique seed based on the username and our randomly generated seed.
			auto seed = m_seed;
			util::copy(msg.username | util::take(seed.size()), seed.begin());

			// Generate fake salt from the unique seed.
			auto fakeSalt = crypto::pw::Salt{};
			crypto::pw::generateSalt(fakeSalt, seed);

			// Always use HashType::FAST for responsiveness.
			this->reply(msg::cl::out::RemoteConsoleLoginInfo{{}, fakeSalt, crypto::pw::HashType::FAST});
		}
	} else {
		this->reply(msg::cl::out::RemoteConsoleLoginDenied{{}});
	}
}

auto RemoteConsoleServer::handleMessage(msg::sv::in::RemoteConsoleLoginRequest&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server received login request for user \"{}\".", msg.username);
	if (sv_rcon_enable) {
		if (const auto it = m_users.find(msg.username); it != m_users.end()) {
			if (crypto::verifyFastHash(it->second.keyHash, util::asBytes(util::Span{msg.passwordKey}))) {
				if (const auto& [sessionIt, inserted] = m_sessions.try_emplace(it->first, it->second.admin); inserted) {
					this->registerCurrentRconClient(sessionIt->first);
					this->reply(msg::cl::out::RemoteConsoleLoginGranted{{}});
					INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server granted login request for user \"{}\".", msg.username);
					return;
				}
			}
		}
	}
	this->reply(msg::cl::out::RemoteConsoleLoginDenied{{}});
	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server denied login request for user \"{}\".", msg.username);
}

auto RemoteConsoleServer::handleMessage(msg::sv::in::RemoteConsoleCommand&& msg) -> void {
	if (this->testSpam()) {
		return;
	}

	if (const auto& registeredUsername = this->getCurrentClientRegisteredRconUsername()) {
		if (const auto it = m_sessions.find(*registeredUsername); it != m_sessions.end()) {
			INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server accepted command.");
			auto& [username, session] = *it;
			if (session.process) {
				this->reply(msg::cl::out::RemoteConsoleResult{{}, cmd::error("{}: Remote process already running!", GET_COMMAND(rcon).getName())});
				this->reply(msg::cl::out::RemoteConsoleDone{{}});
				return;
			}

			auto processFlags = Process::UserFlags{Process::REMOTE | Process::CONSOLE};
			if (session.admin) {
				processFlags |= Process::ADMIN;
			}

			auto& vm = this->vm();

			if (!(session.process = vm.launchProcess(processFlags))) {
				this->reply(msg::cl::out::RemoteConsoleResult{{}, cmd::error("{}: Couldn't launch remote process!", GET_COMMAND(rcon).getName())});
				this->reply(msg::cl::out::RemoteConsoleDone{{}});
				return;
			}

			assert(session.buffer == nullptr);
			if (!(session.buffer = std::make_shared<IOBuffer>())) {
				session.process.reset();
				this->reply(
					msg::cl::out::RemoteConsoleResult{{}, cmd::error("{}: Couldn't allocate remote output buffer!", GET_COMMAND(rcon).getName())});
				this->reply(msg::cl::out::RemoteConsoleDone{{}});
				return;
			}

			const auto frame = session.process->call(std::make_shared<Environment>(vm.globalEnv()), msg.command);
			if (!frame) {
				session.buffer.reset();
				session.process.reset();
				this->reply(msg::cl::out::RemoteConsoleResult{{}, cmd::error("{}: Stack overflow.", GET_COMMAND(rcon).getName())});
				this->reply(msg::cl::out::RemoteConsoleDone{{}});
				return;
			}

			frame->makeTryBlock(); // Prevents error messages being output to the server's virtual machine.
			session.inactiveTime = 0.0f;
			session.process->setOutput(session.buffer);

			if (!this->updateSession(0.0f, username, session, this->game(), this->gameServer())) {
				this->reply(msg::cl::out::RemoteConsoleLoggedOut{{}});
				this->unregisterRconClient(it->first);
				m_sessions.erase(it);
			}
			return;
		}
	}
	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server denied command (invalid token).");
	this->reply(msg::cl::out::RemoteConsoleLoggedOut{{}});
}

auto RemoteConsoleServer::handleMessage(msg::sv::in::RemoteConsoleAbortCommand&&) -> void {
	if (this->testSpam()) {
		return;
	}

	if (const auto& registeredUsername = this->getCurrentClientRegisteredRconUsername()) {
		if (const auto it = m_sessions.find(*registeredUsername); it != m_sessions.end()) {
			INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server accepted abort command.");
			if (it->second.process) {
				it->second.buffer.reset();
				it->second.process.reset();
			}
			this->reply(msg::cl::out::RemoteConsoleDone{{}});
			return;
		}
	}
	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server denied abort command (invalid token).");
	this->reply(msg::cl::out::RemoteConsoleLoggedOut{{}});
}

auto RemoteConsoleServer::handleMessage(msg::sv::in::RemoteConsoleLogout&&) -> void {
	if (this->testSpam()) {
		return;
	}

	if (const auto& registeredUsername = this->getCurrentClientRegisteredRconUsername()) {
		if (const auto it = m_sessions.find(*registeredUsername); it != m_sessions.end()) {
			INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server accepted logout command.");
			this->reply(msg::cl::out::RemoteConsoleLoggedOut{{}});
			this->unregisterRconClient(it->first);
			m_sessions.erase(it);
			return;
		}
	}
	INFO_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server received logout command with invalid token.");
	this->reply(msg::cl::out::RemoteConsoleLoggedOut{{}});
}

auto RemoteConsoleServer::updateSession(float deltaTime, std::string_view username, Session& session, Game& game, GameServer& server) -> bool {
	session.inactiveTime += deltaTime;
	if (session.inactiveTime >= sv_rcon_session_timeout) {
		DEBUG_MSG(Msg::SERVER | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon server session timed out for user \"{}\".", username);
		return false;
	}

	if (session.process) {
		assert(session.buffer);
		auto result = session.process->run(game, &server, nullptr, nullptr, nullptr);

		if (auto output = session.buffer->read()) {
			this->write(username, msg::cl::out::RemoteConsoleOutput{{}, *output});
		}

		this->write(username, msg::cl::out::RemoteConsoleResult{{}, result});
		if (session.process->done()) {
			session.process->end();
			session.buffer.reset();
			session.process.reset();
			this->write(username, msg::cl::out::RemoteConsoleDone{{}});
		}
	}
	return true;
}
