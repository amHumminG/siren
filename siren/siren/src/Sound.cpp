#include "siren/Sound.h"
#include "internal/log.h"
#include "siren/io/FileDataSource.h"
#include "siren/io/MemoryDataSource.h"
#include <fstream>
#include <memory>
#include <filesystem>

namespace siren {

	Sound::Sound(SoundType type, const std::string& tag)
		: m_type(type), m_tag(tag) {
	}

	void Sound::loadFromFile(const std::string& path) {

		if (!m_internalData) {
			m_internalData = std::make_shared<std::vector<std::byte>>();
		}

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
		m_internalData->resize(fileSize);
		if (!in.read(reinterpret_cast<char*>(m_internalData->data()), fileSize)) {
			SIREN_LOG_ERROR("Sound::Internal Failed to read file data. PATH: " << path);
			m_valid = false;
			m_internalData->clear();
			return;
		}

		m_dataView = std::span(m_internalData->data(), m_internalData->size());
		m_valid = true;
		return;
	}

	Sound Sound::Stream(const std::string& path, const std::string& tag) {
		std::string finalTag = tag;
		if (tag.empty()) {
			finalTag = std::filesystem::path(path).filename().string();
		}
		Sound sound(SoundType::Stream, finalTag);
		sound.m_path = path;

		if (!std::filesystem::exists(path)) {
			SIREN_LOG_ERROR("Sound::Stream Failed to find file: " << path);
			sound.m_valid = false;
		}

		return sound;
	}

	Sound Sound::Internal(const std::string& path, const std::string& tag) {
		std::string finalTag = tag.empty() ? path : tag;
		Sound sound(SoundType::MemoryInternal, finalTag);
		sound.m_path = path;

		sound.loadFromFile(path);
		return sound;
	}

	Sound Sound::External(std::span<const std::byte> externalData, const std::string& tag) {
		Sound sound(SoundType::MemoryExternal, tag);

		if (externalData.empty()) {
			SIREN_LOG_ERROR("Sound::External Invalid buffer (empty)");
			sound.m_valid = false;
		}
		else {
			sound.m_dataView = externalData;
		}

		return sound;
	}

	Result<std::unique_ptr<DataSource>> Sound::createDataSource() const {
		if (!m_valid) {
			return ResultCode::InvalidSound;
		}

		std::unique_ptr<DataSource> source = nullptr;
		if (m_type == SoundType::MemoryInternal) {
			// MemorySource shares data ownership
			source = std::make_unique<MemoryDataSource>(m_internalData);
		}
		else if (m_type == SoundType::MemoryExternal) {
			// MemorySource does not share data ownership (user controlled)
			source = std::make_unique<MemoryDataSource>(m_dataView);
		}
		else if (m_type == SoundType::Stream) {
			source = std::make_unique<FileDataSource>(m_path);
		}

		if (!source) {
			return ResultCode::GenericError;
		}

		return source;
	}

	bool Sound::isValid() const {
		return m_valid;
	}

	SoundType Sound::getType() const {
		return m_type;
	}

	const std::string& Sound::getTag() const {
		return m_tag;
	}

	const std::string& Sound::getPath() const {
		return m_path;
	}

	std::span<const std::byte> Sound::getData() const {
		return m_dataView;
	}
	
}