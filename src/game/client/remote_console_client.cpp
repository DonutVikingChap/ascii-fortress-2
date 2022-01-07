#include "remote_console_client.hpp"

#include "../../console/con_command.hpp"     // ConCommand, CON_COMMAND, GET_COMMAND
#include "../../console/virtual_machine.hpp" // VirtualMachine
#include "../../debug.hpp"                   // Msg, INFO_MSG, INFO_MSG_INDENT, DEBUG_MSG

#include <cassert> // assert
#include <utility> // std::move

auto RemoteConsoleClient::initRconClient() noexcept -> bool { // NOLINT(readability-convert-member-functions-to-static)
	return crypto::init();
}

auto RemoteConsoleClient::setRconOutput(const std::shared_ptr<IOBuffer>& output) noexcept -> void {
	output->setDone(false);
	m_output = output;
}

auto RemoteConsoleClient::resetRconOutput() noexcept -> void {
	if (const auto ptr = m_output.lock()) {
		ptr->setDone(true);
	}
	m_output.reset();
}

auto RemoteConsoleClient::getRconState() const noexcept -> RemoteConsoleClient::State {
	return m_state;
}

auto RemoteConsoleClient::writeRconLoginInfoRequest(std::string_view username) -> bool {
	assert(m_state == State::NONE);

	INFO_MSG_INDENT(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logging in.") {
		if (!this->write(msg::sv::out::RemoteConsoleLoginInfoRequest{{}, username})) {
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client failed to write login info request.");
			return false;
		}
	}

	m_state = State::LOGIN_PART1;
	return true;
}

auto RemoteConsoleClient::writeRconLoginRequest(std::string_view username, crypto::pw::PasswordView password) -> bool {
	assert(m_state == State::LOGIN_PART2);

	INFO_MSG_INDENT(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logging in...") {
		auto passwordKey = crypto::pw::Key{};
		if (!crypto::pw::deriveKey(passwordKey, m_salt, password, m_hashType)) {
			m_state = State::NONE;
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client password key derivation failed.");
			return false;
		}

		if (!this->write(msg::sv::out::RemoteConsoleLoginRequest{{}, username, passwordKey})) {
			m_state = State::NONE;
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client failed to write login request.");
			return false;
		}
	}

	return true;
}

auto RemoteConsoleClient::writeRconCommand(std::string_view command) -> bool {
	assert(m_state == State::READY);

	INFO_MSG_INDENT(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client sending command.") {
		if (!this->write(msg::sv::out::RemoteConsoleCommand{{}, command})) {
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client failed to write command.");
			return false;
		}
	}

	m_state = State::WAITING;
	return true;
}

auto RemoteConsoleClient::pullRconResult() noexcept -> cmd::Result {
	assert(m_state == State::RESULT_RECEIVED);

	INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client got result.");

	auto moved = std::move(m_result);
	m_result.reset();
	m_state = State::READY;
	return moved;
}

auto RemoteConsoleClient::writeRconAbortCommand() -> bool {
	INFO_MSG_INDENT(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client aborting.") {
		if (!this->write(msg::sv::out::RemoteConsoleAbortCommand{{}})) {
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client failed to write abort command.");
			return false;
		}
	}

	m_state = State::ABORTING;
	return true;
}

auto RemoteConsoleClient::writeRconLogout() -> bool {
	INFO_MSG_INDENT(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logging out.") {
		if (!this->write(msg::sv::out::RemoteConsoleLogout{{}})) {
			INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client failed to write logout command.");
			return false;
		}
	}

	m_state = State::LOGOUT;
	return true;
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleLoginInfo&& msg) -> void {
	if (m_state == State::LOGIN_PART1) {
		INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logging in..");
		m_salt = msg.passwordSalt;
		m_hashType = msg.passwordHashType;
		m_state = State::LOGIN_PART2;
	} else {
		DEBUG_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client received unsequenced login info.");
	}
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleLoginGranted &&) -> void {
	if (m_state == State::LOGIN_PART1 || m_state == State::LOGIN_PART2) {
		INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logged in successfully.");
		m_state = State::READY;
	} else {
		DEBUG_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client received unsequenced login reply.");
	}
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleLoginDenied &&) -> void {
	if (m_state == State::LOGIN_PART1 || m_state == State::LOGIN_PART2) {
		INFO_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client login failed.");
		m_state = State::NONE;
	} else {
		DEBUG_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client received unsequenced login reply.");
	}
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleResult&& msg) -> void {
	if (m_state == State::WAITING) {
		m_result = std::move(msg.value);
		m_state = State::RESULT_RECEIVED;
	} else if (msg.value.status == cmd::Status::VALUE || msg.value.status == cmd::Status::RETURN_VALUE || msg.value.status == cmd::Status::ERROR_MSG) {
		this->vm().output(std::move(msg.value));
	}
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleOutput&& msg) -> void {
	if (auto ptr = m_output.lock()) {
		ptr->write(std::move(msg.value));
	} else {
		this->vm().outputln(std::move(msg.value));
	}
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleDone &&) -> void {
	if (m_state == State::ABORTING) {
		m_state = State::READY;
	}
	this->resetRconOutput();
}

auto RemoteConsoleClient::handleMessage(msg::cl::in::RemoteConsoleLoggedOut &&) -> void {
	DEBUG_MSG(Msg::CLIENT | Msg::CONNECTION_EVENT | Msg::RCON, "Rcon client logged out.");
	if (m_state == State::READY) {
		this->vm().outputError("Remote console session timed out.");
		this->resetRconOutput();
	} else if (m_state != State::LOGOUT) {
		this->vm().outputError("Remote console session shut down.");
		this->resetRconOutput();
	}
	m_state = State::NONE;
}
