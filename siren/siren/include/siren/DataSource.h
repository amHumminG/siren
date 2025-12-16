#pragma once
#include <span>
#include <cstddef>
#include "siren/result.h"

namespace siren {

	class DataSource {
	public:
		virtual ~DataSource() = default;

		/// @brief Checks if data source has been created correctly
		/// @return True if all setups have been performed, otherwise false
		[[nodiscard]] virtual bool isValid() const = 0;

		/// @brief Reads up to N bytes, where N is the size of the destination buffer
		/// 
		/// Attempts to fill the buffer completely. If the source has fewer bytes
		/// remaining than the buffer size, only the remaining bytes are read
		/// 
		/// This function will also move the cursor along with the read data
		/// 
		/// @param dst Destination buffer
		/// @return Number of bytes read into the buffer
		[[nodiscard]] virtual size_t read(std::span<std::byte> dst) = 0;

		/// @brief Seeks to a specific byte offset from origin
		/// @param byteOffset Specified offset
		/// @return ResultCode::Success if byte offset was set, otherwise error code
		[[nodiscard]] virtual ResultCode seek(size_t byteOffset) = 0;

		/// @return The current byte position
		[[nodiscard]] virtual size_t tell() const = 0;

		/// @return The total size of the source in bytes
		[[nodiscard]] virtual size_t size() const = 0;
	};
}