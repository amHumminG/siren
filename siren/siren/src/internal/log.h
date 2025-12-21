#pragma once
#include <iostream>

// Debug mode (_DEBUG is by Visual Studio if Config = Debug)
#if defined(_DEBUG) || defined (DEBUG)
	
	// Macro prints
	#define SIREN_LOG_ERROR(msg) \
		std::cerr << "[SIREN ERROR] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;

#define SIREN_LOG_WARNING(msg) \
		std::cerr << "[SIREN WARNING] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;

	#define SIREN_LOG_INFO(msg) \
		std::cout << "[SIREN INFO] " << msg << std::endl;

#else
	// Release mode
	#define SIREN_LOG_ERROR(msg) ((void)0)	
	#define SIREN_LOG_WARNING(msg) ((void)0)
	#define SIREN_LOG_INFO(msg) ((void)0)	

#endif