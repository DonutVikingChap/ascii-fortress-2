#include "game_client.hpp"

#include "../../console/commands/file_commands.hpp"             // data_dir, data_subdir_maps, data_subdir_sounds, data_subdir_screens
#include "../../console/commands/game_client_commands.hpp"      // address, port, username, password, cl_...
#include "../../console/commands/game_commands.hpp"             // game_version
#include "../../console/commands/inventory_client_commands.hpp" // cvar_hat
#include "../../console/commands/process_commands.hpp"          // cmd_import, cmd_file
#include "../../console/commands/sound_manager_commands.hpp"    // snd_attenuation, snd_distance
#include "../../console/con_command.hpp"                        // ConCommand, GET_COMMAND
#include "../../console/convar.hpp"                             // ConVar
#include "../../debug.hpp"                                      // Msg, DEBUG_MSG, DEBUG_MSG_INDENT, INFO_MSG
#include "../../gui/layout.hpp"                                 // gui::GRID_SIZE_..., gui::VIEWPORT_...
#include "../../network/byte_stream.hpp"                        // net::ByteInputStream
#include "../../network/config.hpp"                             // net::Duration, net::MAX_CHAT_MESSAGE_LENGTH, net::MAX_PACKET_SIZE
#include "../../network/delta.hpp"                              // deltaDecompress
#include "../../utilities/file.hpp"                             // util::readFile, util::dumpFile, util::pathIsBelowDirectory
#include "../../utilities/span.hpp"                             // util::Span, util::asBytes
#include "../../utilities/string.hpp"                           // util::toString
#include "../../utilities/time.hpp"                             // util::getLocalTimeStr
#include "../game.hpp"                                          // Game
#include "../shared/map.hpp"                                    // Map
#include "char_window.hpp"                                      // CharWindow
#include "input_manager.hpp"                                    // InputManager
#include "sound_manager.hpp"                                    // SoundManager

#include <algorithm>    // std::sort, std::clamp, std::min_element, std::remove, std::min
#include <array>        // std::array
#include <cassert>      // assert
#include <chrono>       // std::chrono::..
#include <cmath>        // std::ceil, std::round, std::sqrt
#include <fmt/core.h>   // fmt::format
#include <ios>          // std::ios
#include <ratio>        // std::milli
#include <system_error> // std::error_code

namespace {

constexpr auto CLIENT_COLOR_BORDER = Color::white();
constexpr auto CLIENT_COLOR_EVENT_MESSAGE = Color::gray();
constexpr auto CLIENT_COLOR_EVENT_MESSAGE_PERSONAL = Color::white();

} // namespace

auto GameClient::getConfigHeader() -> std::string {
	return fmt::format(
		"// This file is auto-generated every time your client is shut down, and loaded every time it is "
		"started.\n"
		"// Do not modify this file manually. Use the autoexec file instead.\n"
		"// Last generated {}.",
		util::getLocalTimeStr("%c"));
}

GameClient::GameClient(Game& game, VirtualMachine& vm, CharWindow& charWindow, SoundManager& soundManager, InputManager& inputManager)
	: m_game(game)
	, m_vm(vm)
	, m_charWindow(charWindow)
	, m_soundManager(soundManager)
	, m_inputManager(inputManager)
	, m_connection(m_socket, net::Duration{}, 0, 0, MessageHandler{*this})
	, m_serverTickrate(static_cast<Tickrate>(cl_cmdrate))
	, m_viewPosition(gui::GRID_SIZE_X / 2, gui::GRID_SIZE_Y / 2)
	, m_viewport(gui::VIEWPORT_X, gui::VIEWPORT_Y, gui::VIEWPORT_W, gui::VIEWPORT_H) {
	this->updateTimeout();
	this->updateThrottle();
	this->updateCommandInterval();
}

auto GameClient::init() -> bool {
	INFO_MSG(Msg::CLIENT, "Game client: Initializing...");

	// Initialize crypto library.
	if (!crypto::init()) {
		m_game.error("Failed to initialize crypto library!");
		return false;
	}

	// Initialize inventory client.
	if (!this->initInventoryClient()) {
		m_game.error("Failed to initialize inventory client!");
		return false;
	}

	// Initialize remote console client.
	if (!this->initRconClient()) {
		m_game.error("Failed to initialize remote console client!");
		return false;
	}

	m_game.println("Connecting to server...");

	auto ec = std::error_code{};

	// Bind socket.
	m_socket.bind(net::IpEndpoint{net::IpAddress::any(), static_cast<net::PortNumber>(cl_port)}, ec);
	if (ec) {
		if (cl_port == 0) {
			m_game.warning(fmt::format("Failed to bind client socket: {}", ec.message()));
		} else {
			m_game.warning(fmt::format("Failed to bind client socket using port {}: {}", cl_port, ec.message()));
		}
		return false;
	}

	const auto ip = net::IpAddress::resolve(std::string{address}.c_str(), ec);
	if (ec) {
		m_game.warning(fmt::format("Couldn't resolve ip address: {}", ec.message()));
		return false;
	}
	// Initialize connection.
	if (!m_connection.connect(net::IpEndpoint{ip, static_cast<net::PortNumber>(port)})) {
		m_game.error(fmt::format("Failed to intialize client connection: {}", m_connection.getDisconnectMessage()));
		return false;
	}

	// Execute client config script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{cl_config_file}}).status ==
		cmd::Status::ERROR_MSG) {
		m_game.error("Client config failed.");
		return false;
	}

	// Execute client autoexec script.
	if (m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{cl_autoexec_file}}).status ==
		cmd::Status::ERROR_MSG) {
		m_game.error("Client autoexec failed.");
		return false;
	}

	// Load sounds.
	for (const auto& id : SoundId::getAll()) {
		if (id != SoundId::none()) {
			if (!m_soundManager.loadSound(id, fmt::format("{}/{}/{}", data_dir, data_subdir_sounds, id.getFilename()).c_str())) {
				m_game.warning(fmt::format("Failed to load sound \"{}\"!", id.getFilename()));
			}
		}
	}

	// Load GUI.
	this->loadScreen(m_screen, "client.txt");
	this->loadScreen(m_teamSelectScreen, "team_select.txt");
	this->loadScreen(m_classSelectScreen, "class_select.txt");
	this->loadScreen(m_scoreboardScreen, "scoreboard.txt");

	INFO_MSG(Msg::CLIENT,
			 "Game client: \"{}\" connecting to \"{}\" using port {}...",
			 username,
			 std::string{m_connection.getRemoteEndpoint()},
			 m_socket.getLocalEndpoint(ec).getPort());
	return true;
}

