#pragma once

#include <imgui.h>

namespace ProjectTemplate::Theme
{
namespace Colors
{
inline constexpr ImU32 Canvas = IM_COL32(9, 9, 9, 255);
inline constexpr ImU32 Surface0 = IM_COL32(15, 15, 15, 255);
inline constexpr ImU32 Surface1 = IM_COL32(20, 20, 20, 255);
inline constexpr ImU32 Surface2 = IM_COL32(28, 28, 28, 255);
inline constexpr ImU32 SurfaceHover = IM_COL32(34, 34, 34, 255);
inline constexpr ImU32 Border = IM_COL32(38, 38, 38, 255);
inline constexpr ImU32 BorderSoft = IM_COL32(26, 26, 26, 255);
inline constexpr ImU32 TextPrimary = IM_COL32(255, 255, 255, 255);
inline constexpr ImU32 TextSecondary = IM_COL32(179, 179, 179, 255);
inline constexpr ImU32 TextMuted = IM_COL32(153, 153, 153, 255);
inline constexpr ImU32 Accent = IM_COL32(0, 153, 255, 255);
inline constexpr ImU32 AccentHover = IM_COL32(26, 163, 255, 255);
inline constexpr ImU32 CloseHover = IM_COL32(196, 43, 28, 255);
inline constexpr ImU32 GradientViolet = IM_COL32(106, 76, 245, 255);
inline constexpr ImU32 GradientMagenta = IM_COL32(212, 77, 240, 170);
inline constexpr ImU32 GradientCoral = IM_COL32(255, 85, 119, 120);
}

void ApplyApplicationTheme(ImGuiStyle& Style);
}
