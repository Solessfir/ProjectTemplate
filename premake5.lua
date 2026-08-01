local PremakeRoot = "Build/Premake"
local Root = path.getabsolute(_MAIN_SCRIPT_DIR)
local Action = _ACTION or "NoAction"
local WorkspaceLocation = Root .. "/Intermediate/ProjectFiles/" .. Action

if Action == "vs2022" or Action == "vs2026" then
    WorkspaceLocation = Root
end

include(PremakeRoot .. "/Toolchains.lua")

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

include(PremakeRoot .. "/Projects.lua")