auto GameClient::shutDown() -> void {
	INFO_MSG(Msg::CLIENT, "Game client: Shutting down.");

	m_soundManager.update(0.0f, 0.0f, 0.0f, 0.0f);

	if (m_game.gameServer()) {
		m_connection.close();
	}

	if (m_aimingUp) {
		m_game.consoleCommand(m_inputManager.releaseMouseLookUp());
	}
	if (m_aimingDown) {
		m_game.consoleCommand(m_inputManager.releaseMouseLookDown());
	}
	if (m_aimingLeft) {
		m_game.consoleCommand(m_inputManager.releaseMouseLookLeft());
	}
	if (m_aimingRight) {
		m_game.consoleCommand(m_inputManager.releaseMouseLookRight());
	}

	// Save client config.
	m_game.awaitConsoleCommand(GET_COMMAND(cl_writeconfig));

	// Unload map.
	if (!m_game.gameServer()) {
		m_game.map().unLoad();
	}

	// Unload sounds.
	for (const auto& id : SoundId::getAll()) {
		if (id != SoundId::none()) {
			m_soundManager.unloadSound(id);
		}
	}

	// Restore replicated cvars.
	for (auto& [name, cvar] : ConVar::all()) {
		if ((cvar.getFlags() & ConVar::REPLICATED) != 0) {
			if (const auto result = cvar.restoreLocalValue(m_game, nullptr, this, nullptr, nullptr); result.status == cmd::Status::ERROR_MSG) {
				m_game.warning(fmt::format("Failed to restore local value: {}", result.value));
			}
		}
	}
}

auto GameClient::handleEvent(const SDL_Event& e) -> void {
	if (e.type == SDL_MOUSEMOTION) {
		m_mousePosition = Vec2{e.motion.x, e.motion.y};
	}
}

auto GameClient::update(float deltaTime) -> bool {
	DEBUG_MSG_INDENT(Msg::CLIENT_TICK | Msg::CONNECTION_DETAILED, "CLIENT @ {} ms", deltaTime * 1000.0f) {
		// Receive packets and handle messages.
		this->receivePackets();
		if (!m_connection.update()) {
			const auto reason = m_connection.getDisconnectMessage();
			m_game.println((reason.empty()) ? "Disconnected." : fmt::format("Disconnected: {}", reason));
			return false;
		}

		if (cl_showping) {
			const auto pingMilliseconds =
				std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(m_connection.getLatestMeasuredPingDuration()).count();
			m_game.drawDebugString(fmt::format("Ping: {} ms", pingMilliseconds));
		}

		// Update viewport.
		if (m_game.map().getWidth() < m_viewport.w) {
			m_viewPosition.x = 0;
		} else {
			m_viewPosition.x = static_cast<Vec2::Length>(std::clamp(
				m_viewport.x + m_snapshot.selfPlayer.position.x - static_cast<int>(std::ceil(static_cast<float>(m_viewport.w) / 2.0f)),
				0,
				m_game.map().getWidth() - m_viewport.w));
		}

		if (m_game.map().getHeight() < m_viewport.h) {
			m_viewPosition.y = 0;
		} else {
			m_viewPosition.y = static_cast<Vec2::Length>(std::clamp(
				m_viewport.y + m_snapshot.selfPlayer.position.y - static_cast<int>(std::ceil(static_cast<float>(m_viewport.h) / 2.0f)),
				0,
				m_game.map().getHeight() - m_viewport.h));
		}

		m_soundManager.update(deltaTime,
							  static_cast<float>(m_snapshot.selfPlayer.position.x) * snd_attenuation,
							  static_cast<float>(m_snapshot.selfPlayer.position.y) * snd_attenuation,
							  0.0f);

		// Update mouselook.
		if (cl_mouselook) {
			const auto playerScreenPosition = m_charWindow.gridToScreenCoordinates(this->worldToGridCoordinates(m_snapshot.selfPlayer.position));
			const auto tileScreenSize = m_charWindow.gridToScreenSize(Vec2{1, 1});
			const auto playerMiddle = static_cast<Vector2<float>>(playerScreenPosition) + static_cast<Vector2<float>>(tileScreenSize) * 0.5f;
			const auto mousePosition = static_cast<Vector2<float>>(m_mousePosition);
			const auto aimVector = mousePosition - playerMiddle;
			const auto aimDirection = Direction{aimVector};

			if (!m_aimingLeft && aimDirection.hasLeft()) {
				m_game.consoleCommand(m_inputManager.pressMouseLookLeft());
				m_aimingLeft = true;
			} else if (m_aimingLeft && !aimDirection.hasLeft()) {
				m_game.consoleCommand(m_inputManager.releaseMouseLookLeft());
				m_aimingLeft = false;
			}

			if (!m_aimingRight && aimDirection.hasRight()) {
				m_game.consoleCommand(m_inputManager.pressMouseLookRight());
				m_aimingRight = true;
			} else if (m_aimingRight && !aimDirection.hasRight()) {
				m_game.consoleCommand(m_inputManager.releaseMouseLookRight());
				m_aimingRight = false;
			}

			if (!m_aimingUp && aimDirection.hasUp()) {
				m_game.consoleCommand(m_inputManager.pressMouseLookUp());
				m_aimingUp = true;
			} else if (m_aimingUp && !aimDirection.hasUp()) {
				m_game.consoleCommand(m_inputManager.releaseMouseLookUp());
				m_aimingUp = false;
			}

			if (!m_aimingDown && aimDirection.hasDown()) {
				m_game.consoleCommand(m_inputManager.pressMouseLookDown());
				m_aimingDown = true;
			} else if (m_aimingDown && !aimDirection.hasDown()) {
				m_game.consoleCommand(m_inputManager.releaseMouseLookDown());
				m_aimingDown = false;
			}
		}

		if (m_commandTimer.advance(deltaTime, m_commandInterval)) {
			DEBUG_MSG_INDENT(Msg::CLIENT_TICK | Msg::CONNECTION_DETAILED, "Game client command.") {
				if (this->hasJoinedGame()) {
					++m_userCmdNumber;
					DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game client wrote snapshot ack #{}.", m_snapshot.tickCount);
					if (!this->writeToGameServer(msg::sv::out::UserCmd{{}, m_userCmdNumber, m_snapshot.tickCount, m_inputManager.getActions()})) {
						m_connection.close("Failed to write usercmd.");
					}
				}
				m_connection.sendPackets();
			}
		}
	}
	return true;
}

