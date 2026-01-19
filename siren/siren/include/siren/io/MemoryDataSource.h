#pragma once
#include "siren/DataSource.h"
#include <cstring>
#include <algorithm>
#include <memory>
#include <vector>

namespace siren {
	 
	class MemoryDataSource : public DataSource {
	private:
		std::shared_ptr<std::vector<std::byte>> m_ownershipAnchor;

		const std::byte* m_data = nullptr;
		size_t m_size = 0;
		size_t m_cursor = 0;

	public:

		explicit MemoryDataSource(std::shared_ptr<std::vector<std::byte>> internalData);

		/// @brief Creates a data source from a buffer in memory
		explicit MemoryDataSource(std::span<const std::byte> buffer);

		/// @brief Creates a data source form a buffer in memory (legacy pointer)
		/// @param ptr Pointer to the buffer
		/// @param size Size of the buffer
		MemoryDataSource(const void* ptr, size_t size);

		/// @brief Verifies data source validity
		/// @return True if data is not nullptr, otherwise false
		[[nodiscard]] bool isValid() const override;

		[[nodiscard]] size_t read(std::span<std::byte> dst) override;
		[[nodiscard]] ResultCode seek(size_t byteOffset) override;
		[[nodiscard]] size_t tell() const override;
		[[nodiscard]] size_t size() const override;
	};
}