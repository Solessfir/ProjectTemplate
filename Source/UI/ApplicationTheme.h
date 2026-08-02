#pragma once

#include <imgui.h>

namespace ProjectTemplate::Theme
{
namespace Colors
{
inline constexpr ImU32 Canvas = IM_COL32(18, 18, 19, 255);
inline constexpr ImU32 ChromeOverlay = IM_COL32(20, 20, 22, 48);
inline constexpr ImU32 TitleBarControlHover = IM_COL32(255, 255, 255, 24);
inline constexpr ImU32 Surface0 = IM_COL32(23, 23, 25, 255);
inline constexpr ImU32 Surface1 = IM_COL32(28, 28, 30, 255);
inline constexpr ImU32 Surface2 = IM_COL32(36, 36, 39, 255);
inline constexpr ImU32 SurfaceHover = IM_COL32(44, 44, 47, 255);
inline constexpr ImU32 Border = IM_COL32(52, 52, 56, 255);
inline constexpr ImU32 BorderSoft = IM_COL32(37, 37, 40, 255);
inline constexpr ImU32 TextPrimary = IM_COL32(255, 255, 255, 255);
inline constexpr ImU32 TextSecondary = IM_COL32(190, 190, 193, 255);
inline constexpr ImU32 TextMuted = IM_COL32(148, 148, 152, 255);
inline constexpr ImU32 Accent = IM_COL32(205, 205, 210, 255);
inline constexpr ImU32 AccentHover = IM_COL32(232, 232, 235, 255);
inline constexpr ImU32 CloseHover = IM_COL32(196, 43, 28, 255);
}

namespace Background
{
struct FPreset
{
	const char* Name;
	ImU32 Color;
};

inline constexpr FPreset Presets[] = {
	{ "Amber", IM_COL32(232, 139, 118, 255) },
	{ "Rust", IM_COL32(205, 148, 30, 255) },
	{ "Olive", IM_COL32(143, 179, 87, 255) },
	{ "Grass", IM_COL32(77, 177, 122, 255) },
	{ "Ocean", IM_COL32(46, 169, 183, 255) },
	{ "Sky", IM_COL32(67, 164, 210, 255) },
	{ "Cobalt", IM_COL32(84, 108, 232, 255) },
	{ "Violet", IM_COL32(147, 80, 220, 255) },
	{ "Plum", IM_COL32(194, 83, 177, 255) }
};

inline constexpr int PresetCount = sizeof(Presets) / sizeof(Presets[0]);
inline constexpr int DefaultPreset = 6;
inline constexpr float DefaultGradientHeight = 0.50f;
inline constexpr float DefaultSaturation = 1.0f;
inline constexpr float DefaultIntensity = 0.18f;
inline constexpr float TrailingIntensityRatio = 0.40f;

static_assert(DefaultPreset >= 0 && DefaultPreset < PresetCount);
}

namespace Rounding
{
inline constexpr float Window = 6.0f;
inline constexpr float Child = 6.0f;
inline constexpr float Popup = 5.0f;
inline constexpr float Frame = 4.0f;
inline constexpr float Scrollbar = 5.0f;
inline constexpr float Grab = 4.0f;
inline constexpr float Tab = 4.0f;
inline constexpr float PrimaryButton = 8.0f;
inline constexpr float TitleBarControlRounding = 4.0f;
}

namespace Interaction
{
inline constexpr float SubtleTint = 0.06f;
inline constexpr float HoverTint = 0.10f;
inline constexpr float ActiveTint = 0.16f;
inline constexpr float StrongTint = 0.24f;
}

void ApplyApplicationTheme(ImGuiStyle& Style);
void ApplyInteractiveColors(ImGuiStyle& Style, ImU32 AccentColor);
}
