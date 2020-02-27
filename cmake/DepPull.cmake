cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if(DEFINED _DEPPULL_SOURCES_DEFAULT)
	return()
endif()

set(_DEPPULL_SOURCES_DEFAULT "${CMAKE_BINARY_DIR}/DepPull")

if(NOT DEFINED DEPPULL_SOURCES AND NOT DEFINED CACHE{DEPPULL_SOURCES})
	if(DEFINED ENV{DEPPULL_SOURCES})
		set(DEPPULL_SOURCES "$ENV{DEPPULL_SOURCES}")
	else()
		set(DEPPULL_SOURCES "${_DEPPULL_SOURCES_DEFAULT}")
	endif()
endif()

string(TOUPPER "${DEPPULL_SOURCES}" _DEPPULL_SOURCES_TOUPPER)

if("${_DEPPULL_SOURCES_TOUPPER}" STREQUAL "IN_HOME")
	if(WIN32)
		set(DEPPULL_SOURCES "$ENV{USERPROFILE}/.DepPull")
	elseif(UNIX)
		set(DEPPULL_SOURCES "$ENV{HOME}/.DepPull")
	endif()
elseif("${_DEPPULL_SOURCES_TOUPPER}" STREQUAL "IN_TEMP")
	if(WIN32)
		set(DEPPULL_SOURCES "$ENV{TEMP}/.DepPull")
	elseif(UNIX)
		set(DEPPULL_SOURCES "/tmp/.DepPull")
	endif()
endif()

string(REPLACE "\\" "/" DEPPULL_SOURCES "${DEPPULL_SOURCES}")

message(STATUS "DepPull: sources in ${DEPPULL_SOURCES}")

if("${DEPPULL_SOURCES}" STREQUAL "${_DEPPULL_SOURCES_DEFAULT}")
	message(STATUS "         You can set \$[CACHE|ENV]{DEPPULL_SOURCES} to change this path,")
	message(STATUS "         if the value is <IN_HOME|IN_TEMP>, will choose the appropriate directory.")
endif()

function(_DepPull_MakeSrcFullPaths A_DEP_RELATIVE_DIR A_LIST_RELATIVE_DIR)
	set(SOURCE_RELATIVE_DIR "${A_DEP_RELATIVE_DIR}/src")
	set(SOURCE_RELATIVE_DIR "${SOURCE_RELATIVE_DIR}" PARENT_SCOPE)

	set(SOURCE_DIR "${DEPPULL_SOURCES}/${SOURCE_RELATIVE_DIR}")
	set(SOURCE_DIR "${SOURCE_DIR}" PARENT_SCOPE)

	if(
		A_LIST_RELATIVE_DIR AND
		NOT "${A_LIST_RELATIVE_DIR}" STREQUAL "/" AND
		NOT "${A_LIST_RELATIVE_DIR}" STREQUAL "\\"
	)
		string(REPLACE "\\" "/" LIST_RELATIVE_DIR "${A_LIST_RELATIVE_DIR}")
		set(SOURCE_WITH_LIST_RELATIVE_DIR "${SOURCE_RELATIVE_DIR}/${LIST_RELATIVE_DIR}" PARENT_SCOPE)
		set(LIST_DIR "${SOURCE_DIR}/${LIST_RELATIVE_DIR}" PARENT_SCOPE)
	else()
		set(SOURCE_WITH_LIST_RELATIVE_DIR "${SOURCE_RELATIVE_DIR}" PARENT_SCOPE)
		set(LIST_DIR "${SOURCE_DIR}" PARENT_SCOPE)
	endif()
endfunction()

