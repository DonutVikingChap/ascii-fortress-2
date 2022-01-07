#include "bot.hpp"

#include "../../console/commands/bot_commands.hpp" // bot_...
#include "../../utilities/algorithm.hpp"           // util::findClosestDistanceSquared, util::filter, util::anyOf
#include "../data/direction.hpp"                   // Direction
#include "../data/rectangle.hpp"                   // Rect
#include "../shared/entities.hpp"                  // ent::sh::Player, ent::sh::SentryGun, ent::findClosestDistanceSquared
#include "../shared/map.hpp"                       // Map

#include <cmath> // std::sqrt

Bot::Bot(const Map& map, std::mt19937& rng, CoordinateDistributionX& xCoordinateDistribution,
		 CoordinateDistributionY& yCoordinateDistribution, PlayerId id, std::string name)
	: m_map(map)
	, m_rng(rng)
	, m_xCoordinateDistribution(xCoordinateDistribution)
	, m_yCoordinateDistribution(yCoordinateDistribution)
	, m_id(id)
	, m_name(std::move(name)) {}

auto Bot::updateHealthProbability() -> void {
	Bot::healthDistribution() = HealthDistribution{static_cast<double>(bot_probability_get_health)};
}

auto Bot::updateClassWeights() -> void {
	Bot::classDistribution() = ClassDistribution{static_cast<double>(bot_class_weight_scout),
												 static_cast<double>(bot_class_weight_soldier),
												 static_cast<double>(bot_class_weight_pyro),
												 static_cast<double>(bot_class_weight_demoman),
												 static_cast<double>(bot_class_weight_heavy),
												 static_cast<double>(bot_class_weight_engineer),
												 static_cast<double>(bot_class_weight_medic),
												 static_cast<double>(bot_class_weight_sniper),
												 static_cast<double>(bot_class_weight_spy)};
}

auto Bot::updateGoalWeights() -> void {
	Bot::goalDistribution() = GoalDistribution{static_cast<double>(bot_decision_weight_do_objective),
											   static_cast<double>(bot_decision_weight_roam),
											   static_cast<double>(bot_decision_weight_defend)};
}

auto Bot::updateSpyCheckProbability() -> void {
	Bot::spyCheckDistribution() = SpyCheckDistribution{static_cast<double>(bot_probability_spycheck)};
}

auto Bot::think(float deltaTime) -> void {
	if (!m_snapshot.selfPlayer.alive) {
		if (m_currentState != State::DEAD) {
			m_currentState = State::DEAD;
			this->onDeath();
		}
		return;
	}

	while (true) {
		switch (m_currentState) {
			case State::DEAD:
				this->onSpawn();
				this->setGoalToRoam();
				break;
			case State::GOING:
				if (this->tryFight(deltaTime)) {
					return;
				} else {
					if (m_currentNode > 0) {
						if (m_currentGoal == Goal::GET_OBJECTIVE) {
							const auto area = Rect{static_cast<Rect::Length>(m_snapshot.selfPlayer.position.x - 1),
												   static_cast<Rect::Length>(m_snapshot.selfPlayer.position.y - 1),
												   3,
												   3};
							for (const auto& cart : m_snapshot.carts) {
								if (area.contains(cart.position)) {
									m_actions = Action::NONE;
									m_currentNode = 0;
									if (cart.team != m_snapshot.selfPlayer.team) {
										this->setRandomGoal();
									}
									return;
								}
							}
						}

						auto currentDestination = m_currentPath[m_currentNode - 1];
						if (m_snapshot.selfPlayer.position == currentDestination) {
							--m_currentNode;
							if (m_currentNode > 0) {
								currentDestination = m_currentPath[m_currentNode - 1];
							}
						}

						m_actions = Action::NONE;
						if (m_snapshot.selfPlayer.playerClass == PlayerClass::spy() &&
							m_snapshot.selfPlayer.skinTeam == m_snapshot.selfPlayer.team) {
							m_actions |= Action::ATTACK2;
						}
						this->moveTowards(currentDestination);
						this->aimTowards(currentDestination);
						return;
					}

					switch (m_currentGoal) {
						case Goal::GET_OBJECTIVE: this->setGoalToCaptureObjective(); break;
						case Goal::CAPTURE_OBJECTIVE: this->setRandomGoal(); break;
						case Goal::ROAM: this->setRandomGoal(); break;
						case Goal::DEFEND: this->setRandomGoal(); break;
						case Goal::GET_HEALTH:
							if (m_snapshot.selfPlayer.health >= m_snapshot.selfPlayer.playerClass.getHealth()) {
								this->setRandomGoal();
							} else {
								this->setGoalToGetHealth();
							}
							break;
					}
				}
				break;
			case State::WAITING:
				if (this->tryFight(deltaTime)) {
					m_currentState = State::FIGHTING;
					return;
				} else {
					if (m_waitTimer.advance(deltaTime).first) {
						this->setRandomGoal();
					} else {
						m_actions = Action::NONE;
						return;
					}
				}
				break;
			case State::FIGHTING:
				if (this->tryFight(deltaTime)) {
					return;
				}
				this->onStopFighting();
				break;
		}
	}
}

