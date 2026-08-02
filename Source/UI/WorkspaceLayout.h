#pragma once

namespace ProjectTemplate
{
inline constexpr float DefaultToolbarHeight = 46.0f;
inline constexpr float ToolbarStatusBreakpoint = 720.0f;

enum class EPanelTransparencyMode
{
	All,
	FloatingOnly,
	DockedOnly,
	Disabled
};

struct FWindowPlacement
{
	int X;
	int Y;
	int Width;
	int Height;
};

[[nodiscard]] constexpr bool IsPanelTransparent(const EPanelTransparencyMode Mode, const bool bDocked) noexcept
{
	switch (Mode)
	{
	case EPanelTransparencyMode::All:
		return true;
	case EPanelTransparencyMode::FloatingOnly:
		return !bDocked;
	case EPanelTransparencyMode::DockedOnly:
		return bDocked;
	case EPanelTransparencyMode::Disabled:
		return false;
	}

	return false;
}

[[nodiscard]] constexpr bool ShouldShowToolbarStatus(const float WindowWidth, const float InterfaceScale) noexcept
{
	return InterfaceScale > 0.0f && WindowWidth >= ToolbarStatusBreakpoint * InterfaceScale;
}

[[nodiscard]] constexpr float ResolveToolbarRightX(const float WindowWidth, const float MinimumRightX, const float RightGroupWidth, const float RightPadding) noexcept
{
	const float AlignedX = WindowWidth - RightPadding - RightGroupWidth;
	return AlignedX > MinimumRightX ? AlignedX : MinimumRightX;
}

[[nodiscard]] constexpr FWindowPlacement ResolveCenteredWindowPlacement(const int WorkAreaX, const int WorkAreaY, const int WorkAreaWidth, const int WorkAreaHeight, const int SizePercent) noexcept
{
	const int Width = WorkAreaWidth * SizePercent / 100;
	const int Height = WorkAreaHeight * SizePercent / 100;
	return {
		WorkAreaX + (WorkAreaWidth - Width) / 2,
		WorkAreaY + (WorkAreaHeight - Height) / 2,
		Width,
		Height
	};
}
}
