#include "siren/codecs/WavDecoder.h"

namespace siren {
	
	ResultCode siren::WavDecoder::decodeHeader() {
		// TODO: Implement support for Mono. For mono to stereo up-mixing,
		// we will need an intermediate buffer to hold the raw mono before expanding it to stereo

		// Verify data is a wav file
		SignatureChunk signatureChunk;
		if (m_dataSource->read(std::as_writable_bytes(std::span(&signatureChunk, 1))) != sizeof(signatureChunk)) {
			return ResultCode::InvalidHeader;
		}

		if (memcmp(signatureChunk.riff, "RIFF", 4) || memcmp(signatureChunk.wave, "WAVE", 4)) {
			return ResultCode::InvalidHeader;
		}

		bool formatChunkFound = false;
		bool dataChunkFound = false;

		// File is a wav file
		while (!formatChunkFound || !dataChunkFound) {
			Chunk chunk;
			if (m_dataSource->read(std::as_writable_bytes(std::span(&chunk, 1))) != sizeof(chunk)) {
				return ResultCode::InvalidHeader;
			}
			
			if (!memcmp(chunk.identifier, "fmt ", 4) && !formatChunkFound) {
				// Format chunk found -> Read into buffer
				FormatChunk formatData;
				if (m_dataSource->read(std::as_writable_bytes(std::span(&formatData, 1))) != sizeof(formatData)) {
					return ResultCode::InvalidHeader;
				}

				if (formatData.formatType != FORMAT_TYPE_PCM) {
					return ResultCode::FormatNotSupported;
				}

				if ((formatData.bitsPerSample != 8) && (formatData.bitsPerSample != 16)) {
					return ResultCode::FormatNotSupported;
				}
				// TODO: If support is added for other formats and bitsPerSample later on,
				// this should be refactored into a function-pointer strategy pattern where
				// we set a decoderoutine based on the format (m_decodeRoutine = &decode16Bit)

				m_sampleRate = formatData.sampleRate;
				m_channelCount = formatData.channelCount;

				if (m_channelCount == 0) {
					return ResultCode::InvalidHeader;
				}

				// Only supports stero as of now
				if (m_channelCount != 2) {
					return ResultCode::FormatNotSupported;
				}

				// Wav-unique
				m_bitsPerSample = formatData.bitsPerSample;
				m_blockAlign = formatData.blockAlign;

				formatChunkFound = true;
				
				// Accounting for potentially extended data formats
				size_t bytesRead = sizeof(formatData);
				if (chunk.size > bytesRead) {
					if (m_dataSource->seek(m_dataSource->tell() + (chunk.size - bytesRead)) != ResultCode::Success) {
						return ResultCode::InvalidHeader;
					}
				}
			}

			else if (!memcmp(chunk.identifier, "data", 4) && !dataChunkFound) {
				// Data chunk found -> Stop and save total frames

				if (!formatChunkFound) {
					// Faulty header order (format should appear before data)
					return ResultCode::InvalidHeader;
				}

				m_dataStartOffset = m_dataSource->tell();

				uint32_t dataSize = chunk.size;
				size_t bytesPerSample = m_bitsPerSample / 8;
				size_t frameSize = m_channelCount * bytesPerSample; // LR = one frame (stereo)
				m_totalFrames = dataSize / frameSize;

				dataChunkFound = true;

				break;
			}

			else if (m_dataSource->seek(m_dataSource->tell() + chunk.size) != ResultCode::Success) {
				return ResultCode::InvalidHeader;
			}
		}

		if (formatChunkFound && dataChunkFound) {
			return ResultCode::Success;
		}

		return ResultCode::InvalidHeader;
	}

	size_t WavDecoder::decode(std::span<float> dst) {
		size_t samples = dst.size();
		if (samples == 0) {
			return 0;
		}

		size_t framesToRead = samples / m_channelCount; // One float per channel
		size_t bytesNeeded = framesToRead * m_blockAlign;

		if (m_rawBuffer.size() < bytesNeeded) {
			m_rawBuffer.resize(bytesNeeded);
		}

		size_t bytesRead = m_dataSource->read(std::span(m_rawBuffer.data(), bytesNeeded));
		size_t framesRead = bytesRead / m_blockAlign;

		if (m_bitsPerSample == 8) {
			// 8-bit (0 -> 255)

			const uint8_t* raw = reinterpret_cast<const uint8_t*>(m_rawBuffer.data());

			for (size_t i = 0; i < framesRead * m_channelCount; i++) {
				// Convert to float (-1.0 -> 1.0)
				dst[i] = (raw[i] - 128.0f) / 128.0f;
			}
		}
		else if (m_bitsPerSample == 16) {
			// 16-bit (-32 768 -> 32 767)

			const int16_t* raw = reinterpret_cast<const int16_t*>(m_rawBuffer.data());

			for (size_t i = 0; i < framesRead * m_channelCount; i++) {
				// Convert to float (-1.0 -> 1.0)
				dst[i] = (raw[i] / 32768.0f);
			}
		}

		return framesRead;
	}

	ResultCode WavDecoder::seek(size_t frameIndex) {
		if (frameIndex > m_totalFrames) {
			return ResultCode::OutOfBounds;
		}

		size_t byteOffset = m_dataStartOffset + (frameIndex * m_blockAlign);

		return m_dataSource->seek(byteOffset);
	}

	size_t WavDecoder::tell() const {
		size_t pos = m_dataSource->tell();

		if (pos < m_dataStartOffset) {
			return 0;
		}

		size_t byteOffset = pos - m_dataStartOffset;
		return byteOffset / m_blockAlign;
	}

	uint32_t WavDecoder::getSampleRate() const {
		return m_sampleRate;
	}

	uint16_t WavDecoder::getChannelCount() const {
		return m_channelCount;
	}

	size_t WavDecoder::getTotalFrames() const {
		return m_totalFrames;
	}
}

