workspace "IniReader"
   configurations { "Release", "Debug" }
   platforms { "Win32", "Win64" }
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
   
   kind "ConsoleApp"
   language "C++"
   characterset ("Unicode")
   staticruntime "On"
   targetdir ("tests")
   
   includedirs { "mINI/src/mini/" }
   includedirs { "mINI/tests/lest" }
   includedirs { "source" }
   
   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "On"

project "IniReader"
   files { "IniReader.h" }
   files { "IniReader.cpp" }
project "testcasesens"
   files { "mINI/tests/testcasesens.cpp" }
project "testcopy"
   files { "mINI/tests/testcopy.cpp" }
project "testgenerate"
   files { "mINI/tests/testgenerate.cpp" }
project "testhuge"
   files { "mINI/tests/testhuge.cpp" }
project "testread"
   files { "mINI/tests/testread.cpp" }
project "testutf8"
   files { "mINI/tests/testutf8.cpp" }
project "testwrite"
   files { "mINI/tests/testwrite.cpp" }