#include "siren/io/FileDataSource.h"

namespace siren {

	FileDataSource::FileDataSource(const std::string& path) {
		m_file.open(path, std::ios::binary | std::ios::ate); // Open file at the end

		if (m_file.is_open()) {
			m_fileSize = static_cast<size_t>(m_file.tellg());
			m_file.seekg(0, std::ios::beg); // Reset cursor to start
		}
	}

	bool FileDataSource::isValid() const {
		return m_file.is_open() && m_fileSize > 0;
	}

	size_t FileDataSource::read(std::span<std::byte> dst) {
		if (!m_file.is_open()) {
			return 0;
		}

		m_file.read(reinterpret_cast<char*>(dst.data()), dst.size());

		return static_cast<size_t>(m_file.gcount());
	}

	ResultCode FileDataSource::seek(size_t byteOffset) {
		if (!m_file.is_open()) {
			return ResultCode::InvalidFile;
		}

		if (byteOffset > m_fileSize) {
			return ResultCode::OutOfBounds;
		}

		m_file.clear();
		m_file.seekg(byteOffset, std::ios::beg);

		if (m_file.fail()) {
			m_file.clear(); // Makes sure that file can still be read from
			return ResultCode::OutOfBounds;
		}

		return ResultCode::Success;
	}

	size_t FileDataSource::tell() const {
		if (!m_file.is_open()) {
			return 0;
		}

		return static_cast<size_t>(m_file.tellg());
	}

	size_t FileDataSource::size() const {
		return m_fileSize;
	}
}


