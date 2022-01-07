#define SHARED_DATA_SOUND_ID_KEEP_ENUM_SOUND_IDS
#include "sound_id.hpp"

#include "../../utilities/string.hpp" // util::ifind

auto SoundId::findByFilename(std::string_view inputFilename) noexcept -> SoundId {
	if (inputFilename.empty()) {
		return SoundId::none();
	}

#define X(name, filename) \
	if constexpr (SoundId::name() != SoundId::none()) { \
		if (util::ifind(std::string_view{filename}, inputFilename) == 0) { \
			return SoundId::name(); \
		} \
	}
	ENUM_SOUND_IDS(X)
#undef X

	return SoundId::none();
}
