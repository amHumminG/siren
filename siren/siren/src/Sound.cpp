#include "siren/Sound.h"
#include "internal/log.h"
#include <fstream>
#include <filesystem>

namespace siren {

	Sound::Sound(SoundType type) 
		: m_type(type) {
	}

	void Sound::loadFromFile(const std::string& path) {
		std::ifstream in(path, std::ios::binary | std::ios::ate);
		if (!in.is_open()) {
			SIREN_LOG_ERROR("Sound::Internal Failed to open file. PATH: " << path);
			m_valid = false;
			return;
		}

		size_t fileSize = static_cast<size_t>(in.tellg());
		if (fileSize <= 0) {
			SIREN_LOG_ERROR("Sound::Internal File is empty. PATH: " << path);
			m_valid = false;
			return;
		}
		in.seekg(0, std::ios::beg);
		m_internalData.resize(fileSize);
		if (!in.read(reinterpret_cast<char*>(m_internalData.data()), fileSize)) {
			SIREN_LOG_ERROR("Sound::Internal Failed to read file data. PATH: " << path);
			m_valid = false;
			m_internalData.clear();
			return;
		}

		m_dataView = std::span(m_internalData);
		return;
	}

	Sound Sound::Stream(const std::string& path) {
		Sound sound(SoundType::Stream);
		sound.m_path = path;

		if (!std::filesystem::exists(path)) {
			SIREN_LOG_ERROR("Sound::Stream Failed to find file: " << path);
			sound.m_valid = false;
		}

		return sound;
	}

	Sound Sound::Internal(const std::string& path) {
		Sound sound(SoundType::MemoryInternal);
		sound.m_path = path;

		sound.loadFromFile(path);
		return sound;
	}

	Sound Sound::External(std::span<const std::byte> externalData) {
		Sound sound(SoundType::MemoryExternal);

		if (externalData.empty()) {
			SIREN_LOG_ERROR("Sound::External Invalid buffer (empty)");
			sound.m_valid = false;
		}
		else {
			sound.m_dataView = externalData;
		}

		return sound;
	}

	bool Sound::isValid() const {
		return m_valid;
	}

	SoundType Sound::getType() const {
		return m_type;
	}

	const std::string& Sound::getPath() const {
		return m_path;
	}

	std::span<const std::byte> Sound::getData() const {
		return m_dataView;
	}
	
}