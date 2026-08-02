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

project "ImGui"
    location(ProjectFiles)
    kind "StaticLib"
    language "C++"
    warnings "Off"

    ApplyProjectDirectories()

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
        External .. "/imgui/backends/imgui_impl_opengl3_loader.h"
    }

    includedirs {
        External .. "/glfw/include",
        External .. "/imgui"
    }

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
        External .. "/glfw/include",
        External .. "/imgui"
    }

    links { "ImGui", "GLFW" }

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
        Root .. "/Tools/AssetBaker/AssetBake.cpp",
        Root .. "/Tools/AssetBaker/AssetBake.h",
        Root .. "/Source/UI/TitleBarLayout.h",
        Root .. "/Tests/**.cpp"
    }

    includedirs {
        Root .. "/Source",
        Root .. "/Tools/AssetBaker",
        External .. "/doctest"
    }