auto GameClient::draw() const -> void {
	const auto& self = m_snapshot.selfPlayer;

	// Border.
	m_charWindow.draw(Vec2{0, 0}, m_screen, CLIENT_COLOR_BORDER);

	// Map.
	m_charWindow.draw(m_viewport.getPosition(),
					  m_game.map(),
					  {m_viewPosition.x, m_viewPosition.y, m_viewport.w, m_viewport.h},
					  cl_color_world,
					  cl_color_non_solid,
					  self.team == Team::red(),
					  self.team == Team::blue(),
					  cl_char_track,
					  cl_color_track,
					  cl_char_respawnvis,
					  cl_color_respawnvis,
					  cl_char_resupply,
					  cl_color_resupply);

	// Generic entities.
	for (const auto& genericEntity : m_snapshot.genericEntities) {
		this->drawGenericEntity(genericEntity.position, genericEntity.color, genericEntity.matrix);
	}

	// Corpses.
	for (const auto& corpse : m_snapshot.corpses) {
		this->drawCorpse(corpse.position, corpse.team.getColor());
	}

	// Medkits.
	for (const auto& medkit : m_snapshot.medkits) {
		this->drawMedkit(medkit.position);
	}

	// Ammopacks.
	for (const auto& ammopack : m_snapshot.ammopacks) {
		this->drawAmmopack(ammopack.position);
	}

	// Other players.
	const auto mouseWorldPosition = this->gridToWorldCoordinates(m_charWindow.screenToGridCoordinates(m_mousePosition));
	for (const auto& ply : m_snapshot.players) {
		// Target id.
		if ((self.team == Team::spectators() && cl_draw_playernames_spectator) || (self.team == ply.team && cl_draw_playernames_friendly) ||
			(self.team != ply.team && cl_draw_playernames_enemy)) {
			if (Rect{ply.position.x - 2, ply.position.y - 2, 5, 5}.contains(mouseWorldPosition)) {
				this->drawChar(Vec2{ply.position.x, ply.position.y - 1}, cl_color_name, 'v');
				this->drawString(Vec2{ply.position.x - static_cast<Vec2::Length>(ply.name.size()) / 2, ply.position.y - 2}, cl_color_name, ply.name);
			}
		}

		// Other player.
		this->drawPlayer(ply.position, ply.team.getColor(), ply.aimDirection, ply.playerClass, ply.hat);
	}

	// Self player.
	if (self.team != Team::spectators() && self.alive) {
		this->drawPlayer(self.position, self.skinTeam.getColor(), self.aimDirection, self.playerClass, self.hat);
	}

	// Sentry guns.
	for (const auto& sentryGun : m_snapshot.sentryGuns) {
		this->drawSentryGun(sentryGun.position, sentryGun.team.getColor(), sentryGun.aimDirection);
	}

	// Projectiles.
	for (const auto& projectile : m_snapshot.projectiles) {
		this->drawProjectile(projectile.position, projectile.team.getColor(), projectile.type);
	}

	// Explosions.
	for (const auto& explosion : m_snapshot.explosions) {
		this->drawExplosion(explosion.position, explosion.team.getColor());
	}

	// Flags.
	for (const auto& flag : m_snapshot.flags) {
		this->drawFlag(flag.position, flag.team.getColor());
	}

	// Carts.
	for (const auto& cart : m_snapshot.carts) {
		this->drawCart(cart.position, cart.team.getColor());
	}

	// Scores.
	const auto scoreX = m_viewport.x;
	auto scoreY = m_viewport.y;
	for (const auto& cartInfo : m_snapshot.cartInfo) {
		constexpr auto CART_PROGRESS_WIDTH = Vec2::Length{16};

		const auto cartOffset = (cartInfo.progress * (CART_PROGRESS_WIDTH - 1)) / cartInfo.trackLength;
		const auto color = cartInfo.team.getColor();
		m_charWindow.draw(Vec2{scoreX, scoreY}, '[', color);
		m_charWindow.draw(Vec2{scoreX + 1 + CART_PROGRESS_WIDTH, scoreY}, ']', color);
		m_charWindow.drawLineHorizontal(Vec2{scoreX + 1, scoreY}, static_cast<Vec2::Length>(cartOffset), '=', color);
		m_charWindow.draw(Vec2{scoreX + 1 + cartOffset, scoreY}, 'P', color);
		m_charWindow.drawLineHorizontal(Vec2{scoreX + 2 + cartOffset, scoreY},
										static_cast<Vec2::Length>(CART_PROGRESS_WIDTH - 1 - cartOffset),
										'=',
										Color::gray());
		++scoreY;
	}
	for (const auto& flagInfo : m_snapshot.flagInfo) {
		const auto scoreStr = fmt::format("{} score: {}", flagInfo.team.getName(), flagInfo.score);
		m_charWindow.draw(Vec2{scoreX, scoreY}, scoreStr, flagInfo.team.getColor());
		++scoreY;
	}

	const auto teamColor = self.team.getColor();

	// Crosshair.
	if (cl_crosshair_enable && self.alive) {
		if (const auto aimVector = self.aimDirection.getVector(); aimVector != Vec2{0, 0}) {
			const auto crosshairTarget = [&] {
				const auto aimLengthSquared = aimVector.lengthSquared();
				const auto aimLength = std::sqrt(static_cast<float>(aimLengthSquared));
				if (cl_mouselook && cl_crosshair_distance_follow_cursor) {
					const auto playerToCursor = mouseWorldPosition - self.position;
					const auto projectedVector = (Vec2::dotProduct(playerToCursor, aimVector) / aimLengthSquared) * aimVector;

					if (cl_crosshair_min_distance <= cl_crosshair_max_distance) {
						const auto distance = std::clamp(static_cast<float>(projectedVector.length()),
														 static_cast<float>(cl_crosshair_min_distance),
														 static_cast<float>(cl_crosshair_max_distance));

						const auto offset = static_cast<Vec2::Length>(std::round(distance / aimLength)) * aimVector;
						return self.position + offset;
					}
					return self.position + projectedVector;
				}

				const auto offset = static_cast<Vec2::Length>(std::round(cl_crosshair_max_distance / aimLength)) * aimVector;
				return self.position + offset;
			}();

			const auto crosshairPosition = [&] {
				if (cl_crosshair_collide_world || cl_crosshair_collide_viewport) {
					const auto crosshairViewport = Rect{m_viewport.x + static_cast<Vec2::Length>(cl_crosshair_viewport_border),
														m_viewport.y + static_cast<Vec2::Length>(cl_crosshair_viewport_border),
														m_viewport.w - static_cast<Vec2::Length>(cl_crosshair_viewport_border) * 2,
														m_viewport.h - static_cast<Vec2::Length>(cl_crosshair_viewport_border) * 2};

					const auto testCollision = [&](Vec2 position) {
						if (cl_crosshair_collide_world && m_game.map().isSolid(position, self.team == Team::red(), self.team == Team::blue())) {
							return true;
						}
						if (cl_crosshair_collide_viewport && !crosshairViewport.contains(this->worldToGridCoordinates(position)) &&
							crosshairViewport.contains(this->worldToGridCoordinates(self.position)) &&
							static_cast<float>(Vec2::distance(position, self.position)) > cl_crosshair_min_distance) {
							return true;
						}
						return false;
					};

					auto position = self.position;
					if (!testCollision(position)) {
						while (position != crosshairTarget) {
							const auto newPosition = position + aimVector;
							if (testCollision(newPosition)) {
								break;
							}
							position = newPosition;
						}
					}
					return position;
				}

				return crosshairTarget;
			}();

			const auto& crosshairColor = (cl_crosshair_use_team_color) ? teamColor : cl_crosshair_color;

			if (crosshairPosition != self.position) {
				this->drawChar(crosshairPosition, crosshairColor, cl_crosshair);
			}
		}
	}

	if (self.team != Team::spectators()) {
		// Ammo.
		const auto ammoPosition = Vec2{m_viewport.x + m_viewport.w / 2 + 1, m_viewport.y + m_viewport.h - 1};
		const auto ammoStr1 = util::toString(self.primaryAmmo);
		m_charWindow.draw(ammoPosition, "Ammo: ", teamColor);
		m_charWindow.draw(Vec2{ammoPosition.x + 6, ammoPosition.y}, ammoStr1, cl_color_ammo);
		if (self.playerClass.getSecondaryWeapon().getAmmoPerClip() != 0) {
			const auto ammoStr1Width = static_cast<Vec2::Length>(ammoStr1.size());
			const auto ammoStr2 = util::toString(self.secondaryAmmo);
			m_charWindow.draw(Vec2{ammoPosition.x + 6 + ammoStr1Width, ammoPosition.y}, '|', teamColor);
			m_charWindow.draw(Vec2{ammoPosition.x + 7 + ammoStr1Width, ammoPosition.y}, ammoStr2, cl_color_ammo);
		}

		// Health.
		const auto healthPosition = Vec2{m_viewport.x + m_viewport.w / 2 - 11, m_viewport.y + m_viewport.h - 1};
		m_charWindow.draw(healthPosition, "Health: ", teamColor);
		m_charWindow.draw(Vec2{healthPosition.x + 8, healthPosition.y},
						  util::toString(self.health),
						  (self.health < self.playerClass.getHealth() / 2) ? cl_color_low_health : cl_color_health);
	}

	// Timer.
	const auto timePosition = Vec2{m_viewport.x + m_viewport.w / 2 - 2, m_viewport.y};
	m_charWindow.draw(timePosition, fmt::format("{:02}:{:02}", m_snapshot.roundSecondsLeft / 60, m_snapshot.roundSecondsLeft % 60), cl_color_timer);

	// Class select.
	if (!m_classSelected) {
		m_charWindow.draw(Vec2{0, 0}, m_classSelectScreen, CLIENT_COLOR_BORDER);
	}

	// Team select.
	if (!m_teamSelected) {
		m_charWindow.draw(Vec2{0, 0}, m_teamSelectScreen, CLIENT_COLOR_BORDER);
	}

	// Scoreboard.
	if (cl_showscores) {
		m_charWindow.draw(Vec2{0, 0}, m_scoreboardScreen, CLIENT_COLOR_BORDER);

		auto redPlayers = std::vector<const ent::sh::PlayerInfo*>{};
		auto bluePlayers = std::vector<const ent::sh::PlayerInfo*>{};
		auto spectators = std::vector<const ent::sh::PlayerInfo*>{};
		for (const auto& ply : m_snapshot.playerInfo) {
			switch (ply.team) {
				case Team::red(): redPlayers.push_back(&ply); break;
				case Team::blue(): bluePlayers.push_back(&ply); break;
				default: spectators.push_back(&ply); break;
			}
		}
		std::sort(redPlayers.begin(), redPlayers.end(), [](const auto* lhs, const auto* rhs) { return lhs->score > rhs->score; });
		std::sort(bluePlayers.begin(), bluePlayers.end(), [](const auto* lhs, const auto* rhs) { return lhs->score > rhs->score; });
		std::sort(spectators.begin(), spectators.end(), [](const auto* lhs, const auto* rhs) { return lhs->score > rhs->score; });

		auto y = Vec2::Length{7};

		if (!redPlayers.empty()) {
			for (const auto& player : redPlayers) {
				m_charWindow.draw(Vec2{6, y}, player->name, Team::red().getColor());
				m_charWindow.draw(Vec2{23, y}, player->playerClass.getName(), Team::red().getColor());
				m_charWindow.draw(Vec2{36, y}, util::toString(player->score), Team::red().getColor());
				m_charWindow.draw(Vec2{45, y}, util::toString(player->ping), Team::red().getColor());
				++y;
			}
			++y;
		}

		if (!bluePlayers.empty()) {
			for (const auto& player : bluePlayers) {
				m_charWindow.draw(Vec2{6, y}, player->name, Team::blue().getColor());
				m_charWindow.draw(Vec2{23, y}, player->playerClass.getName(), Team::blue().getColor());
				m_charWindow.draw(Vec2{36, y}, util::toString(player->score), Team::blue().getColor());
				m_charWindow.draw(Vec2{45, y}, util::toString(player->ping), Team::blue().getColor());
				++y;
			}
			++y;
		}

		for (const auto& player : spectators) {
			m_charWindow.draw(Vec2{6, y}, player->name, Team::spectators().getColor());
			m_charWindow.draw(Vec2{23, y}, player->playerClass.getName(), Team::spectators().getColor());
			m_charWindow.draw(Vec2{36, y}, util::toString(player->score), Team::spectators().getColor());
			m_charWindow.draw(Vec2{45, y}, util::toString(player->ping), Team::spectators().getColor());
			++y;
		}
	}
}