auto Bot::getActions() const -> Actions {
	return m_actions;
}

auto Bot::getId() const -> PlayerId {
	return m_id;
}

auto Bot::getRandomClass() const -> PlayerClass {
	switch (Bot::classDistribution()(*m_rng)) {
		case 0: return PlayerClass::scout();
		case 1: return PlayerClass::soldier();
		case 2: return PlayerClass::pyro();
		case 3: return PlayerClass::demoman();
		case 4: return PlayerClass::heavy();
		case 5: return PlayerClass::engineer();
		case 6: return PlayerClass::medic();
		case 7: return PlayerClass::sniper();
		case 8: return PlayerClass::spy();
	}
	return PlayerClass::none();
}

auto Bot::getName() const -> std::string_view {
	return m_name;
}

auto Bot::setSnapshot(Snapshot&& snapshot) -> void {
	m_snapshot = std::move(snapshot);
}

auto Bot::setSnapshot(const Snapshot& snapshot) -> void {
	m_snapshot = snapshot;
}

auto Bot::getRangeSquared() const -> Vec2::Length { // NOLINT(readability-convert-member-functions-to-static)
	const auto range = static_cast<int>(bot_range);
	return static_cast<Vec2::Length>(range * range);
}

auto Bot::onSpawn() -> void {
	m_spyCheckState = SpyCheckState::COOLDOWN;
	m_spyCheckCountdown.start(bot_spycheck_cooldown_spawn);
}

auto Bot::onDeath() -> void {
	m_actions = Action::NONE;
	m_currentPath.clear();
	m_currentNode = 0;
	m_waitTimer.reset();
	m_healingState = HealingState::NONE;
	m_spyCheckState = SpyCheckState::NONE;
	m_reloading = false;
}

auto Bot::setRandomGoal() -> void {
	// If an enemy flag is in range, prioritize that above all else.
	const auto isEnemyFlag = [&](const auto& flag) {
		return flag.team != m_snapshot.selfPlayer.team;
	};
	const auto otherTeamFlags = m_snapshot.flags | util::filter(isEnemyFlag);
	if (const auto closestFlag = ent::findClosestDistanceSquared(otherTeamFlags, m_snapshot.selfPlayer.position);
		closestFlag.first != otherTeamFlags.end()) {
		if (const auto range = static_cast<int>(bot_range); closestFlag.second < range * range && this->findPath(closestFlag.first->position)) {
			m_currentGoal = Goal::GET_OBJECTIVE;
			m_currentState = State::GOING;
			return;
		}
	}

	switch (Bot::goalDistribution()(*m_rng)) {
		case 0: this->setGoalToGetObjective(); break;
		case 1: this->setGoalToRoam(); break;
		case 2: this->setGoalToDefend(); break;
	}
}

