workspace "TB-Utils"
	configurations { "Debug", "Release" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"
		optimize "Off"

	filter "configurations:Release"
		defines { "NDEBUG" }
		symbols "Off"
		optimize "Speed"

project "VMF-Utils"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir "../build"
	targetname "vmf-utils"
	files { "../vmf-utils/src/*.cpp", "../vmf-utils/src/*.hpp" }
	files { "../shared/include/*.hpp", "../shared/src/*.cpp" }
	includedirs { "../shared/include" }

project "FGD-Utils"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	targetdir "../build"
	targetname "fgd-utils"
	files { "../fgd-utils/src/*.cpp", "../fgd-utils/src/*.hpp" }
	files { "../shared/include/*.hpp", "../shared/src/*.cpp" }
	includedirs { "../shared/include" }