auto GameClient::toggleTeamSelect() -> void {
	m_teamSelected = !m_teamSelected;
}

auto GameClient::toggleClassSelect() -> void {
	m_classSelected = !m_classSelected;
}

auto GameClient::disconnect() -> void {
	m_connection.disconnect("Disconnect by user.");
}

auto GameClient::updateTimeout() -> void {
	m_connection.setTimeout(std::chrono::duration_cast<net::Duration>(std::chrono::duration<float>{static_cast<float>(cl_timeout)}));
}

auto GameClient::updateThrottle() -> void {
	m_connection.setThrottleMaxSendBufferSize(cl_throttle_limit);
	m_connection.setThrottleMaxPeriod(cl_throttle_max_period);
}

auto GameClient::updateCommandInterval() -> void {
	m_commandInterval = 1.0f / std::min(static_cast<float>(cl_cmdrate), static_cast<float>(m_serverTickrate));
	m_commandTimer.reset();
}

auto GameClient::updateUpdateRate() -> bool {
	return this->writeToGameServer(msg::sv::out::UpdateRateChange{{}, static_cast<Tickrate>(cl_updaterate)});
}

auto GameClient::updateUsername() -> bool {
	return this->writeToGameServer(msg::sv::out::UsernameChange{{}, std::string{username}});
}

auto GameClient::writeChatMessage(std::string_view message) -> bool {
	if (!message.empty()) {
		return this->writeToGameServer(msg::sv::out::ChatMessage{{}, std::string{message.substr(0, net::MAX_CHAT_MESSAGE_LENGTH)}});
	}
	return true;
}

auto GameClient::writeTeamChatMessage(std::string_view message) -> bool {
	if (!message.empty()) {
		return this->writeToGameServer(msg::sv::out::TeamChatMessage{{}, std::string{message.substr(0, net::MAX_CHAT_MESSAGE_LENGTH)}});
	}
	return true;
}

auto GameClient::teamSelect(Team team) -> bool {
	m_teamSelected = true;
	m_selectedTeam = team;
	if (m_selectedTeam == Team::spectators()) {
		return this->classSelect(PlayerClass::spectator());
	}
	m_classSelected = false;
	return true;
}

auto GameClient::teamSelectAuto() -> bool {
	static_assert(Team::getAll().size() >= 3, "No teams to choose from!");
	auto teamPlayerCounts = std::unordered_map<Team, int>{};
	for (const auto& team : Team::getAll()) {
		if (team != Team::none() && team != Team::spectators()) {
			teamPlayerCounts.emplace(team, 0);
		}
	}
	for (const auto& player : m_snapshot.playerInfo) {
		if (player.team != Team::none() && player.team != Team::spectators()) {
			++teamPlayerCounts.at(player.team);
		}
	}

	const auto it = std::min_element(teamPlayerCounts.begin(), teamPlayerCounts.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.second < rhs.second;
	});
	assert(it != teamPlayerCounts.end());
	return this->teamSelect(it->first);
}

auto GameClient::teamSelectRandom() -> bool {
	static_assert(Team::getAll().size() >= 3, "No teams to choose from!");
	auto teams = Team::getAll();

	auto end = std::remove(teams.begin(), teams.end(), Team::none());
	end = std::remove(teams.begin(), end, Team::spectators());
	return this->teamSelect(teams[static_cast<std::size_t>(m_vm.randomInt(0, static_cast<int>(end - teams.begin() - 1)))]);
}