auto Bot::setGoalToGetObjective() -> void {
	for (const auto& cart : m_snapshot.carts) {
		if (cart.team == m_snapshot.selfPlayer.team && this->findPath(cart.position)) {
			m_currentGoal = Goal::GET_OBJECTIVE;
			m_currentState = State::GOING;
			return;
		}
	}

	for (const auto& cart : m_snapshot.carts) {
		if (cart.team != m_snapshot.selfPlayer.team && this->findPath(cart.position)) {
			m_currentGoal = Goal::GET_OBJECTIVE;
			m_currentState = State::GOING;
			return;
		}
	}

	const auto isEnemyFlag = [&](const auto& flag) {
		return flag.team != m_snapshot.selfPlayer.team;
	};
	const auto otherTeamFlags = m_snapshot.flags | util::filter(isEnemyFlag);
	if (const auto closestFlag = ent::findClosestDistanceSquared(otherTeamFlags, m_snapshot.selfPlayer.position);
		closestFlag.first != otherTeamFlags.end() && this->findPath(closestFlag.first->position)) {
		m_currentGoal = Goal::GET_OBJECTIVE;
		m_currentState = State::GOING;
	} else {
		this->setGoalToRoam();
	}
}

auto Bot::setGoalToRoam() -> void {
	const auto isRed = m_snapshot.selfPlayer.team == Team::red();
	const auto isBlue = m_snapshot.selfPlayer.team == Team::blue();
	auto destination = Vec2{};
	do {
		do {
			destination.x = (*m_xCoordinateDistribution)(*m_rng);
			destination.y = (*m_yCoordinateDistribution)(*m_rng);
		} while (m_map->isSolid(destination, isRed, isBlue));
	} while (!this->findPath(destination));

	m_currentGoal = Goal::ROAM;
	m_currentState = State::GOING;
	m_healingState = HealingState::NONE;
}

auto Bot::setGoalToDefend() -> void {
	m_waitTimer.start(bot_defend_time);
	m_currentGoal = Goal::DEFEND;
	m_currentState = State::WAITING;
	m_healingState = HealingState::NONE;
}

auto Bot::setGoalToCaptureObjective() -> void {
	if (m_snapshot.selfPlayer.playerClass != PlayerClass::spy()) {
		for (const auto& cart : m_snapshot.carts) {
			if (cart.team == m_snapshot.selfPlayer.team) {
				this->setGoalToGetObjective();
				return;
			}
		}
	}

	const auto& flagSpawns = (m_snapshot.selfPlayer.team == Team::red())  ? m_map->getRedFlagSpawns() :
							 (m_snapshot.selfPlayer.team == Team::blue()) ? m_map->getBlueFlagSpawns() :
                                                                            decltype(m_map->getBlueFlagSpawns()){};

	if (const auto it = util::findClosestDistanceSquared(flagSpawns, m_snapshot.selfPlayer.position).first;
		it != flagSpawns.end() && this->findPath(*it)) {
		m_currentGoal = Goal::CAPTURE_OBJECTIVE;
		m_currentState = State::GOING;
	} else {
		this->setGoalToRoam();
	}
}

auto Bot::setGoalToGetHealth() -> void {
	if (const auto closestMedkit = ent::findClosestDistanceSquared(m_snapshot.medkits, m_snapshot.selfPlayer.position);
		closestMedkit.first != m_snapshot.medkits.end()) {
		if (this->findPath(closestMedkit.first->position)) {
			m_currentGoal = Goal::GET_HEALTH;
			m_currentState = State::GOING;
		} else {
			this->setGoalToRoam();
		}
	} else {
		this->setRandomGoal();
	}
}

