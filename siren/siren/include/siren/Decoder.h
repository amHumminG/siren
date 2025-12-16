#pragma once
#include "siren/DataSource.h"
#include <memory>
#include <vector>

namespace siren {

	class Decoder {
	protected:
		std::unique_ptr<DataSource> m_dataSource;

		std::vector<std::byte> m_rawBuffer;

		uint32_t m_sampleRate;
		uint16_t m_channelCount;
		size_t m_totalFrames;

		/// @brief Decodes the header of the audio source
		/// @return ResultCode::Success if header is fully read and decoded, otherwise error
		virtual ResultCode decodeHeader() = 0;

	public:
		virtual ~Decoder() = default;

		/// @brief Reads the header and verifies the audio format
		/// @param dataSource Audio source
		/// @return ResultCode::Success if format is correct and header is valid, otherwise error
		[[nodiscard]] virtual ResultCode prime(std::unique_ptr<DataSource> audioData) {
			if (!audioData->isValid()) {
				return ResultCode::InvalidData;
			}
			m_dataSource = std::move(audioData);
			m_rawBuffer.reserve(16384); // Reserve standard to avoid memory allocation in decode()
			return decodeHeader();
		}

		/// @brief Reads and decodes audio data into the destination buffer
		/// @param dst Destination buffer
		/// @return The number of frames decoded and read into the destination buffer
		[[nodiscard]] virtual size_t decode(std::span<float> dst) = 0;

		/// @brief Moves the cursor of the audio source to a specific frame index
		/// @param frameIndex The index
		/// @return ResultCode::Success if cursor was moved to frame index, otherwise error
		[[nodiscard]] virtual ResultCode seek(size_t frameIndex) = 0;

		/// @return The frame index that the cursor is currently looking at
		[[nodiscard]] virtual size_t tell() const = 0;

		/// @return The sample rate of the audio source
		[[nodiscard]] virtual uint32_t getSampleRate() const = 0;

		/// @return The number of channels of the audio source
		[[nodiscard]] virtual uint16_t getChannelCount() const = 0;

		/// @return The total number of frames in the audio source
		[[nodiscard]] virtual size_t getTotalFrames() const = 0;
	};
}