local Root = path.getabsolute(_MAIN_SCRIPT_DIR)
local External = Root .. "/External"
local Assets = Root .. "/Assets"
local AssetManifest = Assets .. "/EmbeddedAssets.txt"
local GeneratedAssets = Root .. "/Intermediate/Generated/Assets"
local GeneratedAssetSource = GeneratedAssets .. "/EmbeddedAssets.cpp"
local GeneratedGlfw = Root .. "/Intermediate/Generated/GLFW"
local Action = _ACTION or "NoAction"
local WorkspaceLocation = Root .. "/Intermediate/ProjectFiles/" .. Action
local ProjectFiles = Root .. "/Intermediate/ProjectFiles/" .. Action
local IntermediateOutputDirectory = "%{cfg.system}/%{cfg.architecture}/%{cfg.buildcfg}"
local WindowsOutputDirectory = "Windows/%{cfg.architecture}/%{cfg.buildcfg}"
local LinuxOutputDirectory = "Linux/%{cfg.architecture}/%{cfg.buildcfg}"
local AssetInputs = os.matchfiles(Assets .. "/**")

if Action == "vs2022" or Action == "vs2026" then
    WorkspaceLocation = Root
end

table.insert(AssetInputs, AssetManifest)
table.insert(AssetInputs, Root .. "/Source/Assets/AssetPath.h")
for _, AssetBakerInput in ipairs(os.matchfiles(Root .. "/Tools/AssetBaker/**")) do
    table.insert(AssetInputs, AssetBakerInput)
end

local function ApplyToolchainSettings()
    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"
        defines {
            "NOMINMAX",
            "UNICODE",
            "_UNICODE",
            "WIN32_LEAN_AND_MEAN"
        }

    filter "system:linux"
        pic "On"

    filter "configurations:Debug"
        defines { "PROJECT_DEBUG=1" }
        runtime "Debug"
        symbols "On"

    filter "configurations:Development"
        defines { "PROJECT_DEVELOPMENT=1" }
        runtime "Release"
        symbols "On"
        optimize "Speed"

    filter "configurations:Shipping"
        defines { "PROJECT_SHIPPING=1" }
        runtime "Release"
        symbols "On"
        optimize "Full"

    filter {}
end

local function ApplyProjectDirectories()
    objdir(Root .. "/Intermediate/Build/" .. IntermediateOutputDirectory .. "/%{prj.name}")

    filter "system:windows"
        targetdir(Root .. "/Binaries/" .. WindowsOutputDirectory)

    filter "system:linux"
        targetdir(Root .. "/Binaries/" .. LinuxOutputDirectory)

    filter {}
end

workspace "ProjectTemplate"
    architecture "x86_64"
    configurations { "Debug", "Development", "Shipping" }
    location(WorkspaceLocation)
    startproject "StarterApp"

    language "C++"
    cppdialect "C++23"
    staticruntime "On"
    multiprocessorcompile "On"

    ApplyToolchainSettings()

project "GLFW"
    location(ProjectFiles)
    kind "StaticLib"
    language "C"
    cdialect "C99"
    warnings "Off"

    ApplyProjectDirectories()

    files {
        External .. "/glfw/include/GLFW/glfw3.h",
        External .. "/glfw/include/GLFW/glfw3native.h",
        External .. "/glfw/src/context.c",
        External .. "/glfw/src/egl_context.c",
        External .. "/glfw/src/init.c",
        External .. "/glfw/src/input.c",
        External .. "/glfw/src/internal.h",
        External .. "/glfw/src/mappings.h",
        External .. "/glfw/src/monitor.c",
        External .. "/glfw/src/null_init.c",
        External .. "/glfw/src/null_joystick.c",
        External .. "/glfw/src/null_monitor.c",
        External .. "/glfw/src/null_window.c",
        External .. "/glfw/src/osmesa_context.c",
        External .. "/glfw/src/platform.c",
        External .. "/glfw/src/platform.h",
        External .. "/glfw/src/vulkan.c",
        External .. "/glfw/src/window.c"
    }

    includedirs {
        External .. "/glfw/include",
        External .. "/glfw/src"
    }

    filter "system:windows"
        defines {
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }
        files {
            External .. "/glfw/src/wgl_context.c",
            External .. "/glfw/src/win32_init.c",
            External .. "/glfw/src/win32_joystick.c",
            External .. "/glfw/src/win32_module.c",
            External .. "/glfw/src/win32_monitor.c",
            External .. "/glfw/src/win32_thread.c",
            External .. "/glfw/src/win32_time.c",
            External .. "/glfw/src/win32_window.c"
        }

    filter "system:linux"
        defines {
            "_DEFAULT_SOURCE",
            "_GLFW_X11",
            "_GLFW_WAYLAND"
        }
        includedirs { GeneratedGlfw }
        files {
            GeneratedGlfw .. "/*.h",
            External .. "/glfw/src/glx_context.c",
            External .. "/glfw/src/linux_joystick.c",
            External .. "/glfw/src/posix_module.c",
            External .. "/glfw/src/posix_poll.c",
            External .. "/glfw/src/posix_thread.c",
            External .. "/glfw/src/posix_time.c",
            External .. "/glfw/src/wl_init.c",
            External .. "/glfw/src/wl_monitor.c",
            External .. "/glfw/src/wl_window.c",
            External .. "/glfw/src/x11_init.c",
            External .. "/glfw/src/x11_monitor.c",
            External .. "/glfw/src/x11_window.c",
            External .. "/glfw/src/xkb_unicode.c"
        }

    filter {}

