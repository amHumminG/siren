#include "siren/Utils.h"

#include "internal/log.h"

namespace siren::utils {
	 
	void logError(const char* msg) {
		SIREN_LOG_ERROR(msg);
	}

	void logInfo(const char* msg) {
		SIREN_LOG_INFO(msg);
	}
}