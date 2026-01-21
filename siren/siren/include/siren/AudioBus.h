#pragma once
#include <vector>
#include <atomic>
#include <string>
#include <span>

namespace siren {

	class AudioBus {
	public:
		static constexpr size_t MAX_BUFFER_SIZE = 8192 * 2; // Stereo
		std::vector<float> m_buffer;

		std::atomic<float> m_volume{ 1.0f };
		float m_currentGain = 1.0f;

		AudioBus();

		/// @brief Prepares the bus buffer to be mixed
		/// @param frameCount The amount of frames requested by the audio callback
		/// @param channelCount The amount of channels of the audio output
		void prepare(size_t frameCount, size_t channelCount);

		/// @brief Calculates and applies the bus volume to its buffer
		void process();
	};
}