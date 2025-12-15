#pragma once
#include <variant>
#include <cassert>
#include "siren/Utils.h"

namespace siren {

	enum class ResultCode {
		Success = 0,
		OutOfBounds,
		InvalidFile,
		InvalidData,
		InvalidHeader,
		// more to come
	};

	template<typename T>
	class Result {
	public:
		std::variant<T, ResultCode> m_data; // Holds either a return value or an error code

		// Success constructor
		Result(T value) : m_data(std::move(value)) {}

		// Error constructor
		Result(ResultCode error) : m_data(error) {}

		// Status checks
		bool isOk() const { return std::holds_alternative<T>(m_data); }
		bool isErr() const { return std::holds_alternative<ResultCode>(m_data); }

		explicit operator bool() const {
			return isOk();
		}

		// Get value
		const T& value() const {
			if (!isOk()) {
				siren::utils::logError("Tried to access value of an error result");
				assert(false);
			}
			return std::get<T>(m_data);
		}

		ResultCode error() const {
			if (isOk()) {
				return ResultCode::Success;
			}
			return std::get<ResultCode>(m_data);
		}
	};
}