function(DepPull A_NAME)
	set(oneValueArgs
		ARCHIVE_URL
		ARCHIVE_HASH
		GIT_REPO
		GIT_TAG
		LIST_RELATIVE_DIR
		ANY_VERSION
	)
	cmake_parse_arguments(A "" "${oneValueArgs}" "" ${ARGN})

	if(A_ANY_VERSION)
		find_package(${A_NAME} QUIET)
		if(${A_NAME}_FOUND)
			message(STATUS "DepPull: found ${A_NAME} ${${A_NAME}_VERSION}")
			return()
		endif()
	endif()

	set(FOUND FALSE)
	string(TOLOWER ${A_NAME} NAME_TOLOWER)

	if(ARCHIVE_URL)
		string(REPLACE "=" "-" DEP_RELATIVE_DIR "${A_ARCHIVE_HASH}")
		set(DEP_RELATIVE_DIR "${NAME_TOLOWER}/${DEP_RELATIVE_DIR}")

		_DepPull_MakeSrcFullPaths("${DEP_RELATIVE_DIR}" "${A_LIST_RELATIVE_DIR}")

		if(EXISTS "${SOURCE_DIR}.ready")
			set(FOUND TRUE)
		else()
			message(STATUS "DepPull: getting '${A_NAME}' (${A_ARCHIVE_URL})")

			if(EXISTS "${SOURCE_DIR}")
				file(REMOVE_RECURSE "${SOURCE_DIR}")
			endif()
			file(MAKE_DIRECTORY "${SOURCE_DIR}")

			set(SOURCE_PKG "${SOURCE_DIR}.unkpkg")

			if(EXPECTED_HASH)
				file(DOWNLOAD "${ARCHIVE_URL}" "${SOURCE_PKG}" EXPECTED_HASH ${A_ARCHIVE_HASH})
			else()
				file(DOWNLOAD "${ARCHIVE_URL}" "${SOURCE_PKG}")
			endif()

			execute_process(
				COMMAND "${CMAKE_COMMAND}" -E tar xzf "${SOURCE_PKG}"
				RESULT_VARIABLE EXIT_CODE
				WORKING_DIRECTORY "${SOURCE_DIR}"
				ENCODING NONE
			)
			if(EXIT_CODE STREQUAL "0")
				file(WRITE "${SOURCE_DIR}.ready" "")
				set(FOUND TRUE)
			endif()
			file(REMOVE "${SOURCE_PKG}")
		endif()
	endif()

	if(NOT FOUND AND A_GIT_REPO)
		if(A_GIT_REPO AND (NOT A_GIT_TAG OR "${A_GIT_TAG}" STREQUAL "*"))
			set(A_GIT_TAG "master")
		endif()

		if("${A_GIT_TAG}" STREQUAL "master")
			string(REGEX REPLACE "\\.git$" "" GIT_REPO_CLEAR "${A_GIT_REPO}")
			string(SHA1 GIT_REPO_HASH "${GIT_REPO_CLEAR}")
			set(DEP_RELATIVE_DIR "${NAME_TOLOWER}/git-${GIT_REPO_HASH}")
		else()
			set(DEP_RELATIVE_DIR "${NAME_TOLOWER}/git-${A_GIT_TAG}")
		endif()

		_DepPull_MakeSrcFullPaths("${DEP_RELATIVE_DIR}" "${A_LIST_RELATIVE_DIR}")

		if(NOT EXISTS "${SOURCE_DIR}.ready")

			if(NOT DEFINED GIT_EXECUTABLE)
				find_package(Git REQUIRED)
			endif()

			message(STATUS "DepPull: cloning ${A_GIT_REPO}")

			if(NOT EXISTS "${DEPPULL_SOURCES}")
				file(MAKE_DIRECTORY "${DEPPULL_SOURCES}")
			endif()

			execute_process(
				COMMAND "${GIT_EXECUTABLE}" clone "${A_GIT_REPO}" "${SOURCE_RELATIVE_DIR}"
				RESULT_VARIABLE EXIT_CODE
				WORKING_DIRECTORY "${DEPPULL_SOURCES}"
				ENCODING NONE
			)
			if(EXIT_CODE STREQUAL "0")
				if(NOT "${A_GIT_TAG}" STREQUAL "master")
					execute_process(
						COMMAND "${GIT_EXECUTABLE}" checkout "${A_GIT_TAG}"
						RESULT_VARIABLE EXIT_CODE
						WORKING_DIRECTORY "${SOURCE_DIR}"
						ENCODING NONE
					)
					if(EXIT_CODE STREQUAL "0")
						message(FATAL_ERROR "DepPull: git error code ${EXIT_CODE}")
					endif()

					if(NOT EXISTS "${SOURCE_DIR}/.gitmodules")
						file(REMOVE_RECURSE "${SOURCE_DIR}/.git")
					endif()
				endif()

				file(WRITE "${SOURCE_DIR}.ready" "")
				set(FOUND TRUE)
			endif()

		elseif("${A_GIT_TAG}" STREQUAL "master")

			if(NOT DEFINED GIT_EXECUTABLE)
				find_package(Git REQUIRED QUIET)
			endif()

			if(DEFINED GIT_EXECUTABLE)
				message(STATUS "DepPull: pulling ${SOURCE_RELATIVE_DIR}")

				execute_process(
					COMMAND "${GIT_EXECUTABLE}" pull
					RESULT_VARIABLE EXIT_CODE
					WORKING_DIRECTORY "${SOURCE_DIR}"
					ENCODING NONE
				)
				if(EXIT_CODE STREQUAL "0")
					file(WRITE "${SOURCE_DIR}.ready" "")
					set(FOUND TRUE)
				endif()
			else()
				set(FOUND TRUE)
			endif()

		else()
			set(FOUND TRUE)
		endif()
	endif()

	if(NOT FOUND)
		message(FATAL_ERROR "DepPull: not found '${A_NAME}'")
	endif()

	message(STATUS "DepPull: using ${SOURCE_WITH_LIST_RELATIVE_DIR}")

	set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/DepPull/${DEP_RELATIVE_DIR}/build")
	if(EXISTS "${BUILD_DIR}" AND "${SOURCE_DIR}.ready" IS_NEWER_THAN "${BUILD_DIR}")
		file(REMOVE_RECURSE "${BUILD_DIR}")
	endif()

	if(EXISTS "${LIST_DIR}/CMakeLists.txt")
		add_subdirectory("${LIST_DIR}" "${BUILD_DIR}")
	endif()

	set(DEPPULL_${A_NAME}_SOURCE_DIR "${SOURCE_DIR}" PARENT_SCOPE)
	set(DEPPULL_${A_NAME}_LIST_DIR "${LIST_DIR}" PARENT_SCOPE)
	set(DEPPULL_${A_NAME}_BUILD_DIR "${BUILD_DIR}" PARENT_SCOPE)
endfunction()
