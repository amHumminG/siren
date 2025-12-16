#include "siren/siren.h"
#include "siren/Result.h"

#include <iostream>
#include <vector>
#include <span>
#include <thread>

// Minaudio
#define MINIAUDIO_IMPLEMENTATION
#include "../external/miniaudio/miniaudio.h"

// Quick Testing
#include "siren/io/FileDataSource.h"
#include "siren/io/MemoryDataSource.h"
#include "siren/codecs/WavDecoder.h"

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
	// Get decoder stored in pUserData
	auto* decoder = static_cast<siren::WavDecoder*>(pDevice->pUserData);
	if (!decoder) {
		return;
	}

	// Prep destination buffer
	float* outputBuffer = static_cast<float*>(pOutput);

	size_t channelCount = decoder->getChannelCount();
	size_t totalSamplesNeeded = frameCount * channelCount;

	// Create buffer for decoder to fill
	std::span<float> buffer(outputBuffer, totalSamplesNeeded);

	size_t framesDecoded = decoder->decode(buffer);

	// Handle EOF (Audio clip ended)
	if (framesDecoded < frameCount) {
		size_t samplesDecoded = framesDecoded * channelCount;
		size_t samplesRemaining = totalSamplesNeeded - samplesDecoded;

		// Set remaining samples to 0.0f (silenece)
		std::fill_n(outputBuffer + samplesDecoded, samplesRemaining, 0.0f);
	}
}

using namespace siren;

int main() {
	siren::HelloWorld();
	std::cout << std::endl;

	// FileDataSource
	if (false) {
		std::cout << "-- TESTING [FileDataSource] --" << std::endl;

		siren::FileDataSource fileTest("assets/test.txt");

		std::cout << "Is valid: " << fileTest.isValid() << std::endl;
		std::cout << "Size: " << fileTest.size() << std::endl;
		std::cout << "Cursor pos: " << fileTest.tell() << std::endl;
		if (fileTest.seek(2) != siren::ResultCode::Success) {
			std::cout << "Failed seek" << std::endl;
		}
		std::cout << "Cursor pos: " << fileTest.tell() << std::endl;
		std::vector<std::byte> fileBuffer(fileTest.size());
		std::cout << "Read " << fileTest.read(fileBuffer) << " bytes into the buffer" << std::endl;
		std::cout << "Ruffer contents: ";
		std::cout.write(reinterpret_cast<const char*>(fileBuffer.data()), fileBuffer.size());
		std::cout << std::endl << std::endl;
	}

	// MemoryDataSource
	if (false) {
		std::cout << "-- TESTING [MemoryDataSource] --" << std::endl;

		std::string text = "Testing";
		// siren::MemoryDataSource memoryTest(text.data(), text.size());
		siren::MemoryDataSource memoryTest(std::span(reinterpret_cast<std::byte*>(text.data()), text.size()));

		std::cout << "Size: " << memoryTest.size() << std::endl;
		std::cout << "Cursor pos: " << memoryTest.tell() << std::endl;
		if (memoryTest.seek(2) != siren::ResultCode::Success) {
			std::cout << "Failed seek" << std::endl;
		}
		std::cout << "Cursor pos: " << memoryTest.tell() << std::endl;
		std::vector<std::byte> memoryBuffer(memoryTest.size());
		std::cout << "Read " << memoryTest.read(memoryBuffer) << " bytes into the buffer" << std::endl;
		std::cout << "Buffer: ";
		std::cout.write(reinterpret_cast<const char*>(memoryBuffer.data()), memoryBuffer.size());
		std::cout << std::endl << std::endl;
	}

	// Audio playtest
	if (true) {
		// Engine setup
		const char* filename = "assets/white_ferrari.wav";
		auto source = std::make_unique<FileDataSource>(filename);
		auto decoder = std::make_unique<WavDecoder>();

		std::cout << "Priming decoder for: " << filename << std::endl;
		if (decoder->prime(std::move(source)) != ResultCode::Success) {
			std::cout << "Decoder priming failed" << std::endl;
			return -1;
		}
		
		std::cout << "Decoder priming finished" << std::endl;
		std::cout << "Channel count: " << decoder->getChannelCount() << " Hz" << std::endl;
		std::cout << "Sample rate: " << decoder->getSampleRate() << std::endl;

		// Miniaudio setup
		ma_device_config config = ma_device_config_init(ma_device_type_playback);

		// Matching config to WAV format
		config.playback.format = ma_format_f32;
		config.playback.channels = decoder->getChannelCount();
		config.sampleRate = decoder->getSampleRate();

		config.dataCallback = data_callback;
		config.pUserData = decoder.get();

		ma_device device;
		if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
			std::cout << "Failed to init audio device" << std::endl;
			return -2;
		}

		if (ma_device_start(&device) != MA_SUCCESS) {
			std::cout << "Failed to start playback" << std::endl;
			ma_device_uninit(&device);
			return -3;
		}

		std::cout << "Playing... [Enter] to exit" << std::endl;
		getchar();

		ma_device_uninit(&device);
	}

	return 0;
}