#ifndef AF2_SERVER_REMOTE_CONSOLE_SERVER_HPP
#define AF2_SERVER_REMOTE_CONSOLE_SERVER_HPP

#include "../../console/io_buffer.hpp"        // IOBuffer
#include "../../console/process.hpp"          // Process
#include "../../network/crypto.hpp"           // crypto::...
#include "../shared/game_client_messages.hpp" // msg::cl::out::RemoteConsole...
#include "../shared/game_server_messages.hpp" // msg::sv::in::RemoteConsole...

#include <cassert>       // assert
#include <cstddef>       // std::size_t
#include <memory>        // std::shared_ptr, std::make_shared
#include <optional>      // std::optional, std::nullopt
#include <string>        // std::string
#include <string_view>   // std::string_view
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

class Game;
class GameServer;
class VirtualMachine;

class RemoteConsoleServer {
public:
	struct User final {
		constexpr User(const crypto::FastHash& keyHash, const crypto::pw::Salt& salt, crypto::pw::HashType hashType, bool admin) noexcept
			: keyHash(keyHash)
			, salt(salt)
			, hashType(hashType)
			, admin(admin) {}

		crypto::FastHash keyHash;
		crypto::pw::Salt salt;
		crypto::pw::HashType hashType;
		bool admin;
	};

	struct Session final {
		explicit Session(bool admin)
			: admin(admin) {}

		std::shared_ptr<Process> process{};
		std::shared_ptr<IOBuffer> buffer{};
		float inactiveTime = 0.0f;
		bool admin;
	};

	RemoteConsoleServer() = default;

	RemoteConsoleServer(const RemoteConsoleServer&) = default;
	RemoteConsoleServer(RemoteConsoleServer&&) noexcept = default;

	virtual ~RemoteConsoleServer() = default;

	auto operator=(const RemoteConsoleServer&) -> RemoteConsoleServer& = default;
	auto operator=(RemoteConsoleServer&&) noexcept -> RemoteConsoleServer& = default;

	[[nodiscard]] auto initRconServer() noexcept -> bool;

	auto updateRconServer(float deltaTime) -> void;

	[[nodiscard]] auto isRconUser(const std::string& username) const -> bool;
	[[nodiscard]] auto isRconLoggedIn(std::string_view username) const -> bool;
	[[nodiscard]] auto isRconProcessRunning(std::string_view username) const -> bool;

	auto endRconSession(std::string_view username) -> bool;
	[[nodiscard]] auto killRconProcess(std::string_view username) -> bool;

	[[nodiscard]] auto addRconUser(std::string username, const crypto::FastHash& keyHash, const crypto::pw::Salt& salt,
								   crypto::pw::HashType hashType, bool admin) -> bool;
	[[nodiscard]] auto removeRconUser(const std::string& username) -> bool;

	[[nodiscard]] auto getRconUsernames() const -> std::vector<std::string>;
	[[nodiscard]] auto getRconUserList() const -> std::string;
	[[nodiscard]] auto getRconConfig() const -> std::string;

protected:
	auto handleMessage(msg::sv::in::RemoteConsoleLoginInfoRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::RemoteConsoleLoginRequest&& msg) -> void;
	auto handleMessage(msg::sv::in::RemoteConsoleCommand&& msg) -> void;
	auto handleMessage(msg::sv::in::RemoteConsoleAbortCommand&& msg) -> void;
	auto handleMessage(msg::sv::in::RemoteConsoleLogout&& msg) -> void;

	[[nodiscard]] virtual auto game() -> Game& = 0;
	[[nodiscard]] virtual auto gameServer() -> GameServer& = 0;
	[[nodiscard]] virtual auto vm() -> VirtualMachine& = 0;

	[[nodiscard]] virtual auto testSpam() -> bool = 0;

	virtual auto reply(msg::cl::out::RemoteConsoleLoginInfo&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleLoginGranted&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleLoginDenied&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleResult&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleOutput&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleDone&& msg) -> void = 0;
	virtual auto reply(msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void = 0;

	virtual auto registerCurrentRconClient(std::string_view username) -> void = 0;
	virtual auto unregisterRconClient(std::string_view username) -> void = 0;

	[[nodiscard]] virtual auto getCurrentClientRegisteredRconUsername() const -> std::optional<std::string_view> = 0;

	virtual auto write(std::string_view username, msg::cl::out::RemoteConsoleResult&& msg) -> void = 0;
	virtual auto write(std::string_view username, msg::cl::out::RemoteConsoleOutput&& msg) -> void = 0;
	virtual auto write(std::string_view username, msg::cl::out::RemoteConsoleDone&& msg) -> void = 0;
	virtual auto write(std::string_view username, msg::cl::out::RemoteConsoleLoggedOut&& msg) -> void = 0;

private:
	auto updateSession(float deltaTime, std::string_view username, Session& session, Game& game, GameServer& server) -> bool;

	std::unordered_map<std::string, User> m_users;
	std::unordered_map<std::string_view, Session> m_sessions;
	crypto::Seed m_seed{};
};

#endif
