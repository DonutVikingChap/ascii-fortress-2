#include "map.hpp"

#include "../../utilities/algorithm.hpp" // util::contains

#include <algorithm>     // std::reverse
#include <cmath>         // std::abs
#include <cstddef>       // std::size_t
#include <cstdint>       // std::uint32_t
#include <optional>      // std::optional, std::nullopt
#include <queue>         // std::priority_queue
#include <unordered_map> // std::unordered_map
#include <unordered_set> // std::unordered_set

namespace {

auto parseSubstr(std::string_view str, std::string_view beginTag, std::string_view endTag) -> std::string_view {
	const auto i = str.find(beginTag);
	if (i == std::string_view::npos) {
		return std::string_view{};
	}
	const auto iBegin = i + beginTag.size();
	const auto iEnd = str.find(endTag, i);
	if (iBegin < iEnd) {
		return str.substr(iBegin, iEnd - iBegin);
	}
	return std::string_view{};
}

auto parseChar(std::string_view str, std::string_view tag) -> char {
	if (const auto i = str.find(tag); i != std::string_view::npos && i + tag.size() < str.size()) {
		return str[i + tag.size()];
	}
	return '\0';
}

auto getUnvisitedNeighbor(const std::unordered_set<Vec2>& nodes, Vec2 position, const std::unordered_set<Vec2>& visited) -> std::optional<Vec2> {
	if (const auto up = Vec2{position.x, position.y - 1}; nodes.count(up) != 0 && visited.count(up) == 0) {
		return up;
	}
	if (const auto down = Vec2{position.x, position.y + 1}; nodes.count(down) != 0 && visited.count(down) == 0) {
		return down;
	}
	if (const auto left = Vec2{position.x - 1, position.y}; nodes.count(left) != 0 && visited.count(left) == 0) {
		return left;
	}
	if (const auto right = Vec2{position.x + 1, position.y}; nodes.count(right) != 0 && visited.count(right) == 0) {
		return right;
	}

	if (const auto upLeft = Vec2{position.x - 1, position.y - 1}; nodes.count(upLeft) != 0 && visited.count(upLeft) == 0) {
		return upLeft;
	}
	if (const auto upRight = Vec2{position.x + 1, position.y - 1}; nodes.count(upRight) != 0 && visited.count(upRight) == 0) {
		return upRight;
	}
	if (const auto downLeft = Vec2{position.x - 1, position.y + 1}; nodes.count(downLeft) != 0 && visited.count(downLeft) == 0) {
		return downLeft;
	}
	if (const auto downRight = Vec2{position.x + 1, position.y + 1}; nodes.count(downRight) != 0 && visited.count(downRight) == 0) {
		return downRight;
	}

	return std::nullopt;
}

auto makePath(const std::unordered_set<Vec2>& nodes, Vec2 start) -> std::vector<Vec2> {
	auto path = std::vector<Vec2>{start};
	auto visited = std::unordered_set<Vec2>{};

	auto previousPosition = start;
	auto node = getUnvisitedNeighbor(nodes, start, visited);
	while (node) {
		const auto position = *node;

		const auto extrapolated = position + (position - previousPosition);

		path.push_back(position);
		visited.insert(position);

		if (nodes.count(extrapolated) != 0 && visited.count(extrapolated) == 0) {
			node = extrapolated;
		} else {
			node = getUnvisitedNeighbor(nodes, position, visited);
		}

		previousPosition = position;
	}
	return path;
}

template <typename Func>
auto forEachNonSolidNeighbor(const Map& map, Vec2 p, bool red, bool blue, Func&& callback) -> void {
	constexpr auto COST_STRAIGHT = std::uint32_t{1000};
	constexpr auto COST_DIAGONAL = std::uint32_t{1414};

	if (const auto up = Vec2{p.x, p.y - 1}; !map.isSolid(up, red, blue, Direction::up())) {
		callback(up, COST_STRAIGHT);
	}
	if (const auto down = Vec2{p.x, p.y + 1}; !map.isSolid(down, red, blue, Direction::down())) {
		callback(down, COST_STRAIGHT);
	}
	if (const auto left = Vec2{p.x - 1, p.y}; !map.isSolid(left, red, blue, Direction::left())) {
		callback(left, COST_STRAIGHT);
	}
	if (const auto right = Vec2{p.x + 1, p.y}; !map.isSolid(right, red, blue, Direction::right())) {
		callback(right, COST_STRAIGHT);
	}

	if (const auto upLeft = Vec2{p.x - 1, p.y - 1}; !map.isSolid(upLeft, red, blue, Direction::up() | Direction::left())) {
		callback(upLeft, COST_DIAGONAL);
	}
	if (const auto upRight = Vec2{p.x + 1, p.y - 1}; !map.isSolid(upRight, red, blue, Direction::up() | Direction::right())) {
		callback(upRight, COST_DIAGONAL);
	}
	if (const auto downLeft = Vec2{p.x - 1, p.y + 1}; !map.isSolid(downLeft, red, blue, Direction::down() | Direction::left())) {
		callback(downLeft, COST_DIAGONAL);
	}
	if (const auto downRight = Vec2{p.x + 1, p.y + 1}; !map.isSolid(downRight, red, blue, Direction::down() | Direction::right())) {
		callback(downRight, COST_DIAGONAL);
	}
}

} // namespace