auto GameClient::classSelect(PlayerClass playerClass) -> bool {
	if (m_teamSelected) {
		if (!this->writeToGameServer(msg::sv::out::TeamSelect{{}, m_selectedTeam, playerClass})) {
			return false;
		}
	}
	m_classSelected = true;
	return true;
}

auto GameClient::classSelectAuto() -> bool {
	static_assert(PlayerClass::getAll().size() >= 3, "No classes to choose from!");
	auto classPlayerCounts = std::unordered_map<PlayerClass, int>{};
	for (const auto& playerClass : PlayerClass::getAll()) {
		if (playerClass != PlayerClass::none() && playerClass != PlayerClass::spectator()) {
			classPlayerCounts.emplace(playerClass, 0);
		}
	}
	for (const auto& player : m_snapshot.playerInfo) {
		if (player.team == m_selectedTeam && player.playerClass != PlayerClass::none() && player.playerClass != PlayerClass::spectator()) {
			++classPlayerCounts.at(player.playerClass);
		}
	}

	const auto it = std::min_element(classPlayerCounts.begin(), classPlayerCounts.end(), [](const auto& lhs, const auto& rhs) {
		return lhs.second < rhs.second;
	});
	assert(it != classPlayerCounts.end());
	return this->classSelect(it->first);
}

auto GameClient::classSelectRandom() -> bool {
	static_assert(PlayerClass::getAll().size() >= 3, "No classes to choose from!");
	auto playerClasses = PlayerClass::getAll();

	auto end = std::remove(playerClasses.begin(), playerClasses.end(), PlayerClass::none());
	end = std::remove(playerClasses.begin(), end, PlayerClass::spectator());
	return this->classSelect(playerClasses[static_cast<std::size_t>(m_vm.randomInt(0, static_cast<int>(end - playerClasses.begin() - 1)))]);
}

auto GameClient::forwardCommand(cmd::CommandView argv) -> bool {
	auto command = std::vector<std::string>{};
	command.reserve(argv.size());
	for (const auto& arg : argv) {
		command.push_back(arg);
	}
	return this->writeToGameServer(msg::sv::out::ForwardedCommand{{}, command});
}

auto GameClient::hasJoinedGame() const -> bool {
	return m_playerId != PLAYER_ID_UNCONNECTED;
}

auto GameClient::hasSelectedTeam() const -> bool {
	return m_teamSelected;
}

auto GameClient::getPlayerId() const -> PlayerId {
	return m_playerId;
}

auto GameClient::worldToGridCoordinates(Vec2 position) const -> Vec2 {
	return Vec2{m_viewport.x, m_viewport.y} + position - m_viewPosition;
}

auto GameClient::gridToWorldCoordinates(Vec2 position) const -> Vec2 {
	return position - Vec2{m_viewport.x, m_viewport.y} + m_viewPosition;
}

auto GameClient::getStatusString() const -> std::string {
	auto ec = std::error_code{};
	const auto pingMilliseconds =
		std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(m_connection.getLatestMeasuredPingDuration()).count();
	return fmt::format(
		"=== CLIENT STATUS ===\n"
		"Local address: \"{}:{}\"\n"
		"Server address: \"{}\"\n"
		"Latency: {} ms\n"
		"Command rate: {} Hz\n"
		"Update rate: {} Hz\n"
		"Server tick rate: {} Hz\n"
		"Server tick count: {}\n"
		"Map: \"{}\"\n"
		"Packets sent: {}\n"
		"Packets received: {}\n"
		"Reliable packets written: {}\n"
		"Reliable packets received: {}\n"
		"Reliable packets received out of order: {}\n"
		"Send rate throttled: {}\n"
		"Packet send errors: {}\n"
		"Invalid message types received: {}\n"
		"Invalid message payloads received: {}\n"
		"Invalid packet headers received: {}\n"
		"=====================",
		std::string{net::IpAddress::getLocalAddress(ec)},
		m_socket.getLocalEndpoint(ec).getPort(),
		std::string{m_connection.getRemoteEndpoint()},
		pingMilliseconds,
		1.0f / m_commandInterval,
		(cl_updaterate > 0) ? cl_updaterate : m_serverTickrate,
		m_serverTickrate,
		m_snapshot.tickCount,
		m_game.map().getName(),
		m_connection.getStats().packetsSent,
		m_connection.getStats().packetsReceived,
		m_connection.getStats().reliablePacketsWritten,
		m_connection.getStats().reliablePacketsReceived,
		m_connection.getStats().reliablePacketsReceivedOutOfOrder,
		m_connection.getStats().sendRateThrottleCount,
		m_connection.getStats().packetSendErrorCount,
		m_connection.getStats().invalidMessageTypeCount,
		m_connection.getStats().invalidMessagePayloadCount,
		m_connection.getStats().invalidPacketHeaderCount);
}

auto GameClient::handleMessage(net::msg::in::Connect&&) -> void {
	m_game.println("Connected.");
	m_game.println("Retrieving server info...");
	if (!this->writeToGameServer(msg::sv::out::ServerInfoRequest{{}})) {
		m_connection.disconnect("Failed to send server info request.");
	}
}

