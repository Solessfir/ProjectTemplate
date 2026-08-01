#include "UI/ApplicationTheme.h"

namespace ProjectTemplate::Theme
{
namespace
{
[[nodiscard]] ImVec4 ToFloatColor(const ImU32 Color)
{
	return ImGui::ColorConvertU32ToFloat4(Color);
}

[[nodiscard]] ImVec4 WithAlpha(const ImU32 Color, const float Alpha)
{
	ImVec4 Result = ToFloatColor(Color);
	Result.w = Alpha;
	return Result;
}
}

void ApplyApplicationTheme(ImGuiStyle& Style)
{
	Style.FontSizeBase = 15.0f;
	Style.WindowPadding = { 12.0f, 12.0f };
	Style.WindowRounding = 10.0f;
	Style.WindowBorderSize = 1.0f;
	Style.ChildRounding = 10.0f;
	Style.ChildBorderSize = 1.0f;
	Style.PopupRounding = 8.0f;
	Style.PopupBorderSize = 1.0f;
	Style.FramePadding = { 12.0f, 4.0f };
	Style.FrameRounding = 6.0f;
	Style.FrameBorderSize = 1.0f;
	Style.ItemSpacing = { 8.0f, 8.0f };
	Style.ItemInnerSpacing = { 8.0f, 6.0f };
	Style.CellPadding = { 10.0f, 8.0f };
	Style.IndentSpacing = 20.0f;
	Style.ScrollbarSize = 12.0f;
	Style.ScrollbarRounding = 8.0f;
	Style.GrabMinSize = 10.0f;
	Style.GrabRounding = 6.0f;
	Style.TabRounding = 6.0f;
	Style.TabBorderSize = 0.0f;
	Style.TabBarBorderSize = 1.0f;
	Style.DockingSeparatorSize = 1.0f;
	Style.SeparatorSize = 1.0f;
	Style.DisabledAlpha = 0.55f;

	ImVec4* const Palette = Style.Colors;
	Palette[ImGuiCol_Text] = ToFloatColor(Colors::TextPrimary);
	Palette[ImGuiCol_TextDisabled] = ToFloatColor(Colors::TextMuted);
	Palette[ImGuiCol_WindowBg] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_ChildBg] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_PopupBg] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_Border] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_BorderShadow] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Palette[ImGuiCol_FrameBg] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_FrameBgHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_FrameBgActive] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_TitleBg] = ToFloatColor(Colors::Canvas);
	Palette[ImGuiCol_TitleBgActive] = ToFloatColor(Colors::Canvas);
	Palette[ImGuiCol_TitleBgCollapsed] = ToFloatColor(Colors::Canvas);
	Palette[ImGuiCol_MenuBarBg] = ToFloatColor(Colors::Canvas);
	Palette[ImGuiCol_ScrollbarBg] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_ScrollbarGrab] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_ScrollbarGrabHovered] = ToFloatColor(Colors::TextMuted);
	Palette[ImGuiCol_ScrollbarGrabActive] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_CheckMark] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_CheckboxSelectedBg] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_SliderGrab] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_SliderGrabActive] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_Button] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_ButtonHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_ButtonActive] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_Header] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_HeaderHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_HeaderActive] = WithAlpha(Colors::Accent, 0.30f);
	Palette[ImGuiCol_Separator] = ToFloatColor(Colors::BorderSoft);
	Palette[ImGuiCol_SeparatorHovered] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_SeparatorActive] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_ResizeGrip] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Palette[ImGuiCol_ResizeGripHovered] = WithAlpha(Colors::AccentHover, 0.65f);
	Palette[ImGuiCol_ResizeGripActive] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_InputTextCursor] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TabHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_Tab] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_TabSelected] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_TabSelectedOverline] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TabDimmed] = ToFloatColor(Colors::Canvas);
	Palette[ImGuiCol_TabDimmedSelected] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_TabDimmedSelectedOverline] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_DockingPreview] = WithAlpha(Colors::Accent, 0.35f);
	Palette[ImGuiCol_DockingEmptyBg] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_PlotLines] = ToFloatColor(Colors::TextMuted);
	Palette[ImGuiCol_PlotLinesHovered] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_PlotHistogram] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_PlotHistogramHovered] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_TableHeaderBg] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_TableBorderStrong] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_TableBorderLight] = ToFloatColor(Colors::BorderSoft);
	Palette[ImGuiCol_TableRowBg] = { 0.0f, 0.0f, 0.0f, 0.0f };
	Palette[ImGuiCol_TableRowBgAlt] = WithAlpha(Colors::Surface2, 0.45f);
	Palette[ImGuiCol_TextLink] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TextSelectedBg] = WithAlpha(Colors::Accent, 0.35f);
	Palette[ImGuiCol_TreeLines] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_DragDropTarget] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_DragDropTargetBg] = WithAlpha(Colors::Accent, 0.15f);
	Palette[ImGuiCol_UnsavedMarker] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_NavCursor] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_NavWindowingHighlight] = ToFloatColor(Colors::TextPrimary);
	Palette[ImGuiCol_NavWindowingDimBg] = { 0.0f, 0.0f, 0.0f, 0.65f };
	Palette[ImGuiCol_ModalWindowDimBg] = { 0.0f, 0.0f, 0.0f, 0.72f };
}
}
