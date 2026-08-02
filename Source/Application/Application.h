#pragma once

#include <filesystem>

namespace ProjectTemplate
{
enum class EWindowPlatform
{
	Default,
	X11,
	Wayland
};

int RunApplication(const std::filesystem::path& ExecutablePath, bool bSmokeTest, EWindowPlatform WindowPlatform);
}