auto Bot::tryFight(float deltaTime) -> bool {
	if (m_spyCheckState == SpyCheckState::COOLDOWN || m_spyCheckState == SpyCheckState::ALERT) {
		if (m_spyCheckCountdown.advance(deltaTime).first) {
			m_spyCheckState = SpyCheckState::NONE;
		}
	}

	if (m_healingState == HealingState::COOLDOWN) {
		if (m_healingTimer.advance(deltaTime).first) {
			m_healingState = HealingState::NONE;
		}
	}

	if (m_snapshot.selfPlayer.playerClass == PlayerClass::demoman()) {
		if (const auto enemy = this->findEnemyPlayer(false)) {
			if (this->isNearbySticky(enemy->player.position)) {
				m_actions = Action::NONE;
				this->aimTowards(enemy->player.position);
				this->moveAwayFrom(enemy->player.position);
				this->attack2();
				return true;
			}
		}
	}

	if (const auto* const sentryGun = this->findEnemySentryGun()) {
		this->fight(sentryGun->position, 0.0f, true, PlayerClass::none(), true);
		m_currentState = State::FIGHTING;
		return true;
	}

	if (m_snapshot.selfPlayer.playerClass == PlayerClass::spy() && m_snapshot.selfPlayer.skinTeam == m_snapshot.selfPlayer.team) {
		if (m_currentGoal == Goal::DEFEND && this->findEnemyPlayer(true)) {
			this->setGoalToRoam();
		}
		return false;
	}

	if (const auto* const spy = this->findSpy()) {
		if (spy->team == m_snapshot.selfPlayer.team) {
			if (m_spyCheckState == SpyCheckState::NONE) {
				m_spyCheckState = SpyCheckState::SUSPICIOUS;
				m_spyCheckCountdown.start(bot_spycheck_reaction_time);
			}

			if (m_spyCheckState == SpyCheckState::SUSPICIOUS) {
				if (m_spyCheckCountdown.advance(deltaTime).first) {
					if (Bot::spyCheckDistribution()(*m_rng)) {
						m_spyCheckState = SpyCheckState::ALERT;
						m_spyCheckCountdown.start(bot_spycheck_time);
					} else {
						m_spyCheckState = SpyCheckState::COOLDOWN;
						m_spyCheckCountdown.start(bot_spycheck_cooldown);
					}
				}
			}
		} else {
			m_spyCheckState = SpyCheckState::ALERT;
			m_spyCheckCountdown.start(bot_spycheck_panic_time);
		}
	}

	const auto enemy = this->findEnemyPlayer(true);
	if (m_snapshot.selfPlayer.playerClass == PlayerClass::medic()) {
		if (const auto* const teammate = this->findHealablePlayer()) {
			if (m_healingState == HealingState::NONE) {
				m_healingState = HealingState::HEALING;
				m_healingTimer.start(bot_heal_time);
			}

			if (m_healingState == HealingState::HEALING) {
				if (m_healingTimer.advance(deltaTime).first) {
					m_healingState = HealingState::COOLDOWN;
					m_healingTimer.start(bot_heal_cooldown);
				} else if (teammate->playerClass != PlayerClass::medic() || !enemy) {
					m_actions = Action::NONE;
					this->aimAt(teammate->position);
					this->attack1();
					if (enemy) {
						this->moveRandomlyAwayFrom(enemy->player.position);
					}
					return true;
				}
			}
		}
	}

	if (enemy) {
		this->fight(enemy->player.position,
					enemy->distance,
					m_currentGoal != Goal::DEFEND && enemy->player.team != m_snapshot.selfPlayer.team,
					enemy->player.playerClass,
					false);
		m_currentState = State::FIGHTING;
		return true;
	}

	return false;
}

