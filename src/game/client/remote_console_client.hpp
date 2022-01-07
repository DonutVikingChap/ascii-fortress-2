#ifndef AF2_CLIENT_REMOTE_CONSOLE_CLIENT_HPP
#define AF2_CLIENT_REMOTE_CONSOLE_CLIENT_HPP

#include "../../console/command.hpp"          // cmd::...
#include "../../console/process.hpp"          // IOBuffer, CallFrameHandle
#include "../../network/crypto.hpp"           // crypto::...
#include "../shared/game_client_messages.hpp" // msg::cl::in::RemoteConsole...
#include "../shared/game_server_messages.hpp" // msg::sv::out::RemoteConsole...

#include <cstdint>     // std::uint8_t
#include <memory>      // std::weak_ptr, std::shared_ptr
#include <string_view> // std::string_view

class VirtualMachine;

class RemoteConsoleClient {
public:
	enum class State : std::uint8_t {
		NONE,            // Not logged in.
		LOGIN_PART1,     // Waiting to receive login info.
		LOGIN_PART2,     // Waiting to receive a login reply.
		READY,           // Logged in and ready to send commands.
		WAITING,         // Sent a command. Waiting to receive a result.
		RESULT_RECEIVED, // Received a result.
		ABORTING,        // Aborted. Waiting to receive confirmation.
		LOGOUT,          // Logged out. Waiting to receive conformation.
	};

	RemoteConsoleClient() noexcept = default;

	virtual ~RemoteConsoleClient() = default;

	RemoteConsoleClient(const RemoteConsoleClient&) = default;
	RemoteConsoleClient(RemoteConsoleClient&&) noexcept = default;

	auto operator=(const RemoteConsoleClient&) -> RemoteConsoleClient& = default;
	auto operator=(RemoteConsoleClient&&) noexcept -> RemoteConsoleClient& = default;

	[[nodiscard]] auto initRconClient() noexcept -> bool;

	auto setRconOutput(const std::shared_ptr<IOBuffer>& output) noexcept -> void;
	auto resetRconOutput() noexcept -> void;

	[[nodiscard]] auto getRconState() const noexcept -> State;

	[[nodiscard]] auto writeRconLoginInfoRequest(std::string_view username) -> bool;
	[[nodiscard]] auto writeRconLoginRequest(std::string_view username, crypto::pw::PasswordView password) -> bool;
	[[nodiscard]] auto writeRconCommand(std::string_view command) -> bool;

	[[nodiscard]] auto pullRconResult() noexcept -> cmd::Result;

	[[nodiscard]] auto writeRconAbortCommand() -> bool;
	[[nodiscard]] auto writeRconLogout() -> bool;

protected:
	auto handleMessage(msg::cl::in::RemoteConsoleLoginInfo&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleLoginGranted&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleLoginDenied&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleResult&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleOutput&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleDone&& msg) -> void;
	auto handleMessage(msg::cl::in::RemoteConsoleLoggedOut&& msg) -> void;

	virtual auto vm() -> VirtualMachine& = 0;

	virtual auto write(msg::sv::out::RemoteConsoleLoginInfoRequest&& msg) -> bool = 0;
	virtual auto write(msg::sv::out::RemoteConsoleLoginRequest&& msg) -> bool = 0;
	virtual auto write(msg::sv::out::RemoteConsoleCommand&& msg) -> bool = 0;
	virtual auto write(msg::sv::out::RemoteConsoleAbortCommand&& msg) -> bool = 0;
	virtual auto write(msg::sv::out::RemoteConsoleLogout&& msg) -> bool = 0;

private:
	State m_state = State::NONE;
	crypto::pw::Salt m_salt{};
	crypto::pw::HashType m_hashType{};
	cmd::Result m_result = cmd::done();
	std::weak_ptr<IOBuffer> m_output{};
};

#endif