project "FreeType"
    location(ProjectFiles)
    kind "StaticLib"
    language "C"
    cdialect "C99"
    warnings "Off"

    ApplyProjectDirectories()

    defines { "FT2_BUILD_LIBRARY" }

    files {
        External .. "/freetype/include/**.h",
        External .. "/freetype/src/autofit/autofit.c",
        External .. "/freetype/src/base/ftbase.c",
        External .. "/freetype/src/base/ftbbox.c",
        External .. "/freetype/src/base/ftbdf.c",
        External .. "/freetype/src/base/ftbitmap.c",
        External .. "/freetype/src/base/ftcid.c",
        External .. "/freetype/src/base/ftfstype.c",
        External .. "/freetype/src/base/ftgasp.c",
        External .. "/freetype/src/base/ftglyph.c",
        External .. "/freetype/src/base/ftgxval.c",
        External .. "/freetype/src/base/ftinit.c",
        External .. "/freetype/src/base/ftmm.c",
        External .. "/freetype/src/base/ftotval.c",
        External .. "/freetype/src/base/ftpatent.c",
        External .. "/freetype/src/base/ftpfr.c",
        External .. "/freetype/src/base/ftstroke.c",
        External .. "/freetype/src/base/ftsynth.c",
        External .. "/freetype/src/base/fttype1.c",
        External .. "/freetype/src/base/ftwinfnt.c",
        External .. "/freetype/src/bdf/bdf.c",
        External .. "/freetype/src/bzip2/ftbzip2.c",
        External .. "/freetype/src/cache/ftcache.c",
        External .. "/freetype/src/cff/cff.c",
        External .. "/freetype/src/cid/type1cid.c",
        External .. "/freetype/src/gzip/ftgzip.c",
        External .. "/freetype/src/lzw/ftlzw.c",
        External .. "/freetype/src/pcf/pcf.c",
        External .. "/freetype/src/pfr/pfr.c",
        External .. "/freetype/src/psaux/psaux.c",
        External .. "/freetype/src/pshinter/pshinter.c",
        External .. "/freetype/src/psnames/psnames.c",
        External .. "/freetype/src/raster/raster.c",
        External .. "/freetype/src/sdf/sdf.c",
        External .. "/freetype/src/sfnt/sfnt.c",
        External .. "/freetype/src/smooth/smooth.c",
        External .. "/freetype/src/svg/svg.c",
        External .. "/freetype/src/truetype/truetype.c",
        External .. "/freetype/src/type1/type1.c",
        External .. "/freetype/src/type42/type42.c",
        External .. "/freetype/src/winfonts/winfnt.c"
    }

    includedirs { External .. "/freetype/include" }

    filter "system:windows"
        defines {
            "_CRT_NONSTDC_NO_WARNINGS",
            "_CRT_SECURE_NO_WARNINGS"
        }
        files {
            External .. "/freetype/builds/windows/ftdebug.c",
            External .. "/freetype/builds/windows/ftsystem.c"
        }

    filter "system:linux"
        files {
            External .. "/freetype/src/base/ftdebug.c",
            External .. "/freetype/src/base/ftsystem.c"
        }

    filter {}