auto Bot::fight(Vec2 target, float distance, bool aggressive, PlayerClass enemyClass, bool isSentry) -> void {
	m_actions = Action::NONE;
	this->aimAt(target);
	switch (m_snapshot.selfPlayer.playerClass) {
		case PlayerClass::scout():
			if (this->shouldReload()) {
				this->moveRandomlyAwayFrom(target);
			} else {
				this->attack1();
				if (isSentry) {
					this->moveRandomlyAt(target);
				} else {
					if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::scout()) {
						this->moveAwayFrom(target);
					}
					this->moveRandomly();
				}
			}
			break;
		case PlayerClass::soldier():
			if (isSentry) {
				this->attack1();
				this->moveAt(target);
			} else {
				if (distance > bot_range_shotgun) {
					if (this->shouldReload()) {
						this->moveRandomly();
					} else {
						this->attack1();
						if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
							this->moveAwayFrom(target);
						} else {
							this->moveRandomly();
						}
					}
				} else {
					this->attack2();
					this->moveRandomlyAwayFrom(target);
				}
			}
			break;
		case PlayerClass::pyro():
			if (this->shouldReload()) {
				if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
					this->moveRandomlyAwayFrom(target);
				} else {
					this->moveRandomlyAt(target);
				}
			} else {
				this->attack1();
				if (isSentry) {
					this->moveRandomlyAt(target);
				} else if (enemyClass == PlayerClass::spy()) {
					this->moveRandomly();
				} else if (aggressive) {
					this->moveRandomlyAt(target);
				} else {
					this->moveRandomly();
				}
			}
			break;
		case PlayerClass::demoman():
			if (this->shouldReload()) {
				this->moveRandomlyAwayFrom(target);
			} else {
				this->attack1();
				if (isSentry) {
					this->moveRandomly();
				} else if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
					this->moveAwayFrom(target);
				} else if (aggressive) {
					this->moveRandomlyAwayFrom(target);
				}
			}
			break;
		case PlayerClass::heavy():
			if (this->shouldReload()) {
				this->moveRandomlyAwayFrom(target);
			} else {
				this->attack1();
				if (isSentry) {
					this->moveAt(target);
				} else if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
					this->moveRandomlyAwayFrom(target);
				} else if (aggressive) {
					if (enemyClass == PlayerClass::scout() || enemyClass == PlayerClass::soldier()) {
						this->moveRandomly();
					} else {
						this->moveRandomlyTowards(target);
					}
				}
			}
			break;
		case PlayerClass::engineer():
			if (this->shouldReload()) {
				this->moveAwayFrom(target);
			} else {
				if (isSentry) {
					this->attack1();
					this->moveRandomlyAt(target);
				} else if (enemyClass == PlayerClass::medic() || enemyClass == PlayerClass::engineer() || this->hasBuiltSentry()) {
					this->attack1();
					if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
						this->moveAwayFrom(target);
					} else {
						this->moveRandomly();
					}
				} else {
					this->attack2();
					this->moveRandomlyAwayFrom(target);
				}
			}
			break;
		case PlayerClass::medic():
			this->attack2();
			if (isSentry) {
				this->moveRandomly();
			} else {
				this->moveRandomlyAwayFrom(target);
			}
			break;
		case PlayerClass::sniper():
			this->attack1();
			if (isSentry) {
				this->moveAt(target);
			} else if (enemyClass == PlayerClass::spy() || enemyClass == PlayerClass::pyro()) {
				this->moveAwayFrom(target);
			} else {
				this->moveRandomly();
			}
			break;
		case PlayerClass::spy():
			if (aggressive || isSentry) {
				this->moveAt(target);
			} else {
				this->moveRandomlyAt(target);
			}
			break;
		default: break;
	}
}

auto Bot::shouldReload() -> bool {
	if (m_reloading) {
		if (m_snapshot.selfPlayer.primaryAmmo < m_snapshot.selfPlayer.playerClass.getPrimaryWeapon().getAmmoPerClip() / 2) {
			return true;
		}
		m_reloading = false;
	}

	if (!m_reloading && m_snapshot.selfPlayer.primaryAmmo == 0) {
		m_reloading = true;
		return true;
	}
	return false;
}

