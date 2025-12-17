#include "siren/DecoderFactory.h"
#include <array>

// Data sources
#include "siren/io/FileDataSource.h"
#include "siren/io/MemoryDataSource.h"

// Codecs
#include "siren/codecs/WavDecoder.h"

namespace siren {

	Result<std::unique_ptr<Decoder>> DecoderFactory::createDecoder(const std::string& fileName) {
		auto source = std::make_unique<FileDataSource>(fileName);
		if (!source->isValid()) {
			return ResultCode::InvalidFile;
		}
		return createDecoder(std::move(source));
	}

	Result<std::unique_ptr<Decoder>> DecoderFactory::createDecoder(std::span<std::byte> fileInMemory) {
		auto source = std::make_unique<MemoryDataSource>(fileInMemory);
		if (!source->isValid()) {
			return ResultCode::InvalidData;
		}
		return createDecoder(std::move(source));
	}

	Result<std::unique_ptr<Decoder>> DecoderFactory::createDecoder(std::unique_ptr<DataSource> source) {
		// Read first four bytes of the data source
		ResultCode result = source->seek(0);
		if (result != ResultCode::Success) {
			return result;
		}

		std::array<std::byte, 4> buffer;
		if (source->read(buffer) < 4) {
			return ResultCode::InvalidData;
		}

		// Reset cursor for decoder
		result = source->seek(0);
		if (result != ResultCode::Success) {
			return result;
		}

		std::unique_ptr<Decoder> decoder;
		
		if (!memcmp(buffer.data(), "RIFF", 4)) {
			// WAV file -> Create WAW decoder
			decoder = std::make_unique<WavDecoder>();
		}
		else {
			return ResultCode::FormatNotSupported;
		}

		result = decoder->prime(std::move(source));
		if (result != ResultCode::Success) {
			return result;
		}

		return std::move(decoder);
	}
}