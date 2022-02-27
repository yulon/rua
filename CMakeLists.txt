cmake_minimum_required(VERSION 3.0)

project(rua)

add_library(rua INTERFACE)
set(rua_INCLUDE_DIR "include")
target_include_directories(rua INTERFACE ${rua_INCLUDE_DIR})
if(UNIX)
	target_link_libraries(rua INTERFACE pthread)
endif()

if(DEFINED RUA_TEST)
	include("cmake/depu.cmake")

	depu(
		doctest
		USE_PACKAGE TRUE
		GIT_REPO "https://github.com/onqtam/doctest.git"
	)

	set(rua_test_SOURCE_DIR "test")
	if(${RUA_TEST} STREQUAL "all")
		file(GLOB_RECURSE rua_test_SOURCES "${rua_test_SOURCE_DIR}/*.cpp")
	else()
		file(GLOB_RECURSE rua_test_SOURCES "${rua_test_SOURCE_DIR}/main.cpp" "${rua_test_SOURCE_DIR}/${RUA_TEST}.cpp")
	endif()

	add_executable(rua_test_11 ${rua_test_SOURCES})
	set_target_properties(rua_test_11 PROPERTIES CXX_STANDARD 11)
	target_link_libraries(rua_test_11 rua doctest::doctest)

	add_executable(rua_test_14 ${rua_test_SOURCES})
	set_target_properties(rua_test_14 PROPERTIES CXX_STANDARD 14)
	target_link_libraries(rua_test_14 rua doctest::doctest)

	add_executable(rua_test_17 ${rua_test_SOURCES})
	set_target_properties(rua_test_17 PROPERTIES CXX_STANDARD 17)
	target_link_libraries(rua_test_17 rua doctest::doctest)
endif()
