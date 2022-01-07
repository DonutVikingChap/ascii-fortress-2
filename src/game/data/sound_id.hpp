#ifndef AF2_DATA_SOUND_ID_HPP
#define AF2_DATA_SOUND_ID_HPP

#include "../../debug.hpp" // Msg, DEBUG_MSG
#include "data_type.hpp"   // DataType

#include <array>       // std::array
#include <cstdint>     // std::uint8_t
#include <string_view> // std::string_view

// clang-format off
// X(name, filename)
#define ENUM_SOUND_IDS(X) \
	X(none,					"") \
	X(player_spawn,			"player_spawn.ogg") \
	X(player_death,			"player_death.ogg") \
	X(we_picked_intel, 		"we_picked_intel.ogg") \
	X(they_picked_intel,	"they_picked_intel.ogg") \
	X(we_dropped_intel,		"we_dropped_intel.ogg") \
	X(they_dropped_intel,	"they_dropped_intel.ogg") \
	X(we_returned_intel,	"we_returned_intel.ogg") \
	X(they_returned_intel,	"they_returned_intel.ogg") \
	X(we_captured_intel,	"we_captured_intel.ogg") \
	X(they_captured_intel,	"they_captured_intel.ogg") \
	X(sentry_build,			"sentry_build.ogg") \
	X(sentry_death,			"sentry_death.ogg") \
	X(medkit_spawn,			"medkit_spawn.ogg") \
	X(medkit_collect,		"medkit_collect.ogg") \
	X(explosion,			"explosion.ogg") \
	X(hitsound,				"hitsound.ogg") \
	X(dry_fire,				"dry_fire.ogg") \
	X(shoot_scattergun,		"shoot_scattergun.ogg") \
	X(shoot_rocket,			"shoot_rocket.ogg") \
	X(shoot_flame,			"shoot_flame.ogg") \
	X(shoot_sticky,			"shoot_sticky.ogg") \
	X(shoot_minigun,		"shoot_minigun.ogg") \
	X(shoot_shotgun,		"shoot_shotgun.ogg") \
	X(shoot_heal_beam,		"shoot_heal_beam.ogg") \
	X(shoot_syringe,		"shoot_syringe.ogg") \
	X(shoot_sniper,			"shoot_sniper.ogg") \
	X(shoot_sentry,			"shoot_sentry.ogg") \
	X(reload_rocket,		"reload_rocket.ogg") \
	X(reload_scattergun,	"reload_scattergun.ogg") \
	X(reload_shotgun,		"reload_shotgun.ogg") \
	X(reload_sniper,		"reload_sniper.ogg") \
	X(reload_sticky,		"reload_sticky.ogg") \
	X(spy_kill,				"spy_kill.ogg") \
	X(player_hurt,			"player_hurt.ogg") \
	X(player_heal,			"player_heal.ogg") \
	X(player_hurt_flame,	"player_hurt_flame.ogg") \
	X(sentry_hurt, 			"sentry_hurt.ogg") \
	X(victory,				"victory.ogg") \
	X(defeat,				"defeat.ogg") \
	X(chat_message,			"chat_message.ogg") \
	X(resupply,				"resupply.ogg") \
	X(spy_disguise,			"spy_disguise.ogg") \
	X(stalemate,			"stalemate.ogg") \
	X(push_cart,			"push_cart.ogg") \
	X(ends_1sec,			"ends_1sec.ogg") \
	X(ends_2sec,			"ends_2sec.ogg") \
	X(ends_3sec,			"ends_3sec.ogg") \
	X(ends_4sec,			"ends_4sec.ogg") \
	X(ends_5sec,			"ends_5sec.ogg") \
	X(ends_10sec,			"ends_10sec.ogg") \
	X(ends_30sec,			"ends_30sec.ogg") \
	X(ends_60sec,			"ends_60sec.ogg") \
	X(ends_5min,			"ends_5min.ogg") \
	X(achievement,			"achievement.ogg")
// clang-format on

class SoundId final : public DataType<SoundId, std::uint8_t> {
private:
	using DataType::DataType;

	static constexpr auto FILENAMES = std::array{
#define X(name, filename) std::string_view{filename},
		ENUM_SOUND_IDS(X)
#undef X
	};

public:
	enum class Index : ValueType {
#define X(name, file) name,
		ENUM_SOUND_IDS(X)
#undef X
	};

	constexpr operator Index() const noexcept {
		return static_cast<Index>(m_value);
	}

#define X(name, file) \
	[[nodiscard]] static constexpr auto name() noexcept { \
		return SoundId{static_cast<ValueType>(Index::name)}; \
	}
	ENUM_SOUND_IDS(X)
#undef X

	constexpr SoundId() noexcept
		: SoundId(SoundId::none()) {}

	[[nodiscard]] static constexpr auto getAll() noexcept {
		return std::array{
#define X(name, file) SoundId::name(),
			ENUM_SOUND_IDS(X)
#undef X
		};
	}

	[[nodiscard]] constexpr auto getFilename() const noexcept {
		return FILENAMES[m_value];
	}

	[[nodiscard]] static auto findByFilename(std::string_view inputFilename) noexcept -> SoundId;

	template <typename Stream>
	friend constexpr auto operator<<(Stream& stream, const SoundId& val) -> Stream& {
		return stream << val.m_value;
	}

	template <typename Stream>
	friend constexpr auto operator>>(Stream& stream, SoundId& val) -> Stream& {
		constexpr auto size = SoundId::getAll().size();
		if (!(stream >> val.m_value) || val.m_value >= size) {
			DEBUG_MSG(Msg::CONNECTION_DETAILED, "Read invalid SoundId value \"{}\".", val.m_value);
			val.m_value = 0;
			stream.invalidate();
		}
		return stream;
	}
};

#ifndef SHARED_DATA_SOUND_ID_KEEP_ENUM_SOUND_IDS
#undef ENUM_SOUND_IDS
#endif

template <>
class std::hash<SoundId> {
public:
	auto operator()(const SoundId& id) const -> std::size_t {
		return m_hasher(id.value());
	}

private:
	std::hash<SoundId::ValueType> m_hasher{};
};

#endif
