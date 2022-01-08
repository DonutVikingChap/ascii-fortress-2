#ifndef AF2_SERVER_BOT_HPP
#define AF2_SERVER_BOT_HPP

#include "../../utilities/countdown.hpp" // util::Countdown
#include "../../utilities/reference.hpp" // util::Reference
#include "../data/actions.hpp"           // Actions, Action
#include "../data/direction.hpp"         // Direction
#include "../data/player_class.hpp"      // PlayerClass
#include "../data/player_id.hpp"         // PlayerId
#include "../data/team.hpp"              // Team
#include "../data/vector.hpp"            // Vec2
#include "../shared/snapshot.hpp"        // Snapshot

#include <cstddef>     // std::size_t
#include <optional>    // std::optional, std::nullopt
#include <random>      // std::mt19937, std::uniform_int_distribution, std::bernoulli_distribution, std::discrete_distribution
#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::pair, std::move
#include <vector>      // std::vector

class Map;
namespace ent {
namespace sh {
struct Player;
struct SentryGun;
} // namespace sh
} // namespace ent

class Bot final {
public:
	using CoordinateDistributionX = std::uniform_int_distribution<decltype(Vec2::x)>;
	using CoordinateDistributionY = std::uniform_int_distribution<decltype(Vec2::y)>;

	Bot(const Map& map, std::mt19937& rng, CoordinateDistributionX& xCoordinateDistribution,
	    CoordinateDistributionY& yCoordinateDistribution, PlayerId id, std::string name);

	static auto updateHealthProbability() -> void;
	static auto updateClassWeights() -> void;
	static auto updateGoalWeights() -> void;
	static auto updateSpyCheckProbability() -> void;

	auto think(float deltaTime) -> void;

	[[nodiscard]] auto getActions() const -> Actions;
	[[nodiscard]] auto getId() const -> PlayerId;
	[[nodiscard]] auto getRandomClass() const -> PlayerClass;
	[[nodiscard]] auto getName() const -> std::string_view;

	auto setSnapshot(Snapshot&& snapshot) -> void;
	auto setSnapshot(const Snapshot& snapshot) -> void;

private:
	struct FoundPlayer final {
		const ent::sh::Player& player;
		float distance;
	};

	auto onSpawn() -> void;
	auto onDeath() -> void;

	auto setRandomGoal() -> void;
	auto setGoalToGetObjective() -> void;
	auto setGoalToRoam() -> void;
	auto setGoalToDefend() -> void;
	auto setGoalToCaptureObjective() -> void;
	auto setGoalToGetHealth() -> void;

	[[nodiscard]] auto tryFight(float deltaTime) -> bool;

	auto fight(Vec2 target, float distance, bool aggressive, PlayerClass enemyClass, bool isSentry) -> void;

	[[nodiscard]] auto shouldReload() -> bool;
	[[nodiscard]] auto shouldGetHealth() -> bool;
	[[nodiscard]] auto shouldFlee() -> bool;

	auto onStopFighting() -> void;

	auto aimTowards(Vec2 position) -> void;
	auto aimAt(Vec2 position) -> void;

	auto attack1() -> void;
	auto attack2() -> void;

	[[nodiscard]] auto getMovementTowards(Vec2 position) const -> Actions;
	[[nodiscard]] auto getMovementAwayFrom(Vec2 position) const -> Actions;
	[[nodiscard]] auto getRandomMovement() const -> Actions;

	auto moveTowards(Vec2 position) -> void;
	auto moveRandomlyTowards(Vec2 position) -> void;
	auto moveAt(Vec2 position) -> void;
	auto moveRandomlyAt(Vec2 position) -> void;
	auto moveAwayFrom(Vec2 position) -> void;
	auto moveRandomlyAwayFrom(Vec2 position) -> void;
	auto moveRandomly() -> void;

	[[nodiscard]] auto getRangeSquared() const -> Vec2::Length;

	[[nodiscard]] auto isPotentialEnemy(const ent::sh::Player& player, bool requireLineOfSight) const -> bool;
	[[nodiscard]] auto isPotentiallyHealable(const ent::sh::Player& player) const -> bool;

	[[nodiscard]] auto findEnemyPlayer(bool requireLineOfSight) const -> std::optional<FoundPlayer>;
	[[nodiscard]] auto findHealablePlayer() const -> const ent::sh::Player*;
	[[nodiscard]] auto findSpy() const -> const ent::sh::Player*;
	[[nodiscard]] auto findEnemySentryGun() const -> const ent::sh::SentryGun*;

	[[nodiscard]] auto hasBuiltSentry() const -> bool;
	[[nodiscard]] auto isNearbySticky(Vec2 position) const -> bool;
	[[nodiscard]] auto findPath(Vec2 destination) -> bool;

	enum class Goal {
		GET_OBJECTIVE,
		CAPTURE_OBJECTIVE,
		ROAM,
		DEFEND,
		GET_HEALTH,
	};

	enum class State {
		DEAD,
		GOING,
		WAITING,
		FIGHTING,
	};

	enum class HealingState {
		NONE,
		HEALING,
		COOLDOWN,
	};

	enum class SpyCheckState {
		NONE,
		SUSPICIOUS,
		ALERT,
		COOLDOWN,
	};

	using DirectionDistribution = std::uniform_int_distribution<unsigned short>;
	using HealthDistribution = std::bernoulli_distribution;
	using ClassDistribution = std::discrete_distribution<unsigned short>;
	using GoalDistribution = std::discrete_distribution<unsigned short>;
	using SpyCheckDistribution = std::bernoulli_distribution;

	static auto directionDistribution() -> DirectionDistribution&;
	static auto healthDistribution() -> HealthDistribution&;
	static auto classDistribution() -> ClassDistribution&;
	static auto goalDistribution() -> GoalDistribution&;
	static auto spyCheckDistribution() -> SpyCheckDistribution&;

	util::Reference<const Map> m_map;
	util::Reference<std::mt19937> m_rng;
	util::Reference<CoordinateDistributionX> m_xCoordinateDistribution;
	util::Reference<CoordinateDistributionY> m_yCoordinateDistribution;
	PlayerId m_id;
	std::string m_name;
	Snapshot m_snapshot{};
	Actions m_actions = Action::NONE;
	Goal m_currentGoal = Goal::GET_OBJECTIVE;
	State m_currentState = State::DEAD;
	std::vector<Vec2> m_currentPath{};
	std::size_t m_currentNode = 0;
	util::Countdown<float> m_waitTimer{};
	HealingState m_healingState = HealingState::NONE;
	util::Countdown<float> m_healingTimer{};
	SpyCheckState m_spyCheckState = SpyCheckState::NONE;
	util::Countdown<float> m_spyCheckCountdown{};
	bool m_reloading = false;
};

#endif
