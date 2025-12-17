#pragma once
#include "siren/Decoder.h"
#include "siren/Result.h"
#include <memory>
#include <string>

namespace siren {

	class DecoderFactory {
	public: 
		DecoderFactory() = delete;
		~DecoderFactory() = delete;

		/// @brief Determines the decoder needed to decode an audio file and creates it
		/// @param file File to be decoded
		/// @return The created decoder
		static Result<std::unique_ptr<Decoder>> CreateDecoder(const std::string& fileName);

		/// @brief Determines the decoder needed to decode an audio file stored in memory and creates it
		/// @param fileInMemory File in memory to be decoded
		/// @return The created decoder
		static Result<std::unique_ptr<Decoder>> CreateDecoder(std::span<std::byte> fileInMemory);

		/// @brief Determines the decoder needed to decode an audio source and creates it
		/// @param source The data source to be decoded
		/// @return The created decoder
		static Result<std::unique_ptr<Decoder>> CreateDecoder(std::unique_ptr<DataSource> source);
	};
}