auto Map::unLoad() -> void {
	m_matrix.clear();
	m_name.clear();
	m_redCartSpawn = {};
	m_blueCartSpawn = {};
	m_redCartPath.clear();
	m_blueCartPath.clear();
	m_redFlagSpawns.clear();
	m_blueFlagSpawns.clear();
	m_redSpawns.clear();
	m_blueSpawns.clear();
	m_redRespawnRoomVisualizers.clear();
	m_blueRespawnRoomVisualizers.clear();
	m_resupplyLockers.clear();
	m_medkitSpawns.clear();
	m_ammopackSpawns.clear();
	m_resources.clear();
	m_script.clear();
}

auto Map::load(std::string name, std::string_view str) -> bool {
	this->unLoad();

	const auto data = [&] {
		if (const auto dataStr = parseSubstr(str, "[DATA]\n", "\n[END_DATA]"); !dataStr.empty()) {
			return dataStr;
		}
		return str; // Interpret the entire string as data if the [DATA] tag was not found.
	}();

	m_hash = util::CRC32{util::asBytes(util::Span{data})};

	m_matrix = util::TileMatrix<char>{data, AIR_CHAR};
	if (m_matrix.empty()) {
		return false;
	}

	m_name = std::move(name);

	const auto resScript = Script::parse(parseSubstr(str, "[RESOURCES]\n", "[END_RESOURCES]"));
	m_resources.reserve(resScript.size());
	for (const auto& cmd : resScript) {
		m_resources.push_back(Script::commandString(cmd));
	}

	m_script = Script::parse(parseSubstr(str, "[SCRIPT]\n", "\n[END_SCRIPT]"));

	const auto redSpawnChar = parseChar(str, "[SPAWN_RED] ");
	const auto blueSpawnChar = parseChar(str, "[SPAWN_BLU] ");
	const auto medkitChar = parseChar(str, "[MEDKIT] ");
	const auto ammopackChar = parseChar(str, "[AMMOPACK] ");
	const auto redFlagChar = parseChar(str, "[FLAG_RED] ");
	const auto blueFlagChar = parseChar(str, "[FLAG_BLU] ");
	const auto redSpawnVisChar = parseChar(str, "[SPAWNVIS_RED] ");
	const auto blueSpawnVisChar = parseChar(str, "[SPAWNVIS_BLU] ");
	const auto resupplyChar = parseChar(str, "[RESUPPLY] ");
	const auto redTrackChar = parseChar(str, "[TRACK_RED] ");
	const auto blueTrackChar = parseChar(str, "[TRACK_BLU] ");
	const auto redCartChar = parseChar(str, "[CART_RED] ");
	const auto blueCartChar = parseChar(str, "[CART_BLU] ");

	auto redTrack = std::unordered_set<Vec2>{};
	auto blueTrack = std::unordered_set<Vec2>{};
	for (auto y = std::size_t{0}; y < m_matrix.getHeight(); ++y) {
		for (auto x = std::size_t{0}; x < m_matrix.getWidth(); ++x) {
			const auto position = Vec2{static_cast<Vec2::Length>(x), static_cast<Vec2::Length>(y)};
			if (const auto ch = m_matrix.get(x, y); ch != '\0') {
				if (ch == redTrackChar) {
					redTrack.insert(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == blueTrackChar) {
					blueTrack.insert(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == redCartChar) {
					m_redCartSpawn = position;
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == blueCartChar) {
					m_blueCartSpawn = position;
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == redSpawnChar) {
					m_redSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == blueSpawnChar) {
					m_blueSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == medkitChar) {
					m_medkitSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == ammopackChar) {
					m_ammopackSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == redFlagChar) {
					m_redFlagSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == blueFlagChar) {
					m_blueFlagSpawns.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == redSpawnVisChar) {
					m_redRespawnRoomVisualizers.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == blueSpawnVisChar) {
					m_blueRespawnRoomVisualizers.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				} else if (ch == resupplyChar) {
					m_resupplyLockers.push_back(position);
					m_matrix.set(x, y, AIR_CHAR);
				}
			}
		}
	}

	if (!redTrack.empty()) {
		m_redCartPath = makePath(redTrack, m_redCartSpawn);
	}

	if (!blueTrack.empty()) {
		m_blueCartPath = makePath(blueTrack, m_blueCartSpawn);
	}

	return true;
}

auto Map::isLoaded() const noexcept -> bool {
	return !m_matrix.empty();
}

auto Map::getWidth() const noexcept -> Vec2::Length {
	return static_cast<Vec2::Length>(m_matrix.getWidth());
}

auto Map::getHeight() const noexcept -> Vec2::Length {
	return static_cast<Vec2::Length>(m_matrix.getHeight());
}

auto Map::getHash() const noexcept -> util::CRC32 {
	return m_hash;
}

auto Map::getName() const noexcept -> std::string_view {
	return m_name;
}

auto Map::getMatrix() const noexcept -> const util::TileMatrix<char>& {
	return m_matrix;
}

auto Map::getResources() const noexcept -> util::Span<const std::string> {
	return m_resources;
}

auto Map::getScript() const noexcept -> const Script& {
	return m_script;
}

auto Map::getRedCartSpawn() const noexcept -> Vec2 {
	return m_redCartSpawn;
}

auto Map::getBlueCartSpawn() const noexcept -> Vec2 {
	return m_blueCartSpawn;
}

auto Map::getRedCartPath() const noexcept -> util::Span<const Vec2> {
	return m_redCartPath;
}

auto Map::getBlueCartPath() const noexcept -> util::Span<const Vec2> {
	return m_blueCartPath;
}

auto Map::getRedFlagSpawns() const noexcept -> util::Span<const Vec2> {
	return m_redFlagSpawns;
}

auto Map::getBlueFlagSpawns() const noexcept -> util::Span<const Vec2> {
	return m_blueFlagSpawns;
}

auto Map::getRedSpawns() const noexcept -> util::Span<const Vec2> {
	return m_redSpawns;
}

auto Map::getBlueSpawns() const noexcept -> util::Span<const Vec2> {
	return m_blueSpawns;
}

auto Map::getRedRespawnRoomVisualizers() const noexcept -> util::Span<const Vec2> {
	return m_redRespawnRoomVisualizers;
}

auto Map::getBlueRespawnRoomVisualizers() const noexcept -> util::Span<const Vec2> {
	return m_blueRespawnRoomVisualizers;
}

auto Map::getResupplyLockers() const noexcept -> util::Span<const Vec2> {
	return m_resupplyLockers;
}

auto Map::getMedkitSpawns() const noexcept -> util::Span<const Vec2> {
	return m_medkitSpawns;
}

auto Map::getAmmopackSpawns() const noexcept -> util::Span<const Vec2> {
	return m_ammopackSpawns;
}

auto Map::get(Vec2 p, char defaultVal) const noexcept -> char {
	return m_matrix.get(static_cast<std::size_t>(p.x), static_cast<std::size_t>(p.y), defaultVal);
}

auto Map::isResupplyLocker(Vec2 p) const noexcept -> bool {
	return util::contains(m_resupplyLockers, p);
}

auto Map::isRedRespawnRoomVisualizer(Vec2 p) const noexcept -> bool {
	return util::contains(m_redRespawnRoomVisualizers, p);
}

auto Map::isBlueRespawnRoomVisualizer(Vec2 p) const noexcept -> bool {
	return util::contains(m_blueRespawnRoomVisualizers, p);
}

auto Map::isSolid(Vec2 p, bool red, bool blue) const noexcept -> bool {
	switch (this->get(p)) {
		case AIR_CHAR:
			if (!red && this->isRedRespawnRoomVisualizer(p)) {
				return true;
			}
			if (!blue && this->isBlueRespawnRoomVisualizer(p)) {
				return true;
			}
			break;
		case ONEWAY_LEFT_CHAR: break;
		case ONEWAY_RIGHT_CHAR: break;
		case ONEWAY_UP_CHAR: break;
		case ONEWAY_DOWN_CHAR: break;
		default: return true;
	}
	return false;
}

auto Map::isSolid(Vec2 p, bool red, bool blue, Direction moveDirection) const noexcept -> bool {
	switch (this->get(p)) {
		case AIR_CHAR:
			if (!red && this->isRedRespawnRoomVisualizer(p)) {
				return true;
			}
			if (!blue && this->isBlueRespawnRoomVisualizer(p)) {
				return true;
			}
			break;
		case ONEWAY_LEFT_CHAR:
			if (!moveDirection.hasLeft()) {
				return true;
			}
			break;
		case ONEWAY_RIGHT_CHAR:
			if (!moveDirection.hasRight()) {
				return true;
			}
			break;
		case ONEWAY_UP_CHAR:
			if (!moveDirection.hasUp()) {
				return true;
			}
			break;
		case ONEWAY_DOWN_CHAR:
			if (!moveDirection.hasDown()) {
				return true;
			}
			break;
		default: return true;
	}
	return false;
}

auto Map::lineOfSight(Vec2 p1, Vec2 p2) const noexcept -> bool {
	const auto dx = std::abs(p2.x - p1.x);
	const auto dy = std::abs(p2.y - p1.y);
	const auto sx = (p1.x < p2.x) ? Vec2::Length{1} : Vec2::Length{-1};
	const auto sy = (p1.y < p2.y) ? Vec2::Length{1} : Vec2::Length{-1};

	auto err = ((dx > dy) ? dx : -dy) / 2;
	while (true) {
		if (Map::isSolidChar(this->get(p1))) {
			return false;
		}
		if (p1 == p2) {
			break;
		}
		const auto error = err;
		if (error > -dx) {
			err -= dy;
			p1.x = static_cast<Vec2::Length>(p1.x + sx);
		}
		if (error < dy) {
			err += dx;
			p1.y = static_cast<Vec2::Length>(p1.y + sy);
		}
	}
	return true;
}

auto Map::findPath(Vec2 start, Vec2 destination, bool red, bool blue) const -> std::vector<Vec2> {
	// Use A* pathfinding algorithm to find a path to the destination.
	struct Node final {
		constexpr Node(std::uint32_t cost, Vec2 position) noexcept
			: cost(cost)
			, position(position) {}

		[[nodiscard]] constexpr auto operator<(const Node& other) const noexcept -> bool {
			return cost > other.cost;
		}

		std::uint32_t cost;
		Vec2 position;
	};

	// Heuristic function for A*. Uses Manhattan distance.
	const auto heuristic = [destination](Vec2 p) {
		return std::abs(p.x - destination.x) + std::abs(p.y - destination.y);
	};

	auto cost = std::unordered_map<Vec2, std::uint32_t>{{start, 0}};
	auto previous = std::unordered_map<Vec2, Vec2>{{start, start}};
	auto queue = std::priority_queue<Node>{};
	queue.emplace(0, start);

	while (!queue.empty()) {
		const auto node = queue.top().position;
		if (node == destination) {
			break;
		}
		queue.pop();

		forEachNonSolidNeighbor(*this, node, red, blue, [&](Vec2 neighbor, std::uint32_t weight) {
			const auto newCost = cost[node] + weight;
			if (const auto result = cost.emplace(neighbor, newCost); result.second) {
				previous.emplace(neighbor, node);
				queue.emplace(newCost + heuristic(neighbor), neighbor);
			} else if (newCost < result.first->second) {
				result.first->second = newCost;
				previous.at(neighbor) = node;
				queue.emplace(newCost + heuristic(neighbor), neighbor);
			}
		});
	}

	// Make path.
	auto path = std::vector<Vec2>{};
	if (auto it = previous.find(destination); it != previous.end()) {
		// Path is valid.
		path.push_back(destination);
		do {
			if (it->second == start) {
				break;
			}
			path.push_back(it->second);
			it = previous.find(it->second);
		} while (it != previous.end());
	}
	return path;
}
