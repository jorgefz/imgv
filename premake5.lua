
workspace "imgv"
	architecture "x86"
	configurations {
		"Debug",
		"Release",
	}
	startproject "imgv"
	flags { "MultiProcessorCompile" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "imgv"
	--location "vsproject"
	kind "ConsoleApp"
	language "C"
	staticruntime "on"

	--targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	--objdir ("bin-int/" .. outputdir .. "/%{prj.name}") 

	targetdir ("bin/")
	objdir ("bin-int/")

	files {
		"src/**.h",
		"src/**.c"
	}

	defines{
		"GLEW_STATIC"
	}

	--defines 

	includedirs {
		"vendor/GLEW/include",
		"vendor/GLFW/include",
		"vendor/stb/include"
	}

	-- only for Windows
	libdirs {
		"vendor/GLFW/lib-vc2019",
		"vendor/GLEW/lib"
	}

	-- %%% External libraries %%%
	 links { 
	 	"glfw3",
	 	"glew32s",
	 	"opengl32"
	 }
	-- For GNU/Linux
	-- libdirs { os.findlib("GLFW") }

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"
		defines {
			"IMGV_PLATFORM_WINDOWS",
			"_WINDOWS"
		}

	filter "system:linux"
		staticruntime "On"
		systemversion "latest"
		defines {
			"IMGV_PLATFORM_LINUX"
		}

	filter "configurations:Debug"
		buildoptions "/MDd"
		defines "IMGV_DEBUG"
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		buildoptions "/MD"
		defines "IMGV_RELEASE"
		runtime "Release"
		optimize "On"

