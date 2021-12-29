
local action = _ACTION or ""

solution "cpa"
	location ( "Build" )
	configurations { "Debug", "Release" }
	platforms {"native", "x64", "x32"}
  
	configuration "Debug"
		defines { "DEBUG" }
		flags { "Symbols", "ExtraWarnings"}

	configuration "Release"
		defines { "NDEBUG" }
		flags { "Optimize", "ExtraWarnings"}    

	project "nanovg"
		language "C"
		kind "StaticLib"
		includedirs { "nanovg" }
		files { "nanovg/*.c" }
		targetdir("build")
		defines { "_CRT_SECURE_NO_WARNINGS" } --,"FONS_USE_FREETYPE" } Uncomment to compile with FreeType support

		configuration "Debug"
			defines { "DEBUG" }
			flags { "Symbols", "ExtraWarnings"}

		configuration "Release"
			defines { "NDEBUG" }
			flags { "Optimize", "ExtraWarnings"}

	project "test"
		kind "ConsoleApp"
		language "C++"
		flags { "Cpp17" }
		links { "nanovg" }
		files { "*.cpp", "*.c" }
		includedirs { "nanovg" }
		targetdir("Build")
	 
		configuration { "linux" }
			 linkoptions { "`pkg-config --libs glfw3`" }
			 links { "GL", "GLU", "m", "GLEW" }
			 defines { "NANOVG_GLEW" }

		configuration { "windows" }
			 links { "glfw3", "gdi32", "winmm", "user32", "GLEW", "glu32","opengl32" }
			 defines { "NANOVG_GLEW" }

		configuration { "macosx" }
			links { "glfw" }
			defines { "GL_SILENCE_DEPRECATION" }
			linkoptions { "-framework OpenGL", "-framework Cocoa", "-framework IOKit", "-framework CoreVideo" }
