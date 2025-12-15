#include "siren/codecs/WavDecoder.h"
#include <vector>

namespace siren {
	
	ResultCode siren::WavDecoder::decodeHeader() {
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

				m_sampleRate = formatData.sampleRate;
				m_channelCount = formatData.channelCount;

				// Wav-unique
				m_bitsPerSample = formatData.bitsPerSample;
				m_blockAlign = formatData.blockAlign;

				formatChunkFound = true;
				
				// Accounting for potentially extended data formats
				size_t bytesRead = sizeof(formatData);
				if (chunk.chunkSize > bytesRead) {
					if (m_dataSource->seek(m_dataSource->tell() + (chunk.chunkSize - bytesRead)) != ResultCode::Success) {
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

				uint32_t dataSize = chunk.chunkSize;
				size_t bytesPerSample = m_bitsPerSample / 8;
				size_t frameSize = m_channelCount * bytesPerSample; // LR = one frame if stereo
				m_totalFrames = dataSize / frameSize;

				break;
			}

			else if (m_dataSource->seek(m_dataSource->tell() + chunk.chunkSize) != ResultCode::Success) {
				return ResultCode::InvalidHeader;
			}
		}

		if (formatChunkFound && dataChunkFound) {
			return ResultCode::Success;
		}

		return ResultCode::InvalidHeader;
	}
}

