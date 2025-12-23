#pragma once
#include "siren/Decoder.h"
#include "siren/SirenMath.h"
#include <atomic>
#include <mutex>

enum class VoiceState {
	Inactive,
	Playing,
	Paused
};

enum class VoiceMode {
	Global, // Manual pan
	Spatial // Automatic pan and volume based on position and listener
};

namespace siren {

	class AudioBus;

	class Voice {
	private:
		AudioBus* m_bus = nullptr;

		std::unique_ptr<Decoder> m_decoder;
		
		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		VoiceMode m_mode = VoiceMode::Global;
		std::atomic<float> m_volume{ 1.0f };	// clamped between 0.0f and 1.0f
		std::atomic<float> m_pan{ 0.0f };	// clamped between -1.0f and 1.0f
		std::atomic<float> m_isLooping{ false };

		std::atomic<int64_t> m_seekFrame{ -1 }; // Seek request flag (-1 = No pending seek)
		uint32_t m_sampleRate = 0; // Stored to be used for frame to seconds conversion

		std::string m_tag; // Defaults to the tag that the sound held when this voice was constructed

		// Emitter data
		Vector3 m_position; // The position of the voice
		float m_minDistance = 1.0f; // Minimum distance the voice can be heard from (for volume scaling)
		float m_maxDistance = 50.0f; // Maximum distance the voice can be heard from (for volume scaling)

	public:
		Voice() = default;
		~Voice() = default;

		/// @brief Assigns a decoder for the voice to use
		/// @param decoder The decoder of an audio source
		void attachDecoder(std::unique_ptr<Decoder> decoder);

		/// @brief Mixes the buffer of its audio bus with decoded audio data if
		/// state is Playing
		/// 
		/// Supports [Mono, Stereo]
		/// @return True if voice is still alive, otherwise false
		bool mix(const ListenerData& listener);

		/// @brief Sets voice state to Playing
		///
		/// If voice state is Inactive, it will play from the beginning
		void play();

		/// @brief Sets voice state to Paused
		void pause();

		/// @brief Sets voice state to Inactive
		void stop();

		/// @param bus The bus that the voice will write to
		void setBus(AudioBus* bus);

		/// @param value New pan (clamped between -1.0f and 1.0f)
		void setPan(float value);

		/// @param value New value
		void setLooping(bool value);

		/// @param tag The tag given to the voice
		void setTag(const std::string& tag);

		/// @return The tag given to the voice
		const std::string& getTag();

		/// @brief Sends a request to seek to a given time point
		/// @param timePoint Represents the position (in seconds) to jump to
		void seek(float timePoint);

		/// @return Current pan
		[[nodiscard]] float getPan() const;

		/// @return True if voice is looping, otherwise false
		[[nodiscard]] bool isLooping() const;

		/// @return True if voice state = Playing, otherwise false
		[[nodiscard]] bool isPlaying() const;

		/// @brief Sets the voice position and sets mode to spatial if
		/// voice is in any other mode
		void setPosition(const Vector3& pos);

		/// @brief Sets voice mode to Global
		void setGlobal();

		/// @param minDistance The minimum distance the voice can be heard from
		/// @param maxDistance The maximum distance the voice can be heard from
		void setDistance(float minDistance, float maxDistance);

		/// @return The minimum distance the voice can be heard from
		float getMinDistance();

		/// @return The maximum distance the voice can be heard from
		float getMaxDistance();
	};
}