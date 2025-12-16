#pragma once
#include <fstream>
#include <string>
#include "siren/DataSource.h"

namespace siren {

	class FileDataSource : public DataSource {
	private:
		mutable std::ifstream m_file;
		size_t m_fileSize = 0;

	public:
		/// @brief Opens a file for reading in binary mode
		/// @param path Path to the file
		explicit FileDataSource(const std::string& path);

		/// @brief Checks if file was successfully opened
		/// @return True if file is open, otherwise false
		[[nodiscard]] bool isValid() const;

		[[nodiscard]] size_t read(std::span<std::byte> dst) override;
		[[nodiscard]] ResultCode seek(size_t byteOffset) override;
		[[nodiscard]] size_t tell() const override;
		[[nodiscard]] size_t size() const override;
	};
}