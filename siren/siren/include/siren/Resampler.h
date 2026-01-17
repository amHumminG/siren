#pragma once
#include <vector>
#include <span>
#include <functional>

namespace siren {

	class Resampler {
	private:
		uint16_t m_channels = 2;

		std::vector<float> m_inputBuffer;
		float m_cursor = 0.0f;
		size_t m_validInputFrames = 0; // Number of valid input frames currently stored in buffer

	public:
		static constexpr size_t MAX_BLOCK_SIZE = 256;
		static constexpr float MAX_PITCH = 4.0f;
		static constexpr size_t PADDING = 2; // For interpolation

		Resampler() = default;

		/// @brief Initializes resampler for a specified format
		void init(uint16_t channelCount);
		
		/// @brief Resets resampler state
		void flush() noexcept;

		/// @brief Fills the destination buffer with pitch shifted audio
		/// @param dst Destination buffer to be filled 
		/// @param pitch Pitch factor (clamped between 0.1 and 4.0)
		/// @param dataProvider Function that fetches raw data from the decoder
		/// @return Number of frames written to the destination buffer
		size_t getSamples(std::span<float> dst, float pitch,
			std::function<size_t(std::span<float>)> dataProvider) noexcept;
	};
}
