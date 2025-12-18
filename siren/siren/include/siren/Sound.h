#pragma once
#include "siren/Result.h"
#include <string>
#include <vector>
#include <span>

namespace siren {

	enum class SoundType {
		Stream,				// Stream from disk
		MemoryInternal,		// Sound loads and owns the memory
		MemoryExternal		// Sound holds a view to the memory (user owns it)
	};

	class Sound {
	private:
		SoundType m_type;
		bool m_valid;

		std::string m_path; // Valid if m_type != MemoryExternal
		std::vector<std::byte> m_internalData; // Valid if m_type == MemoryInternal

		std::span<const std::byte> m_dataView; // Valid if m_type != Stream

		Sound(SoundType type);

		/// @brief Fills m_internalData with file data
		/// @param path Path to the file
		/// @return ResultCode::Success if file was successfully read into m_internalData,
		/// otherwise false
		ResultCode loadFromFile(const std::string& path);

	public:
		/// @brief Constructs a sound object for streaming from an audio file
		/// @param path Path to the file
		/// @return Sound object
		static Sound Stream(const std::string& path);

		/// @brief Constructs a sound object that loads an audio file into memory (ownership)
		/// @param path Path to the file to be loaded
		/// @return Sound object
		static Sound Internal(const std::string& path);

		/// @brief Constructs a sound object with a view of a sound file loaded into memory
		/// (non-ownership)
		/// @param externalData Sound file in memory
		/// @return Sound object
		static Sound External(std::span<const std::byte> externalData);

		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		Sound(Sound&&) = default;
		Sound& operator=(Sound&&) = default;

		/// @return True if sound is valid, otherwise false
		[[nodiscard]] bool isValid() const;

		/// @return The type of the sound object
		[[nodiscard]] SoundType getType() const;

		/// @return The path to the audio file if type is Stream/MemoryInternal, otherwise empty
		[[nodiscard]] const std::string& getPath() const;

		/// @return View of the file data in memory if type is MemoryInternal/MemoryExternal, 
		/// otherwise empty
		[[nodiscard]] std::span<const std::byte> getData() const;
	};
}