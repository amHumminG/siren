#pragma once
#include <vector>
#include <atomic>
#include <string>

namespace siren {

	class AudioBus {
	public:
		std::string m_name;

		static constexpr size_t MAX_BUFFER_SIZE = 8192 * 2; // Stereo
		std::vector<float> m_buffer;

		std::atomic<float> m_volume;

		AudioBus(const std::string& name);

		/// @brief Prepares the bus buffer to be mixed
		/// @param frameCount The amount of frames requested by the audio callback
		/// @param channelCount The amount of channels of the audio output
		void prepare(size_t frameCount, size_t channelCount);
	};
}