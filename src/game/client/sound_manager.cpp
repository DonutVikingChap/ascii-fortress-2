#include "sound_manager.hpp"

#include <fmt/core.h> // fmt::format
#include <limits>     // std::numeric_limits
#include <stdexcept>  // std::runtime_error

SoundManager::SoundManager() {
	if (const auto err = m_soloud.init(); err != SoLoud::SO_NO_ERROR) {
		throw std::runtime_error{fmt::format("Failed to initialize sound manager: {}", m_soloud.getErrorString(err))};
	}
}

SoundManager::~SoundManager() {
	m_soloud.deinit();
}

auto SoundManager::setGlobalVolume(float volume) noexcept -> void {
	m_soloud.setGlobalVolume(volume);
}

auto SoundManager::setRolloffFactor(float rolloffFactor) noexcept -> void {
	m_rolloffFactor = rolloffFactor;
	for (auto& [id, sound] : m_loadedSounds) {
		sound.set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, m_rolloffFactor);
	}
	for (auto& [id, sound] : m_loadedSoundsRelative) {
		sound.set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, m_rolloffFactor);
	}
}

auto SoundManager::setMaxSimultaneouslyPlayingSounds(std::size_t maxSimultaneous) noexcept -> void {
	m_soloud.setMaxActiveVoiceCount(static_cast<unsigned>(maxSimultaneous));
}

auto SoundManager::update(float deltaTime, float x, float y, float z) noexcept -> void {
	m_time += deltaTime;
	m_soloud.set3dListenerPosition(x, y, z);
	m_soloud.update3dAudio();
}

auto SoundManager::loadSound(SoundId id, const char* filepath) noexcept -> bool {
	if (auto& sound = m_loadedSounds[id]; sound.load(filepath) == SoLoud::SO_NO_ERROR) {
		sound.setInaudibleBehavior(false, true);
		sound.set3dMinMaxDistance(1.0f, std::numeric_limits<float>::max());
		sound.set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, m_rolloffFactor);
		sound.set3dDopplerFactor(0.0f);
		sound.set3dDistanceDelay(false);
		sound.set3dListenerRelative(false);
	} else {
		m_loadedSoundsRelative.erase(id);
		m_loadedSounds.erase(id);
		return false;
	}
	if (auto& sound = m_loadedSoundsRelative[id]; sound.load(filepath) == SoLoud::SO_NO_ERROR) {
		sound.setInaudibleBehavior(false, true);
		sound.set3dMinMaxDistance(1.0f, std::numeric_limits<float>::max());
		sound.set3dAttenuation(SoLoud::AudioSource::INVERSE_DISTANCE, m_rolloffFactor);
		sound.set3dDopplerFactor(0.0f);
		sound.set3dDistanceDelay(false);
		sound.set3dListenerRelative(true);
	} else {
		m_loadedSoundsRelative.erase(id);
		m_loadedSounds.erase(id);
		return false;
	}
	return true;
}

auto SoundManager::isSoundLoaded(SoundId id) const noexcept -> bool {
	return m_loadedSounds.count(id) != 0;
}

auto SoundManager::unloadSound(SoundId id) noexcept -> bool {
	return m_loadedSounds.erase(id) != 0;
}

auto SoundManager::playSound(SoundId id, float volume) noexcept -> bool {
	return this->playSoundAtRelativePosition(id, 0.0f, 0.0f, 0.0f, volume);
}

auto SoundManager::playSoundAtPosition(SoundId id, float x, float y, float z, float volume) noexcept -> bool {
	if (const auto it = m_loadedSounds.find(id); it != m_loadedSounds.end() && it->second.mData) {
		m_soloud.play3dClocked(m_time, it->second, x, y, z, 0.0f, 0.0f, 0.0f, volume);
		m_soloud.update3dAudio();
		return true;
	}
	return false;
}

auto SoundManager::playSoundAtRelativePosition(SoundId id, float x, float y, float z, float volume) noexcept -> bool {
	if (const auto it = m_loadedSoundsRelative.find(id); it != m_loadedSoundsRelative.end() && it->second.mData) {
		m_soloud.play3dClocked(m_time, it->second, x, y, z, 0.0f, 0.0f, 0.0f, volume);
		m_soloud.update3dAudio();
		return true;
	}
	return false;
}

auto SoundManager::playMusic(const char* filepath, float volume, bool looping) noexcept -> bool {
	this->stopMusic();
	if (m_loadedMusic.load(filepath) != SoLoud::SO_NO_ERROR) {
		return false;
	}
	m_music = m_soloud.playBackground(m_loadedMusic, volume);
	m_soloud.setProtectVoice(m_music, true);
	m_soloud.setLooping(m_music, looping);
	return true;
}

auto SoundManager::isMusicPlaying() const noexcept -> bool {
	return m_soloud.isValidVoiceHandle(m_music);
}

auto SoundManager::stopMusic() noexcept -> void {
	m_soloud.stop(m_music);
	m_music = 0;
}