project "ImGui"
    location(ProjectFiles)
    kind "StaticLib"
    language "C++"
    warnings "Off"

    ApplyProjectDirectories()

    defines { "IMGUI_ENABLE_FREETYPE" }

    files {
        External .. "/imgui/imconfig.h",
        External .. "/imgui/imgui.cpp",
        External .. "/imgui/imgui.h",
        External .. "/imgui/imgui_demo.cpp",
        External .. "/imgui/imgui_draw.cpp",
        External .. "/imgui/imgui_internal.h",
        External .. "/imgui/imgui_tables.cpp",
        External .. "/imgui/imgui_widgets.cpp",
        External .. "/imgui/backends/imgui_impl_glfw.cpp",
        External .. "/imgui/backends/imgui_impl_glfw.h",
        External .. "/imgui/backends/imgui_impl_opengl3.cpp",
        External .. "/imgui/backends/imgui_impl_opengl3.h",
        External .. "/imgui/backends/imgui_impl_opengl3_loader.h",
        External .. "/imgui/misc/freetype/imgui_freetype.cpp",
        External .. "/imgui/misc/freetype/imgui_freetype.h"
    }

    includedirs {
        External .. "/freetype/include",
        External .. "/glfw/include",
        External .. "/imgui"
    }

    links { "FreeType" }

project "AssetBaker"
    location(ProjectFiles)
    kind "ConsoleApp"
    warnings "Extra"
    fatalwarnings "All"

    ApplyProjectDirectories()

    files {
        Root .. "/Source/Assets/AssetPath.h",
        Root .. "/Tools/AssetBaker/**.cpp",
        Root .. "/Tools/AssetBaker/**.h"
    }

    includedirs {
        Root .. "/Source",
        Root .. "/Tools/AssetBaker"
    }

project "StarterApp"
    location(ProjectFiles)
    kind "WindowedApp"
    debugdir(Root)
    warnings "Extra"
    fatalwarnings "All"

    ApplyProjectDirectories()

    files {
        Root .. "/Source/**.cpp",
        Root .. "/Source/**.h",
        AssetManifest
    }

    includedirs {
        Root .. "/Source",
        External .. "/freetype/include",
        External .. "/glfw/include",
        External .. "/imgui"
    }

    links { "ImGui", "GLFW", "FreeType" }

    filter "system:windows"
        entrypoint "mainCRTStartup"
        links { "gdi32", "opengl32", "shell32" }
        files {
            Assets .. "/Application/ApplicationIcon.ico",
            Assets .. "/Application/ApplicationIcon.rc"
        }

    filter "system:linux"
        links { "GL", "dl", "m", "pthread", "rt" }

    filter { "configurations:Shipping" }
        defines { "PROJECTTEMPLATE_EMBEDDED_ASSETS=1" }
        dependson { "AssetBaker" }

    filter { "system:windows", "configurations:Shipping", "files:**/EmbeddedAssets.txt" }
        buildmessage "Embedding application assets"
        buildcommands {
            '"' .. Root .. '/Binaries/' .. WindowsOutputDirectory .. '/AssetBaker.exe"' ..
            ' --manifest "' .. AssetManifest .. '"' ..
            ' --asset-root "' .. Assets .. '"' ..
            ' --output "' .. GeneratedAssetSource .. '"'
        }
        buildinputs(AssetInputs)
        buildoutputs { GeneratedAssetSource }
        compilebuildoutputs "On"

    filter { "system:linux", "configurations:Shipping", "files:**/EmbeddedAssets.txt" }
        buildmessage "Embedding application assets"
        buildcommands {
            '"' .. Root .. '/Binaries/' .. LinuxOutputDirectory .. '/AssetBaker"' ..
            ' --manifest "' .. AssetManifest .. '"' ..
            ' --asset-root "' .. Assets .. '"' ..
            ' --output "' .. GeneratedAssetSource .. '"'
        }
        buildinputs(AssetInputs)
        buildoutputs { GeneratedAssetSource }
        compilebuildoutputs "On"

    filter {}

project "StarterTests"
    location(ProjectFiles)
    kind "ConsoleApp"
    debugdir(Root)
    warnings "Extra"
    fatalwarnings "All"

    ApplyProjectDirectories()

    files {
        Root .. "/Source/Assets/AssetPath.h",
        Root .. "/Source/Assets/AssetProvider.cpp",
        Root .. "/Source/Assets/AssetProvider.h",
        Root .. "/Source/Logging/Log.cpp",
        Root .. "/Source/Logging/Log.h",
        Root .. "/Tools/AssetBaker/AssetBake.cpp",
        Root .. "/Tools/AssetBaker/AssetBake.h",
        Root .. "/Source/UI/LogTextSelection.h",
        Root .. "/Source/UI/OutputLog.h",
        Root .. "/Source/UI/TitleBarLayout.h",
        Root .. "/Tests/**.cpp"
    }

    includedirs {
        Root .. "/Source",
        Root .. "/Tools/AssetBaker",
        External .. "/doctest"
    }

    filter "system:linux"
        links { "pthread" }

    filter {}
