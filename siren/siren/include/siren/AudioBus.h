#pragma once
#include <vector>
#include <atomic>
#include <string>
#include <span>

namespace siren {

	class AudioBus {
	public:
		AudioBus();

		/// @brief Sets the output volume.
		/// 
		/// The value is clamped between @c 0.0 (silence) and @c 1.0 (full volume).
		/// @param value The volume.
		void setVolume(float value);

		/// @return The current volume (range: [0.0, 1.0]).
		float getVolume();

	private:
		friend class AudioContext;
		friend class Voice;

		/// @brief Prepares the bus buffer to be mixed
		/// @param frameCount The amount of frames requested by the audio callback
		/// @param channelCount The amount of channels of the audio output
		void prepare(size_t frameCount, size_t channelCount);

		/// @brief Calculates and applies the bus volume to its buffer
		void process();

		static constexpr size_t MAX_BUFFER_SIZE = 8192 * 2; // Stereo
		std::vector<float> m_buffer;

		std::atomic<float> m_volume{ 1.0f };
		float m_currentGain = 1.0f;
	};
}