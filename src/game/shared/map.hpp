#ifndef AF2_SHARED_MAP_HPP
#define AF2_SHARED_MAP_HPP

#include "../../console/script.hpp"        // Script
#include "../../utilities/crc.hpp"         // util::CRC32
#include "../../utilities/span.hpp"        // util::Span, util::asBytes
#include "../../utilities/tile_matrix.hpp" // util::TileMatrix
#include "../data/direction.hpp"           // Direction
#include "../data/vector.hpp"              // Vec2

#include <string>      // std::string
#include <string_view> // std::string_view
#include <utility>     // std::forward
#include <vector>      // std::vector

class Map final {
public:
	static constexpr auto AIR_CHAR = ' ';
	static constexpr auto ONEWAY_LEFT_CHAR = '<';
	static constexpr auto ONEWAY_RIGHT_CHAR = '>';
	static constexpr auto ONEWAY_UP_CHAR = '^';
	static constexpr auto ONEWAY_DOWN_CHAR = 'v';

	[[nodiscard]] static constexpr auto isSolidChar(char ch) noexcept -> bool {
		return ch != AIR_CHAR && ch != ONEWAY_LEFT_CHAR && ch != ONEWAY_RIGHT_CHAR && ch != ONEWAY_UP_CHAR && ch != ONEWAY_DOWN_CHAR;
	}

	auto unLoad() -> void;

	auto load(std::string name, std::string_view str) -> bool;

	[[nodiscard]] auto isLoaded() const noexcept -> bool;

	[[nodiscard]] auto getWidth() const noexcept -> Vec2::Length;
	[[nodiscard]] auto getHeight() const noexcept -> Vec2::Length;

	[[nodiscard]] auto getHash() const noexcept -> util::CRC32;
	[[nodiscard]] auto getName() const noexcept -> std::string_view;
	[[nodiscard]] auto getMatrix() const noexcept -> const util::TileMatrix<char>&;
	[[nodiscard]] auto getResources() const noexcept -> util::Span<const std::string>;
	[[nodiscard]] auto getScript() const noexcept -> const Script&;
	[[nodiscard]] auto getRedCartSpawn() const noexcept -> Vec2;
	[[nodiscard]] auto getBlueCartSpawn() const noexcept -> Vec2;
	[[nodiscard]] auto getRedCartPath() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getBlueCartPath() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getRedFlagSpawns() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getBlueFlagSpawns() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getRedSpawns() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getBlueSpawns() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getRedRespawnRoomVisualizers() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getBlueRespawnRoomVisualizers() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getResupplyLockers() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getMedkitSpawns() const noexcept -> util::Span<const Vec2>;
	[[nodiscard]] auto getAmmopackSpawns() const noexcept -> util::Span<const Vec2>;

	[[nodiscard]] auto get(Vec2 p, char defaultVal = '\0') const noexcept -> char;

	[[nodiscard]] auto isResupplyLocker(Vec2 p) const noexcept -> bool;
	[[nodiscard]] auto isRedRespawnRoomVisualizer(Vec2 p) const noexcept -> bool;
	[[nodiscard]] auto isBlueRespawnRoomVisualizer(Vec2 p) const noexcept -> bool;

	[[nodiscard]] auto isSolid(Vec2 p, bool red, bool blue) const noexcept -> bool;
	[[nodiscard]] auto isSolid(Vec2 p, bool red, bool blue, Direction moveDirection) const noexcept -> bool;

	[[nodiscard]] auto lineOfSight(Vec2 p1, Vec2 p2) const noexcept -> bool;

	// Find a walkable path from start to destination.
	// Returns an empty vector if no path was found.
	// A valid path always includes the destination as the first element and should be traversed in reverse order.
	// The start position is not included in the path, unless it is the same as the destination.
	[[nodiscard]] auto findPath(Vec2 start, Vec2 destination, bool red, bool blue) const -> std::vector<Vec2>;

private:
	util::TileMatrix<char> m_matrix{};
	std::string m_name{};
	util::CRC32 m_hash{};
	Vec2 m_redCartSpawn{};
	Vec2 m_blueCartSpawn{};
	std::vector<Vec2> m_redCartPath{};
	std::vector<Vec2> m_blueCartPath{};
	std::vector<Vec2> m_redFlagSpawns{};
	std::vector<Vec2> m_blueFlagSpawns{};
	std::vector<Vec2> m_redSpawns{};
	std::vector<Vec2> m_blueSpawns{};
	std::vector<Vec2> m_redRespawnRoomVisualizers{};
	std::vector<Vec2> m_blueRespawnRoomVisualizers{};
	std::vector<Vec2> m_resupplyLockers{};
	std::vector<Vec2> m_medkitSpawns{};
	std::vector<Vec2> m_ammopackSpawns{};
	std::vector<std::string> m_resources{};
	Script m_script{};
};

#endif
