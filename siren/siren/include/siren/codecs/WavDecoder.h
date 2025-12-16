#pragma once
#include "siren/Decoder.h"

#pragma pack(push, 1) // Disable padding

struct SignatureChunk {
	char riff[4];		// "RIFF"
	int32_t fileSize;	// Total size of the audio file
	char wave[4];		// "WAVE"
};

struct Chunk {
	char identifier[4];		// Chunk identifier "data" or "fmt "
	uint32_t chunkSize;		// Size of chunk
};

struct FormatChunk {
	int16_t formatType;		// Type of format (1 is PCM)
	int16_t channelCount;	// Number of Channels
	int32_t sampleRate;		// Sample Rate (Number of Samples per second, or Hertz)
	int32_t byteRate;		// (Sample Rate * BitsPerSample * Channels) / 8
	int16_t blockAlign;		// (BitsPerSample * Channels) / 8
	int16_t bitsPerSample;	// Bits per sample
};

#pragma pack(pop) // Enable padding

namespace siren {

	class WavDecoder : public Decoder {
	private:
		uint16_t m_bitsPerSample = 0;
		uint16_t m_blockAlign = 0;
		size_t m_dataStartOffset = 0;

		ResultCode decodeHeader();

	public:
		WavDecoder() = default;

		[[nodiscard]] size_t decode(std::span<float> dst) override;
		[[nodiscard]] ResultCode seek(size_t frameIndex) override;
		[[nodiscard]] size_t tell() const override;

		[[nodiscard]] uint32_t getSampleRate() const override;
		[[nodiscard]] uint16_t getChannelCount() const override;
		[[nodiscard]] size_t getTotalFrames() const override;
	};
}