auto GameClient::handleMessage(msg::cl::in::ServerInfo&& msg) -> void {
	INFO_MSG(Msg::CLIENT, "Game client: Received info from server.");

	if (this->hasJoinedGame()) {
		m_game.println(fmt::format("Server is changing level to \"{}\"...", net::sanitizeMessage(msg.mapName)));
		m_playerId = PLAYER_ID_UNCONNECTED;
	}

	INFO_MSG(Msg::CLIENT, "Game client: Server host name is \"{}\".", net::sanitizeMessage(msg.hostName));
	INFO_MSG(Msg::CLIENT, "Game client: Server game version is \"{}\".", net::sanitizeMessage(msg.gameVersion));

	if (msg.tickrate <= 0) {
		m_game.warning("Invalid tickrate received from server.");
	} else {
		m_serverTickrate = msg.tickrate;
		this->updateCommandInterval();
		INFO_MSG(Msg::CLIENT, "Game client: Server tickrate is {} Hz.", m_serverTickrate);
	}

	m_serverMapName = std::move(msg.mapName);
	INFO_MSG(Msg::CLIENT, "Game client: Server is running map \"{}\".", net::sanitizeMessage(m_serverMapName));

	m_serverPasswordSalt = msg.passwordSalt;
	m_serverPasswordHashType = msg.passwordHashType;

	m_snapshot = Snapshot{};
	m_userCmdNumber = 0;
	m_teamSelected = false;
	m_classSelected = false;
	m_selectedTeam = Team::spectators();

	m_resourceDownloadQueue.clear();

	// Check resources.
	for (const auto& resource : msg.resources) {
		const auto filepath = fmt::format("{}/{}", data_dir, resource.name);
		if (!util::pathIsBelowDirectory(filepath, data_dir)) {
			m_connection.disconnect("Server tried to access a resource outside of the game directory.");
			return;
		}

		const auto openmode = (resource.isText) ? std::ios::in : std::ios::in | std::ios::binary;

		auto buf = util::readFile(filepath, openmode);
		if (!buf) {
			const auto filepathInDownloads = fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, resource.name);
			if (!util::pathIsBelowDirectory(filepathInDownloads, fmt::format("{}/{}", data_dir, data_subdir_downloads))) {
				m_connection.disconnect("Server tried to access a resource outside of the game directory.");
				return;
			}
			buf = util::readFile(filepathInDownloads, openmode);
		}
		if (buf) {
			if (util::CRC32{util::asBytes(util::Span{*buf})} != resource.fileHash) {
				const auto resourceName = net::sanitizeMessage(resource.name);
				if (cl_allow_resource_download) {
					if (resource.canDownload) {
						m_connection.disconnect(
							fmt::format("Your version of {} differs from the server's. Remove the file to download the server's version.", resourceName));
					} else {
						m_connection.disconnect(
							fmt::format("Your version of {} differs from the server's. The server does not provide the file for download.", resourceName));
					}
				} else {
					m_connection.disconnect(fmt::format("Your version of {} differs from the server's. Resource downloads are disabled.", resourceName));
				}
				return;
			}
		} else {
			const auto resourceName = net::sanitizeMessage(resource.name);
			if (cl_allow_resource_download) {
				if (resource.canDownload) {
					if (cl_max_resource_download_size != 0 && resource.size > static_cast<std::size_t>(cl_max_resource_download_size)) {
						m_connection.disconnect(
							fmt::format("Resource {} exceeds the maximum download size ({}/{}).", resourceName, resource.size, cl_max_resource_download_size));
						return;
					}

					m_resourceDownloadQueue.emplace_back(resource.name, resource.nameHash, resource.fileHash, resource.size, resource.isText);
				} else {
					m_connection.disconnect(fmt::format("Missing resource {}. The server does not provide the file for download.", resourceName));
					return;
				}
			} else {
				m_connection.disconnect(fmt::format("Missing resource {}. Resource downloads are disabled.", resourceName));
				return;
			}
		}
	}

	if (m_resourceDownloadQueue.empty()) {
		this->joinGame();
	} else {
		auto totalDownloadSize = std::size_t{0};
		for (const auto& resourceDownload : m_resourceDownloadQueue) {
			totalDownloadSize += resourceDownload.size;
		}

		if (cl_max_resource_total_download_size != 0 && totalDownloadSize > static_cast<std::size_t>(cl_max_resource_total_download_size)) {
			m_connection.disconnect(fmt::format("Total resources size exceeds the maximum total download size ({}/{}).",
												totalDownloadSize,
												cl_max_resource_total_download_size));
			return;
		}

		auto message = fmt::format("Downloading resources ({} bytes total)...", totalDownloadSize);
		INFO_MSG(Msg::CLIENT, "{}", message);
		m_game.println(std::move(message));
		this->downloadNextResourceInQueue();
	}
}

auto GameClient::handleMessage(msg::cl::in::Joined&& msg) -> void {
	m_playerId = msg.playerId;
	INFO_MSG(Msg::CLIENT, "Game client: Successfully joined server with player id \"{}\".", msg.playerId);
	auto& inventory = this->getInventory(m_connection.getRemoteEndpoint());
	inventory.id = msg.inventoryId;
	inventory.token = msg.inventoryToken;
	if (!msg.motd.empty()) {
		m_game.println(net::sanitizeMessage(msg.motd, true));
	}
	if (!this->writeInventoryEquipHatRequest(Hat::findByName(cvar_hat))) {
		m_game.warning("Failed to write equip hat request!");
	}
}

auto GameClient::handleMessage(msg::cl::in::Snapshot&& msg) -> void {
	if (msg.snapshot.tickCount > m_snapshot.tickCount) {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game client: Received new full snapshot #{}.", msg.snapshot.tickCount);
		m_snapshot = std::move(msg.snapshot);
	} else {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game client: Received old snapshot #{}.", msg.snapshot.tickCount);
	}
}

auto GameClient::handleMessage(msg::cl::in::SnapshotDelta&& msg) -> void {
	if (msg.source == m_snapshot.tickCount) {
		auto deltaDataStream = net::ByteInputStream{msg.data};
		if (!deltaDecompress(deltaDataStream, m_snapshot)) {
			INFO_MSG(Msg::CLIENT, "Game client: Failed to read delta-compressed snapshot!");
		} else {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game client: Received snapshot delta from #{} to #{}.", msg.source, m_snapshot.tickCount);
		}
	} else {
		DEBUG_MSG(Msg::CONNECTION_DETAILED, "Game client: Received snapshot delta from invalid source #{}.", msg.source);
	}
}

auto GameClient::handleMessage(msg::cl::in::CvarMod&& msg) -> void {
	if (!m_game.gameServer()) {
		for (auto& modifiedCvar : msg.cvars) {
			if (auto&& cvar = ConVar::find(std::move(modifiedCvar.name))) {
				if ((cvar->getFlags() & ConVar::REPLICATED) != 0) {
					if (const auto result = cvar->overrideLocalValue(modifiedCvar.newValue, m_game, nullptr, this, nullptr, nullptr);
						result.status == cmd::Status::ERROR_MSG) {
						m_game.warning(fmt::format("Failed to override local value: {}", result.value));
					}
				} else {
					m_game.warning(fmt::format("Server asked local client to set non-networked cvar \"{}\".", cvar->getName()));
				}
			}
		}
	}
}

auto GameClient::handleMessage(msg::cl::in::ServerEventMessage&& msg) -> void {
	m_game.println(net::sanitizeMessage(msg.message), CLIENT_COLOR_EVENT_MESSAGE);
}

auto GameClient::handleMessage(msg::cl::in::ServerEventMessagePersonal&& msg) -> void {
	m_game.println(net::sanitizeMessage(msg.message), CLIENT_COLOR_EVENT_MESSAGE_PERSONAL);
}

auto GameClient::handleMessage(msg::cl::in::ChatMessage&& msg) -> void {
	if (cl_chat_enable) {
		if (const auto* const player = m_snapshot.findPlayerInfo(msg.sender)) {
			m_game.println(fmt::format("[CHAT] {}: {}", player->name, net::sanitizeMessage(msg.message)), player->team.getColor());
		} else {
			m_game.println(fmt::format("[CHAT] ???: {}", net::sanitizeMessage(msg.message)));
		}
	}
}

auto GameClient::handleMessage(msg::cl::in::TeamChatMessage&& msg) -> void {
	if (cl_chat_enable) {
		if (const auto* const player = m_snapshot.findPlayerInfo(msg.sender)) {
			m_game.println(fmt::format("[TEAM CHAT] {}: {}", player->name, net::sanitizeMessage(msg.message)), player->team.getColor());
		} else {
			m_game.println(fmt::format("[TEAM CHAT] ???: {}", net::sanitizeMessage(msg.message)));
		}
	}
}

auto GameClient::handleMessage(msg::cl::in::ServerChatMessage&& msg) -> void {
	m_game.println(fmt::format("[SERVER] {}", net::sanitizeMessage(msg.message)), Color::orange());
}

auto GameClient::handleMessage(msg::cl::in::PleaseSelectTeam&&) -> void {
	m_teamSelected = false;
}

auto GameClient::handleMessage(msg::cl::in::PlaySoundUnreliable&& msg) -> void {
	m_soundManager.playSoundAtRelativePosition(msg.id, 0.0f, 0.0f, -snd_distance, 1.0f);
}

