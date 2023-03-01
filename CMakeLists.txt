cmake_minimum_required(VERSION 3.0)

project(rua)

add_library(rua INTERFACE)
set(rua_INCLUDE_DIR "include")
target_include_directories(rua INTERFACE ${rua_INCLUDE_DIR})
if(UNIX)
	target_link_libraries(rua INTERFACE pthread)
endif()

if ("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
	include("cmake/depu.cmake")

	depu(
		doctest
		USE_PACKAGE TRUE
		GIT_REPO "https://github.com/onqtam/doctest.git"
	)

	file(GLOB_RECURSE rua_test_SOURCES "test/*.cpp")

	add_executable(rua_test_11 ${rua_test_SOURCES})
	set_target_properties(rua_test_11 PROPERTIES CXX_STANDARD 11)
	target_link_libraries(rua_test_11 rua doctest::doctest)

	add_executable(rua_test_14 ${rua_test_SOURCES})
	set_target_properties(rua_test_14 PROPERTIES CXX_STANDARD 14)
	target_link_libraries(rua_test_14 rua doctest::doctest)

	add_executable(rua_test_17 ${rua_test_SOURCES})
	set_target_properties(rua_test_17 PROPERTIES CXX_STANDARD 17)
	target_link_libraries(rua_test_17 rua doctest::doctest)

	add_executable(rua_test_20 ${rua_test_SOURCES})
	set_target_properties(rua_test_20 PROPERTIES CXX_STANDARD 20)
	target_link_libraries(rua_test_20 rua doctest::doctest)
endif()
