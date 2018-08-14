#ifndef _RUA_RASSERT_HPP
#define _RUA_RASSERT_HPP

#include <iostream>
#include <sstream>

#ifdef _WIN32
	#include <windows.h>

	#define RUA_RASSERT(_exp) { \
		if (!(_exp)) { \
			std::ostringstream ss; \
			ss << "File: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl <<  "Expression: " << #_exp; \
			std::cout << std::endl << "Assertion failed!" << std::endl << std::endl << ss.str() << std::endl; \
			MessageBoxA(0, ss.str().c_str(), "Assertion failed!", MB_OK | MB_ICONERROR); \
			exit(EXIT_FAILURE); \
		} \
	}
#else
	#define RUA_RASSERT(_exp) { \
		if (!(_exp)) { \
			std::ostringstream ss; \
			ss << "File: " << __FILE__ << ":" << __LINE__ << std::endl << std::endl <<  "Expression: " << #_exp; \
			std::cout << std::endl << "Assertion failed!" << std::endl << std::endl << ss.str() << std::endl; \
			exit(EXIT_FAILURE); \
		} \
	}
#endif

#ifdef _WIN32
	#include <windows.h>

	#define RUA_RASSERT_EX(_exp, _part, _culprit) { \
		if (!(_exp)) { \
			std::ostringstream ss; \
			ss << "File: [" << _part << "] " << __FILE__ << ":" << __LINE__ << std::endl << std::endl << "Expression: " << #_exp << std::endl << std::endl << "Culprit: " << _culprit; \
			std::cout << std::endl << "Assertion failed!" << std::endl << std::endl << ss.str() << std::endl; \
			MessageBoxA(0, ss.str().c_str(), "Assertion failed!", MB_OK | MB_ICONERROR); \
			exit(EXIT_FAILURE); \
		} \
	}
#else
	#define RUA_RASSERT_EX(_exp, _part, _culprit) { \
		if (!(_exp)) { \
			std::ostringstream ss; \
			ss << "File: [" << _part << "] " << __FILE__ << ":" << __LINE__ << std::endl << std::endl << "Expression: " << #_exp << std::endl << std::endl << "Culprit: " << _culprit; \
			std::cout << std::endl << "Assertion failed!" << std::endl << std::endl << ss.str() << std::endl; \
			exit(EXIT_FAILURE); \
		} \
	}
#endif

#endif