auto GameClient::handleMessage(msg::cl::in::PlaySoundReliable&& msg) -> void {
	this->handleMessage(msg::cl::in::PlaySoundUnreliable{{}, msg.id});
}

auto GameClient::handleMessage(msg::cl::in::PlaySoundPositionalUnreliable&& msg) -> void {
	m_soundManager.playSoundAtPosition(msg.id,
									   static_cast<float>(msg.position.x) * snd_attenuation,
									   static_cast<float>(msg.position.y) * snd_attenuation,
									   -snd_distance,
									   1.0f);
}

auto GameClient::handleMessage(msg::cl::in::PlaySoundPositionalReliable&& msg) -> void {
	this->handleMessage(msg::cl::in::PlaySoundPositionalUnreliable{{}, msg.id, msg.position});
}

auto GameClient::handleMessage(msg::cl::in::ResourceDownloadPart&& msg) -> void {
	if (m_resourceDownloadQueue.empty() || m_resourceDownloadQueue.front().nameHash != msg.nameHash) {
		m_connection.disconnect("Server tried to send an unknown resource.");
		return;
	}

	auto& resourceDownload = m_resourceDownloadQueue.front();
	const auto resourceName = net::sanitizeMessage(resourceDownload.name);
	const auto newSize = resourceDownload.data.size() + msg.part.size();
	if (cl_max_resource_download_size != 0 && newSize > resourceDownload.size) {
		m_connection.disconnect(
			fmt::format("Resource \"{}\" exceeded the expected download size ({}/{}).", resourceName, newSize, resourceDownload.size));
		return;
	}
	resourceDownload.data.insert(resourceDownload.data.end(), msg.part.begin(), msg.part.end());
	auto message = fmt::format("Downloading {} ({}/{})...", resourceName, newSize, resourceDownload.size);
	INFO_MSG(Msg::CLIENT, "{}", message);
	m_game.println(std::move(message));
}

auto GameClient::handleMessage(msg::cl::in::ResourceDownloadLast&& msg) -> void {
	if (m_resourceDownloadQueue.empty() || m_resourceDownloadQueue.front().nameHash != msg.nameHash) {
		m_connection.disconnect("Server tried to send an unknown resource.");
		return;
	}

	auto& resourceDownload = m_resourceDownloadQueue.front();
	const auto resourceName = net::sanitizeMessage(resourceDownload.name);
	const auto newSize = resourceDownload.data.size() + msg.part.size();
	if (cl_max_resource_download_size != 0 && newSize != resourceDownload.size) {
		m_connection.disconnect(
			fmt::format("Resource \"{}\" did not match the expected download size ({}/{}).", resourceName, newSize, resourceDownload.size));
		return;
	}
	resourceDownload.data.insert(resourceDownload.data.end(), msg.part.begin(), msg.part.end());

	auto message = fmt::format("Downloaded {} ({} bytes).", resourceName, newSize);
	INFO_MSG(Msg::CLIENT, "{}", message);
	m_game.println(std::move(message));
	if (util::CRC32{util::asBytes(util::Span{resourceDownload.data})} != resourceDownload.fileHash) {
		m_connection.disconnect(fmt::format("Resource \"{}\" did not match the expected hash.", resourceName));
		return;
	}

	const auto openmode = (resourceDownload.isText) ? std::ios::trunc : std::ios::trunc | std::ios::binary;
	if (!util::dumpFile(fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, resourceDownload.name), resourceDownload.data, openmode)) {
		m_connection.disconnect(fmt::format("Failed to write file for resource \"{}\"!", resourceName));
		return;
	}

	m_resourceDownloadQueue.pop_front();
	if (m_resourceDownloadQueue.empty()) {
		this->joinGame();
	} else {
		this->downloadNextResourceInQueue();
	}
}

auto GameClient::handleMessage(msg::cl::in::PlayerTeamSelected&& msg) -> void {
	m_teamSelected = true;
	m_classSelected = true;
	m_selectedTeam = msg.newTeam;
}

auto GameClient::handleMessage(msg::cl::in::PlayerClassSelected&& msg) -> void {
	m_teamSelected = true;
	m_classSelected = true;
	if (const auto& scriptPath = msg.newPlayerClass.getScriptPath(); !scriptPath.empty()) {
		m_game.consoleCommand(GET_COMMAND(import), std::array{cmd::Value{GET_COMMAND(file).getName()}, cmd::Value{scriptPath}});
	}
}

auto GameClient::handleMessage(msg::cl::in::CommandOutput&& msg) -> void {
	if (msg.error) {
		m_vm.outputError(std::move(msg.str));
	} else {
		m_vm.outputln(std::move(msg.str));
	}
}

auto GameClient::handleMessage(msg::cl::in::HitConfirmed&& msg) -> void {
	if (cl_hitsound_enable) {
		if (msg.damage > 0) {
			this->handleMessage(msg::cl::in::PlaySoundUnreliable{{}, SoundId::hitsound()});
		}
	}
}

auto GameClient::vm() -> VirtualMachine& {
	return m_vm;
}

