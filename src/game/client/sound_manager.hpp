#ifndef AF2_CLIENT_SOUND_MANAGER_HPP
#define AF2_CLIENT_SOUND_MANAGER_HPP

#include "../../utilities/resource.hpp" // util::Resource
#include "../data/sound_id.hpp"         // SoundId

#include <cstddef>       // std::size_t, std::ptrdiff_t
#include <soloud.h>      // SoLoud::...
#include <soloud_wav.h>  // SoLoud::Wav
#include <unordered_map> // std::unordered_map
#include <vector>        // std::vector

class SoundManager final {
public:
	SoundManager();
	~SoundManager();

	SoundManager(const SoundManager&) = delete;
	SoundManager(SoundManager&&) = delete;
	auto operator=(const SoundManager&) -> SoundManager& = delete;
	auto operator=(SoundManager&&) -> SoundManager& = delete;

	auto setGlobalVolume(float volume) noexcept -> void;
	auto setRolloffFactor(float rolloffFactor) noexcept -> void;
	auto setMaxSimultaneouslyPlayingSounds(std::size_t maxSimultaneous) noexcept -> void;

	auto update(float deltaTime, float x, float y, float z) noexcept -> void;

	auto loadSound(SoundId id, const char* filepath) noexcept -> bool;
	[[nodiscard]] auto isSoundLoaded(SoundId id) const noexcept -> bool;
	auto unloadSound(SoundId id) noexcept -> bool;

	auto playSound(SoundId id, float volume) noexcept -> bool;
	auto playSoundAtPosition(SoundId id, float x, float y, float z, float volume) noexcept -> bool;
	auto playSoundAtRelativePosition(SoundId id, float x, float y, float z, float volume) noexcept -> bool;

	auto playMusic(const char* filepath, float volume, bool looping) noexcept -> bool;
	[[nodiscard]] auto isMusicPlaying() const noexcept -> bool;
	auto stopMusic() noexcept -> void;

private:
	mutable SoLoud::Soloud m_soloud{};
	std::unordered_map<SoundId, SoLoud::Wav> m_loadedSounds{};
	std::unordered_map<SoundId, SoLoud::Wav> m_loadedSoundsRelative{};
	SoLoud::Wav m_loadedMusic{};
	SoLoud::handle m_music = 0;
	float m_rolloffFactor = 1.0f;
	float m_time = 0.0f;
};

#endif
