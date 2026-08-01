#pragma once

namespace ProjectTemplate
{
enum class EWindowPlatform
{
	Default,
	X11,
	Wayland
};

int RunApplication(bool bSmokeTest, EWindowPlatform WindowPlatform);
}
