configurations
{
	"Debug",
	"Release",
	"Dist"
}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"]          = "%{wks.location}/Dependencies/GLFW/include"
IncludeDir["GLM"]           = "%{wks.location}/Dependencies/GLM/"
IncludeDir["stb_image"]     = "%{wks.location}/Dependencies/stb_image/include"
IncludeDir["ImGui"]         = "%{wks.location}/Dependencies/ImGui"
IncludeDir["fastgltf"]      = "%{wks.location}/Dependencies/fastgltf/include"
IncludeDir["simdjson"]      = "%{wks.location}/Dependencies/simdjson"
IncludeDir["VulkanSDK"]     = "%{VULKAN_SDK}/Include"
IncludeDir["Optick"]        = "%{wks.location}/Dependencies/Optick/src"
IncludeDir["slang"]         = "%{wks.location}/Dependencies/slang/include"

LibraryDir = {}
LibraryDir["slang"]     = "%{wks.location}/Dependencies/slang/lib"
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"
LibraryDir["Optick"]    = "%{wks.location}/Dependencies/Optick/bin/vs2022/x64/Release"

Library = {}
Library["slang"]  = "%{LibraryDir.slang}/slang.lib"
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["Optick"] = "%{LibraryDir.Optick}/OptickCore.lib"

workspace "Cobalt"
	architecture "x64"
	startproject "CobaltApp"
	language "C++"
	cppdialect "C++latest"
	staticruntime "On"

	flags { "MultiProcessorCompile" }

	targetdir ("%{wks.location}/bin/"     .. outputdir .. "/%{prj.name}")
	objdir    ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	debugdir "%{wks.location}"

	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		defines "CO_DEBUG"
		symbols "On"
		runtime "Debug"

	filter "configurations:Release"
		defines "CO_RELEASE"
		optimize "On"
		runtime "Release"

	filter "configurations:Dist"
		defines "CO_DIST"
		optimize "On"
		runtime "Release"

    filter { "system:linux" }
        buildoptions { "-Wno-return-type" }

group "Dependencies"
    include "Dependencies/GLFW"
    include "Dependencies/ImGui"
    include "Dependencies/stb_image"
	include "Dependencies/simdjson"
	include "Dependencies/fastgltf"
group ""

include "CobaltApp"