workspace "nob"
	location ( "build" )
	objdir("build/obj")

	dofileopt "include.pm5"

	platforms { "native" }
	configurations { "Debug", "Release" }

	configuration "Debug"
		targetsuffix "_d"
		defines { "DEBUG" }
		symbols "On"
		warnings "Extra"

	configuration "Release"
		defines { "NDEBUG" }
		optimize "Speed"

	configuration { "not StaticLib" }
		targetdir("bin")

	configuration { "StaticLib" }
		targetdir("lib")

	configuration { "windows", "gmake*", "StaticLib" }
		targetprefix "lib"
		targetextension ".a"

	project "rua_tests"
		language "C++"
		cppdialect "C++11"
		kind "ConsoleApp"
		files {
			"test/*.cpp"
		}
		links { "gtest" }
		configuration { "linux" }
			links { "pthread" }