auto Bot::shouldGetHealth() -> bool {
	return m_currentGoal != Goal::CAPTURE_OBJECTIVE && m_snapshot.selfPlayer.health < m_snapshot.selfPlayer.playerClass.getHealth() &&
		   Bot::healthDistribution()(*m_rng);
}

auto Bot::shouldFlee() -> bool {
	return m_currentGoal != Goal::CAPTURE_OBJECTIVE &&
		   (m_snapshot.selfPlayer.playerClass == PlayerClass::medic() || m_snapshot.selfPlayer.playerClass == PlayerClass::spy() ||
			m_snapshot.selfPlayer.playerClass == PlayerClass::demoman() || this->shouldReload());
}

auto Bot::onStopFighting() -> void {
	if (this->shouldGetHealth()) {
		this->setGoalToGetHealth();
	} else if (this->shouldFlee()) {
		this->setRandomGoal();
	} else {
		m_currentState = State::GOING;
		if (!m_currentPath.empty()) {
			if (!this->findPath(m_currentPath.front())) {
				this->setRandomGoal();
			}
		}
	}
}

auto Bot::aimTowards(Vec2 position) -> void {
	if (position.y < m_snapshot.selfPlayer.position.y) {
		m_actions |= Action::AIM_UP;
	} else if (position.y > m_snapshot.selfPlayer.position.y) {
		m_actions |= Action::AIM_DOWN;
	}

	if (position.x < m_snapshot.selfPlayer.position.x) {
		m_actions |= Action::AIM_LEFT;
	} else if (position.x > m_snapshot.selfPlayer.position.x) {
		m_actions |= Action::AIM_RIGHT;
	}
}

auto Bot::aimAt(Vec2 position) -> void {
	const auto aimVector = position - m_snapshot.selfPlayer.position;
	const auto direction = Direction{aimVector};
	if (direction.hasLeft()) {
		m_actions |= Action::AIM_LEFT;
	}
	if (direction.hasRight()) {
		m_actions |= Action::AIM_RIGHT;
	}
	if (direction.hasUp()) {
		m_actions |= Action::AIM_UP;
	}
	if (direction.hasDown()) {
		m_actions |= Action::AIM_DOWN;
	}
}

auto Bot::attack1() -> void {
	m_actions |= Action::ATTACK1;
}

auto Bot::attack2() -> void {
	m_actions |= Action::ATTACK2;
}

auto Bot::getMovementTowards(Vec2 position) const -> Actions {
	auto actions = Actions{Action::NONE};
	if (position.y < m_snapshot.selfPlayer.position.y) {
		actions |= Action::MOVE_UP;
	} else if (position.y > m_snapshot.selfPlayer.position.y) {
		actions |= Action::MOVE_DOWN;
	}

	if (position.x < m_snapshot.selfPlayer.position.x) {
		actions |= Action::MOVE_LEFT;
	} else if (position.x > m_snapshot.selfPlayer.position.x) {
		actions |= Action::MOVE_RIGHT;
	}
	return actions;
}

auto Bot::getMovementAwayFrom(Vec2 position) const -> Actions {
	auto actions = Actions{Action::NONE};
	if (position.y < m_snapshot.selfPlayer.position.y) {
		actions |= Action::MOVE_DOWN;
	} else if (position.y > m_snapshot.selfPlayer.position.y) {
		actions |= Action::MOVE_UP;
	}

	if (position.x < m_snapshot.selfPlayer.position.x) {
		actions |= Action::MOVE_RIGHT;
	} else if (position.x > m_snapshot.selfPlayer.position.x) {
		actions |= Action::MOVE_LEFT;
	}
	return actions;
}

auto Bot::getRandomMovement() const -> Actions {
	switch (Bot::directionDistribution()(*m_rng)) {
		case 1: return Action::MOVE_UP;
		case 2: return Action::MOVE_DOWN;
		case 3: return Action::MOVE_LEFT;
		case 4: return Action::MOVE_RIGHT;
		case 5: return Action::MOVE_UP | Action::MOVE_LEFT;
		case 6: return Action::MOVE_UP | Action::MOVE_RIGHT;
		case 7: return Action::MOVE_DOWN | Action::MOVE_LEFT;
		case 8: return Action::MOVE_DOWN | Action::MOVE_RIGHT;
	}
	return Action::NONE;
}

