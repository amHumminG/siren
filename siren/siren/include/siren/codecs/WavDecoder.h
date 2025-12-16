#pragma once
#include "siren/Decoder.h"

#pragma pack(push, 1) // Disable padding

struct SignatureChunk {
	char riff[4];		// "RIFF"
	uint32_t fileSize;	// Total size of the audio file
	char wave[4];		// "WAVE"
};

struct Chunk {
	char identifier[4];		// Chunk identifier "data" or "fmt "
	uint32_t size;			// Size of chunk
};

struct FormatChunk {
	uint16_t formatType;		// Type of format (1 is PCM)
	uint16_t channelCount;		// Number of Channels
	uint32_t sampleRate;		// Sample Rate (Number of Samples per second, or Hertz)
	uint32_t byteRate;			// (Sample Rate * BitsPerSample * Channels) / 8
	uint16_t blockAlign;		// (BitsPerSample * Channels) / 8
	uint16_t bitsPerSample;		// Bits per sample
};

#pragma pack(pop) // Enable padding

constexpr uint16_t WAVE_FORMAT_PCM = 0x001;
constexpr uint16_t WAVE_FORMAT_IEEE_FLOAT = 0x0003;
constexpr uint16_t WAVE_FORMAT_EXTENSIBLE = 0xFFFE;

namespace siren {

	class WavDecoder : public Decoder {
	private:
		uint16_t m_bitsPerSample = 0;
		uint16_t m_blockAlign = 0;	// Bytes per frame
		size_t m_dataStartOffset = 0;

	protected:
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