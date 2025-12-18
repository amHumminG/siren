#pragma once
#include "siren/Decoder.h"
#include <atomic>
#include <mutex>

enum VoiceState {
	Inactive,
	Playing,
	Paused
};

namespace siren {

	class Voice {
	private:
		std::mutex m_mutex;
		std::unique_ptr<Decoder> m_decoder;
		
		std::atomic<VoiceState> m_state{ VoiceState::Inactive };
		std::atomic<float> m_volume{ 1.0f };	// clamped between 0.0f and 1.0f
		std::atomic<float> m_pan{ 0.0f };	// clamped between -1.0f and 1.0f
		std::atomic<float> m_isLooping{ false };

	public:
		Voice() = default;
		~Voice() = default;

		/// @brief Assigns a decoder for the voice to use
		/// @param decoder The decoder of an audio source
		void attachDecoder(std::unique_ptr<Decoder> decoder);

		/// @brief Fills the destination buffer with audio data
		/// 
		/// If voice state = Playing: buffer will be filled with decoded audio data
		/// 
		/// If voice state = Paused/Inactive: buffer will be filled with silence
		/// @param dst Destination buffer
		void process(std::span<float> dst);

		/// @brief Sets voice state to Playing.
		///
		/// If voice state = Inactive: Resets audio cursor to start
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

		/// @return Current volume
		[[nodiscard]] float getVolume();

		/// @return Current pan
		[[nodiscard]] float getPan();

		/// @return True if voice is looping, otherwise false
		[[nodiscard]] bool isLooping();

		/// @return True if voice state = Playing, otherwise false
		[[nodiscard]] bool isPlaying();
	};
}