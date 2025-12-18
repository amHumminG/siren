#pragma once
#include "siren/Decoder.h"
#include <atomic>
#include <mutex>

enum class VoiceState {
	Inactive,
	Playing,
	Paused
};

namespace siren {

	class Voice {
	private:
		std::unique_ptr<Decoder> m_decoder;
		
		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		std::atomic<float> m_volume{ 1.0f };	// clamped between 0.0f and 1.0f
		std::atomic<float> m_pan{ 0.0f };	// clamped between -1.0f and 1.0f
		std::atomic<float> m_isLooping{ false };

		std::atomic<int64_t> m_seekFrame{ -1 }; // Seek request flag (-1 = No pending seek)
		uint32_t m_sampleRate = 0; // Stored to be used for frame to seconds conversion

	public:
		Voice() = default;
		~Voice() = default;

		/// @brief Assigns a decoder for the voice to use
		/// @param decoder The decoder of an audio source
		void attachDecoder(std::unique_ptr<Decoder> decoder);

		/// @brief Mixes the destination buffer with decoded audio data if
		/// state is Playing, otherwise fills buffer with silence
		/// 
		/// Supports [Mono, Stereo]
		/// @param dst Destination buffer
		/// @return True if voice is still alive, otherwise false
		bool mix(std::span<float> dst);

		/// @brief Sets voice state to Playing
		///
		/// If voice state is Inactive, it will play from the beginning
		void play();

		/// @brief Sets voice state to Paused
		void pause();

		/// @brief Sets voice state to Inactive
		void stop();

		/// @param value New volume (clamped between 0.0f and 1.0f)
		void setVolume(float value);

		/// @param value New pan (clamped between -1.0f and 1.0f)
		void setPan(float value);

		/// @param value New value
		void setLooping(bool value);

		/// @brief Sends a request to seek to a given time point
		/// @param timePoint Represents the position (in seconds) to jump to
		void seek(float timePoint);

		/// @return Current volume
		[[nodiscard]] float getVolume();

		/// @return Current pan
		[[nodiscard]] float getPan() const;

		/// @return True if voice is looping, otherwise false
		[[nodiscard]] bool isLooping() const;

		/// @return True if voice state = Playing, otherwise false
		[[nodiscard]] bool isPlaying() const;
	};
}