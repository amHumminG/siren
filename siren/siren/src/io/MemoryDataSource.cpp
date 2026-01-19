#include "siren/io/MemoryDataSource.h"

namespace siren {
	MemoryDataSource::MemoryDataSource(std::shared_ptr<std::vector<std::byte>> internalData) 
		: m_ownershipAnchor(std::move(internalData)) {
		if (m_ownershipAnchor && !m_ownershipAnchor->empty()) {
			m_data = m_ownershipAnchor->data();
			m_size = m_ownershipAnchor->size();
		}
		else {
			m_ownershipAnchor = nullptr;
			m_data = nullptr;
			m_size = 0;
		}
		m_cursor = 0;
	}
	MemoryDataSource::MemoryDataSource(std::span<const std::byte> buffer)
		: m_data(buffer.data()), m_size(buffer.size()) {
		m_ownershipAnchor = nullptr;
	}

	MemoryDataSource::MemoryDataSource(const void* ptr, size_t size) 
		: m_data(static_cast<const std::byte*>(ptr)), m_size(size) {
		m_ownershipAnchor = nullptr;
	}

	bool MemoryDataSource::isValid() const {
		return m_data != nullptr && m_size > 0;
	}

	size_t MemoryDataSource::read(std::span<std::byte> dst) {
		if (m_cursor >= m_size) {
			return 0;
		}

		size_t numBytes = std::min<size_t>(dst.size(), m_size - m_cursor);
		memcpy(dst.data(), m_data + m_cursor, numBytes);

		m_cursor += numBytes;

		return numBytes;
	}

	ResultCode MemoryDataSource::seek(size_t byteOffset) {
		if (byteOffset > m_size) {
			return ResultCode::OutOfBounds;
		}

		m_cursor = byteOffset;
		return ResultCode::Success;
	}

	size_t MemoryDataSource::tell() const {
		return m_cursor;
	}

	size_t MemoryDataSource::size() const {
		return m_size;
	}


}