auto Bot::moveTowards(Vec2 position) -> void {
	m_actions |= this->getMovementTowards(position);
}

auto Bot::moveRandomlyTowards(Vec2 position) -> void {
	if (position != m_snapshot.selfPlayer.position) {
		m_actions |= this->getMovementTowards(position) | this->getRandomMovement();
	} else {
		this->moveRandomly();
	}
}

auto Bot::moveAt(Vec2 position) -> void {
	if (position != m_snapshot.selfPlayer.position) {
		this->moveTowards(position);
	} else {
		this->moveRandomly();
	}
}

auto Bot::moveRandomlyAt(Vec2 position) -> void {
	if (position != m_snapshot.selfPlayer.position) {
		m_actions |= this->getMovementTowards(position) | (this->getRandomMovement() & ~this->getMovementAwayFrom(position));
	} else {
		this->moveRandomly();
	}
}

auto Bot::moveAwayFrom(Vec2 position) -> void {
	m_actions |= this->getMovementAwayFrom(position);
}

auto Bot::moveRandomly() -> void {
	m_actions |= this->getRandomMovement();
}

auto Bot::moveRandomlyAwayFrom(Vec2 position) -> void {
	this->moveAwayFrom(position);
	this->moveRandomly();
}

auto Bot::isPotentialEnemy(const ent::sh::Player& player, bool requireLineOfSight) const -> bool {
	if (player.team != m_snapshot.selfPlayer.team ||
		(player.playerClass == PlayerClass::spy() && m_snapshot.selfPlayer.playerClass != PlayerClass::spy() && m_spyCheckState == SpyCheckState::ALERT)) {
		return !requireLineOfSight || m_map->lineOfSight(m_snapshot.selfPlayer.position, player.position);
	}
	return false;
}

auto Bot::isPotentiallyHealable(const ent::sh::Player& player) const -> bool {
	return player.team == m_snapshot.selfPlayer.team && (player.playerClass != PlayerClass::spy() || m_spyCheckState != SpyCheckState::ALERT) &&
		   m_map->lineOfSight(m_snapshot.selfPlayer.position, player.position);
}

auto Bot::findEnemyPlayer(bool requireLineOfSight) const -> std::optional<Bot::FoundPlayer> {
	const auto isPotentialEnemy = [&](const auto& player) {
		return this->isPotentialEnemy(player, requireLineOfSight);
	};
	const auto potentialEnemies = m_snapshot.players | util::filter(isPotentialEnemy);
	if (const auto closestEnemy = ent::findClosestDistanceSquared(potentialEnemies, m_snapshot.selfPlayer.position);
		closestEnemy.first != potentialEnemies.end()) {
		if (const auto range = static_cast<int>(bot_range); closestEnemy.second <= range * range) {
			return FoundPlayer{*closestEnemy.first, std::sqrt(static_cast<float>(closestEnemy.second)) / static_cast<float>(range)};
		}
	}
	return std::nullopt;
}

auto Bot::findHealablePlayer() const -> const ent::sh::Player* {
	const auto isPotentiallyHealable = [&](const auto& player) {
		return this->isPotentiallyHealable(player);
	};
	const auto healableTeammates = util::filter(m_snapshot.players, isPotentiallyHealable);
	if (const auto closestTeammate = ent::findClosestDistanceSquared(healableTeammates, m_snapshot.selfPlayer.position);
		closestTeammate.first != healableTeammates.end()) {
		const auto speed = 1.0f / ProjectileType::heal_beam().getMoveInterval();
		const auto time = ProjectileType::heal_beam().getDisappearTime();
		if (const auto range = static_cast<int>(speed * time); closestTeammate.second <= range * range) {
			return &*closestTeammate.first;
		}
	}
	return nullptr;
}

