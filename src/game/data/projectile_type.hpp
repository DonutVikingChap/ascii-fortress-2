#ifndef AF2_DATA_PROJECTILE_TYPE_HPP
#define AF2_DATA_PROJECTILE_TYPE_HPP

#include "../../debug.hpp" // Msg, DEBUG_MSG
#include "data_type.hpp"   // DataType

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

// clang-format off
// X(name, str)
#define ENUM_PROJECTILE_TYPES(X) \
	X(none,			"") \
	X(bullet,		"Bullet") \
	X(rocket,		"Rocket") \
	X(sticky,		"Stickybomb") \
	X(flame,		"Flame") \
	X(heal_beam,	"Heal Beam") \
	X(syringe,		"Syringe") \
	X(sniper_trail,	"Sniper Trail")
// clang-format on

class ProjectileType final : public DataType<ProjectileType, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto NAMES = std::array{
#define X(name, str) std::string_view{str},
		ENUM_PROJECTILE_TYPES(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, str) name,
		ENUM_PROJECTILE_TYPES(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, str) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return ProjectileType{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_PROJECTILE_TYPES(X)
#undef X

	constexpr ProjectileType() noexcept
		: ProjectileType(ProjectileType::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, str) ProjectileType::name(),
			ENUM_PROJECTILE_TYPES(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getName() const noexcept {
		return NAMES[m_value];
	}

	[[nodiscard]] constexpr auto getId() const noexcept {
		return m_value;
	}

	[[nodiscard]] auto getChar() const noexcept -> char;
	[[nodiscard]] auto getMoveInterval() const noexcept -> float;
	[[nodiscard]] auto getDisappearTime() const noexcept -> float;

	[[nodiscard]] static auto findByName(std::string_view inputName) noexcept -> ProjectileType;
	[[nodiscard]] static auto findById(ValueType id) noexcept -> ProjectileType;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const ProjectileType& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, ProjectileType& val) -> Stream& {
		constexpr auto size = ProjectileType::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid ProjectileType value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_DATA_PROJECTILE_TYPE_KEEP_ENUM_PROJECTILE_TYPES
#undef ENUM_PROJECTILE_TYPES
#endif

template <>
class std::hash<ProjectileType> {
public:
	auto operator()(const ProjectileType& type) const -> std::size_t {
		return m_hasher(type.value());
	}

private:
	std::hash<ProjectileType::ValueType> m_hasher{};
};

#endif