auto GameClient::write(msg::sv::out::InventoryEquipHatRequest&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::write(msg::sv::out::RemoteConsoleLoginInfoRequest&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::write(msg::sv::out::RemoteConsoleLoginRequest&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::write(msg::sv::out::RemoteConsoleCommand&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::write(msg::sv::out::RemoteConsoleAbortCommand&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::write(msg::sv::out::RemoteConsoleLogout&& msg) -> bool {
	return this->writeToGameServer(msg);
}

auto GameClient::downloadNextResourceInQueue() -> void {
	assert(!m_resourceDownloadQueue.empty());
	INFO_MSG(Msg::CLIENT, "Game client: Acquiring resource \"{}\" from server.", net::sanitizeMessage(m_resourceDownloadQueue.front().name));
	if (!this->writeToGameServer(msg::sv::out::ResourceDownloadRequest{{}, m_resourceDownloadQueue.front().nameHash})) {
		m_connection.disconnect("Failed to write resource download request.");
	}
}

auto GameClient::joinGame() -> void {
	// Load map.
	if (m_game.map().getName() != m_serverMapName) {
		const auto mapName = net::sanitizeMessage(m_serverMapName);

		INFO_MSG(Msg::CLIENT, "Game client: Loading map \"{}\"...", mapName);
		const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_maps, m_serverMapName);
		if (!util::pathIsBelowDirectory(filepath, fmt::format("{}/{}", data_dir, data_subdir_maps))) {
			m_connection.disconnect("Server tried to load a map outside of the map directory.");
			return;
		}

		auto buf = util::readFile(filepath);
		if (!buf) {
			const auto filepathInDownloads = fmt::format("{}/{}/{}/{}", data_dir, data_subdir_downloads, data_subdir_maps, m_serverMapName);
			if (!util::pathIsBelowDirectory(filepathInDownloads, fmt::format("{}/{}/{}", data_dir, data_subdir_downloads, data_subdir_maps))) {
				m_connection.disconnect("Server tried to load a map outside of the map directory.");
				return;
			}

			buf = util::readFile(filepathInDownloads);
			if (!buf) {
				m_connection.disconnect(fmt::format("Missing map \"{}\".", mapName));
				return;
			}
		}

		m_game.println(fmt::format("Loading map {}...", mapName));
		if (!m_game.map().load(m_serverMapName, std::move(*buf))) {
			m_connection.disconnect(fmt::format("Failed to load map \"{}\"!", mapName));
			return;
		}
	}

	// Write join request.
	m_game.println("Sending client info...");
	if (auto passwordKey = crypto::pw::Key{}; crypto::pw::deriveKey(passwordKey, m_serverPasswordSalt, password, m_serverPasswordHashType)) {
		const auto& inventory = this->getInventory(m_connection.getRemoteEndpoint());
		if (!this->writeToGameServer(msg::sv::out::JoinRequest{{},
															   m_game.map().getHash(),
															   game_version,
															   username,
															   static_cast<Tickrate>(cl_updaterate),
															   passwordKey,
															   inventory.id,
															   inventory.token})) {
			m_connection.close("Failed to write join request.");
		}
	} else {
		m_connection.disconnect("Failed to derive server password key.");
	}
}

auto GameClient::receivePackets() -> void {
	auto buffer = std::vector<std::byte>(net::MAX_PACKET_SIZE);
	while (true) {
		auto ec = std::error_code{};
		auto remoteEndpoint = net::IpEndpoint{};
		const auto receivedBytes = m_socket.receiveFrom(remoteEndpoint, buffer, ec).size();
		if (ec) {
			if (ec != net::SocketError::WAIT) {
				DEBUG_MSG(Msg::CLIENT, "Game client: Failed to receive packet: {}", ec.message());
			}
			break;
		}
		if (remoteEndpoint == m_connection.getRemoteEndpoint()) {
			buffer.resize(receivedBytes);
			m_connection.receivePacket(std::move(buffer));
			buffer.resize(net::MAX_PACKET_SIZE);
		} else {
			DEBUG_MSG(Msg::CLIENT, "Game client: Received packet from invalid sender \"{}\"!", std::string{remoteEndpoint});
		}
	}
}

auto GameClient::loadScreen(Screen& screen, std::string_view filename) -> void {
	const auto filepath = fmt::format("{}/{}/{}", data_dir, data_subdir_screens, filename);
	if (auto buf = util::readFile(filepath)) {
		screen = Screen{*buf};
	} else {
		m_game.error(fmt::format("Failed to load screen \"{}\"!", filename));
	}
}

auto GameClient::drawChar(Vec2 position, Color color, char ch) const -> void {
	if (const auto tilePosition = this->worldToGridCoordinates(position); m_viewport.contains(tilePosition)) {
		m_charWindow.draw(tilePosition, ch, color);
	}
}

auto GameClient::drawString(Vec2 position, Color color, std::string_view str) const -> void {
	if (auto tilePosition = this->worldToGridCoordinates(position);
		tilePosition.y >= m_viewport.y && tilePosition.y < m_viewport.y + m_viewport.h) {
		for (const auto ch : str) {
			if (tilePosition.x < m_viewport.x || tilePosition.x >= m_viewport.x + m_viewport.w) {
				break;
			}
			m_charWindow.draw(tilePosition, ch, color);
			++tilePosition.x;
		}
	}
}

auto GameClient::drawGun(Vec2 position, Color color, Direction direction, std::string_view gun) const -> void {
	if (gun.size() == 8) {
		const auto directionVector = direction.getVector();
		if (direction.isUp()) {
			if (direction.isLeft()) {
				this->drawChar(position + directionVector, color, gun[0]);
			} else if (direction.isRight()) {
				this->drawChar(position + directionVector, color, gun[2]);
			} else {
				this->drawChar(position + directionVector, color, gun[1]);
			}
		} else if (direction.isDown()) {
			if (direction.isLeft()) {
				this->drawChar(position + directionVector, color, gun[6]);
			} else if (direction.isRight()) {
				this->drawChar(position + directionVector, color, gun[4]);
			} else {
				this->drawChar(position + directionVector, color, gun[5]);
			}
		} else {
			if (direction.isLeft()) {
				this->drawChar(position + directionVector, color, gun[7]);
			} else if (direction.isRight()) {
				this->drawChar(position + directionVector, color, gun[3]);
			}
		}
	}
}

auto GameClient::drawCorpse(Vec2 position, Color color) const -> void {
	this->drawChar(position, color, cl_char_corpse);
}

auto GameClient::drawPlayer(Vec2 position, Color color, Direction aimDirection, PlayerClass playerClass, Hat hat) const -> void {
	this->drawChar(position, color, cl_char_player);
	this->drawChar(Vec2{position.x, position.y - 1}, hat.getColor(), hat.getChar());
	this->drawGun(position, color, aimDirection, playerClass.getGun());
}

auto GameClient::drawProjectile(Vec2 position, Color color, ProjectileType type) const -> void {
	this->drawChar(position, color, type.getChar());
}

auto GameClient::drawExplosion(Vec2 position, Color color) const -> void {
	if (cl_chars_explosion.size() == 9) {
		this->drawChar(position, color, cl_chars_explosion[4]);
		this->drawChar(Vec2{position.x - 1, position.y}, color, cl_chars_explosion[3]);
		this->drawChar(Vec2{position.x + 1, position.y}, color, cl_chars_explosion[5]);
		this->drawChar(Vec2{position.x - 1, position.y - 1}, color, cl_chars_explosion[0]);
		this->drawChar(Vec2{position.x, position.y - 1}, color, cl_chars_explosion[1]);
		this->drawChar(Vec2{position.x + 1, position.y - 1}, color, cl_chars_explosion[2]);
		this->drawChar(Vec2{position.x - 1, position.y + 1}, color, cl_chars_explosion[6]);
		this->drawChar(Vec2{position.x, position.y + 1}, color, cl_chars_explosion[7]);
		this->drawChar(Vec2{position.x + 1, position.y + 1}, color, cl_chars_explosion[8]);
	}
}

auto GameClient::drawSentryGun(Vec2 position, Color color, Direction aimDirection) const -> void {
	this->drawChar(position, color, cl_char_sentry);
	this->drawGun(position, color, aimDirection, cl_gun_sentry);
}

auto GameClient::drawGenericEntity(Vec2 position, Color color, const util::TileMatrix<char>& matrix) const -> void {
	const auto xEnd = static_cast<Vec2::Length>(position.x + matrix.getWidth());
	const auto yEnd = static_cast<Vec2::Length>(position.y + matrix.getHeight());

	auto localY = std::size_t{0};
	for (auto y = position.y; y != yEnd; ++y) {
		auto localX = std::size_t{0};
		for (auto x = position.x; x != xEnd; ++x) {
			this->drawChar(Vec2{x, y}, color, matrix.getUnchecked(localX, localY));
			++localX;
		}
		++localY;
	}
}

auto GameClient::drawMedkit(Vec2 position) const -> void {
	this->drawChar(position, cl_color_medkit, cl_char_medkit);
}

auto GameClient::drawAmmopack(Vec2 position) const -> void {
	this->drawChar(position, cl_color_ammopack, cl_char_ammopack);
}

auto GameClient::drawFlag(Vec2 position, Color color) const -> void {
	this->drawChar(position, color, cl_char_flag);
}

auto GameClient::drawCart(Vec2 position, Color color) const -> void {
	this->drawChar(position, color, cl_char_cart);
}
