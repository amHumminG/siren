#pragma once

namespace siren::utils {

	// Helper to log errors from header-only classes
	void logError(const char* msg);

	// Helper to log info from header-only classes
	void logInfo(const char* msg);
}