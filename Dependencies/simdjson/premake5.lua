project "simdjson"
	kind "StaticLib"
	language "C++"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"simdjson.h",
		"simdjson.cpp"
	}

	filter "configurations:Debug"
		symbols "on"

	filter "configurations:Release"
		optimize "on"
