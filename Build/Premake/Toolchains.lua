function ApplyToolchainSettings()
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
