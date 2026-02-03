#pragma once
#include "siren/Result.h"
#include "siren/DataSource.h"
#include <string>
#include <memory>
#include <vector>
#include <span>

namespace siren {

	class AudioContext;

	class Sound {
	public:
		enum class SoundType {
			Stream,				// Stream from disk
			MemoryInternal,		// Sound loads and owns the memory
			MemoryExternal		// Sound holds a view to the memory (user owns it)
		};

		Sound() = default;

		/// @brief Creates a Sound that streams audio directly from the disk.
		/// 
		/// The audio data is not loaded into memory. Instead, a file handle is opened
		/// and data is read in small chunks during playback.
		/// 
		/// Recommended for playing longer audio files like music tracks or ambiance.
		/// 
		/// @note It is safe to destroy this Sound object while it is being played by a Voice.
		/// The Voice retains its own copy of the file path and manages the file handle
		/// independently.
		/// 
		/// @param path The absolute or relative path to the audio file.
		/// @param tag Optional identifier. Defaults to filename if left empty.
		/// @return A Sound object. Check isValid() before use.
		[[nodiscard]] static Sound Stream(const std::string& path, const std::string& tag = "");

		/// @brief Creates a Sound by loading a file completely into memory.
		/// 
		/// The memory is allocated and owned by the Sound itself.
		/// 
		/// Recommended for shorter, frequently played sounds like sound effects.
		/// 
		/// @note It is safe to destroy this Sound object while it is being used by a Voice.
		/// The Voice holds a shared reference to the audio data, ensuring it stays alive
		/// until the Voice is destroyed.
		/// 
		/// @param path The absolute or relative path to the audio file.
		/// @param tag Optional identifier. Defaults to filename if left empty.
		/// @return A Sound object. Check isValid() before use.
		[[nodiscard]] static Sound Internal(const std::string& path, const std::string& tag = "");

		/// @brief Creates a Sound from an existing memory buffer (non-copy)
		/// 
		/// The Sound only holds a non-owning view over the provided data. It
		/// only reads from the pointer it has been given.
		/// 
		/// @note It is safe to destroy this Sound object while it is being played by a Voice.
		/// 
		/// @attention Ensure the memory pointed to by @p externalData remains valid
		/// for the entire lifespan of any Voice created from this Sound.
		/// 
		/// @c Voice->destroy() must be called on all associated voices before de-allocation
		/// of the external buffer. Failure to do so will result in undefined behaviour.
		/// 
		/// @param externalData A view covering the raw audio file data in memory.
		/// @param tag Optional identifier. Defaults to "External_Memory" if left empty.
		/// @return A Sound object. Check isValid() before use.
		[[nodiscard]] static Sound External(std::span<const std::byte> externalData, const std::string& tag = "External_Memory");

		Sound(const Sound&) = delete;
		Sound& operator=(const Sound&) = delete;

		Sound(Sound&&) = default;
		Sound& operator=(Sound&&) = default;

		/// @brief Checks if the Sound was successfully initialized and is ready for use.
		/// @return @c true if the sound is ready to be played.
		bool isValid() const;

		/// @return The storage type of the Sound [Stream, MemoryInternal, MemoryExternal].
		SoundType getType() const;

		/// @return The identifier tag given to this sound.
		const std::string& getTag() const;

		/// @brief Gets the file path associated with the Sound.
		/// @return The file path string. Returns an empty string if the sound is of type @b MemoryExternal.
		const std::string& getPath() const;

		/// @brief Gets a view of the raw audio data in memory.
		/// 
		/// - For Sounds of type @b MemoryInternal or @b MemoryExternal: Returns a valid span
		/// covering the audio data.
		/// 
		/// - For Sounds of type @b Stream: Returns an emtpy span.
		/// @return A read-only span of bytes.
		std::span<const std::byte> getData() const;

	private:
		friend class AudioContext;

		/// @brief Fills m_internalData with file data. If it failed to read the
		/// file into memory, m_isValid is set to false.
		/// @param path Path to the file.
		void loadFromFile(const std::string& path);

		/// @brief Creates a datasource that can be used by a decoder to play a Voice.
		/// @return A pointer to a DataSource object.
		[[nodiscard]] Result<std::unique_ptr<DataSource>> createDataSource() const;

		bool m_valid = false;
		SoundType m_type;
		std::string m_tag;

		std::string m_path; // Used by [Stream, MemoryInternal]
		std::shared_ptr<std::vector<std::byte>> m_internalData; // Used by [MemoryInternal]
		std::span<const std::byte> m_dataView; // Used by [MemoryInternal, MemoryExternal]
	};
}