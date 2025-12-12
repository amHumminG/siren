#pragma once
#include <iostream>

// Debug mode (_DEBUG is by Visual Studio if Config = Debug)
#if defined(_DEBUG) || defined (DEBUG)
	
	// Macro prints
	#define SIREN_LOG_ERROR(msg) \
		std::cerr << "[Siren Error] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl;

	#define SIREN_LOG_INFO(msg) \
		std::cout << "[Siren Info] " << msg << std::endl;

#else
	// Release mode
	#define SIREN_LOG_ERROR(msg) ((void)0)	
	#define SIREN_LOG_INFO(msg) ((void)0)	

#endif