auto Bot::findSpy() const -> const ent::sh::Player* {
	const auto visibleSpies =
		m_snapshot.players | util::filter([&](const auto& player) {
			if (player.playerClass != PlayerClass::spy()) {
				return false;
			}

			if (const auto inFrontX = (player.position.x <= m_snapshot.selfPlayer.position.x && m_snapshot.selfPlayer.aimDirection.hasLeft()) ||
									  (player.position.x >= m_snapshot.selfPlayer.position.x && m_snapshot.selfPlayer.aimDirection.hasRight());
				!inFrontX) {
				return false;
			}
			if (const auto inFrontY = (player.position.y <= m_snapshot.selfPlayer.position.y && m_snapshot.selfPlayer.aimDirection.hasUp()) ||
									  (player.position.y >= m_snapshot.selfPlayer.position.y && m_snapshot.selfPlayer.aimDirection.hasDown());
				!inFrontY) {
				return false;
			}
			return m_map->lineOfSight(m_snapshot.selfPlayer.position, player.position);
		});

	if (const auto closestSpy = ent::findClosestDistanceSquared(visibleSpies, m_snapshot.selfPlayer.position);
		closestSpy.first != visibleSpies.end() && closestSpy.second <= this->getRangeSquared()) {
		return &*closestSpy.first;
	}
	return nullptr;
}

auto Bot::findEnemySentryGun() const -> const ent::sh::SentryGun* {
	const auto visibleSentryGuns = m_snapshot.sentryGuns | util::filter([&](const auto& sentryGun) {
									   return sentryGun.team != m_snapshot.selfPlayer.team &&
											  m_map->lineOfSight(m_snapshot.selfPlayer.position, sentryGun.position);
								   });

	if (const auto& closestSentry = ent::findClosestDistanceSquared(visibleSentryGuns, m_snapshot.selfPlayer.position);
		closestSentry.first != visibleSentryGuns.end() && closestSentry.second <= this->getRangeSquared()) {
		return &*closestSentry.first;
	}
	return nullptr;
}

auto Bot::hasBuiltSentry() const -> bool {
	return util::anyOf(m_snapshot.sentryGuns, [&](const auto& sentryGun) { return sentryGun.owner == m_id; });
}

auto Bot::isNearbySticky(Vec2 position) const -> bool {
	const auto area = Rect{static_cast<Rect::Length>(position.x - 2), static_cast<Rect::Length>(position.y - 2), 5, 5};
	return util::anyOf(m_snapshot.projectiles, [&](const auto& projectile) {
		return projectile.owner == m_id && projectile.type == ProjectileType::sticky() && area.contains(projectile.position);
	});
}

auto Bot::findPath(Vec2 destination) -> bool {
	m_currentPath = m_map->findPath(m_snapshot.selfPlayer.position,
									destination,
									m_snapshot.selfPlayer.team == Team::red(),
									m_snapshot.selfPlayer.team == Team::blue());
	m_currentNode = m_currentPath.size();
	return !m_currentPath.empty();
}

auto Bot::directionDistribution() -> Bot::DirectionDistribution& {
	static auto directionDistribution = DirectionDistribution{0, 8};
	return directionDistribution;
}

auto Bot::healthDistribution() -> Bot::HealthDistribution& {
	static auto healthDistribution = HealthDistribution{};
	return healthDistribution;
}

auto Bot::classDistribution() -> Bot::ClassDistribution& {
	static auto classDistribution = ClassDistribution{};
	return classDistribution;
}

auto Bot::goalDistribution() -> Bot::GoalDistribution& {
	static auto goalDistribution = GoalDistribution{};
	return goalDistribution;
}

auto Bot::spyCheckDistribution() -> Bot::SpyCheckDistribution& {
	static auto spyCheckDistribution = SpyCheckDistribution{};
	return spyCheckDistribution;
}
