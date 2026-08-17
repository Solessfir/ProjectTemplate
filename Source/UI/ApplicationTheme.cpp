#include "UI/ApplicationTheme.h"

#include <algorithm>

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

[[nodiscard]] ImVec4 AdditiveTint(const ImU32 BaseColor, const ImU32 AddedColor, const float Intensity)
{
	ImVec4 Result = ToFloatColor(BaseColor);
	const ImVec4 Added = ToFloatColor(AddedColor);
	Result.x = std::min(1.0f, Result.x + Added.x * Intensity);
	Result.y = std::min(1.0f, Result.y + Added.y * Intensity);
	Result.z = std::min(1.0f, Result.z + Added.z * Intensity);
	return Result;
}
}

void ApplyApplicationTheme(ImGuiStyle& Style)
{
	Style.FontSizeBase = 15.0f;
	Style.WindowPadding = {12.0f, 12.0f};
	Style.WindowRounding = Rounding::Window;
	Style.WindowBorderSize = 1.0f;
	Style.ChildRounding = Rounding::Child;
	Style.ChildBorderSize = 1.0f;
	Style.PopupRounding = Rounding::Popup;
	Style.PopupBorderSize = 1.0f;
	Style.FramePadding = {12.0f, 8.0f};
	Style.FrameRounding = Rounding::Frame;
	Style.FrameBorderSize = 1.0f;
	Style.ItemSpacing = {8.0f, 8.0f};
	Style.ItemInnerSpacing = {8.0f, 6.0f};
	Style.CellPadding = {10.0f, 8.0f};
	Style.IndentSpacing = 20.0f;
	Style.ScrollbarSize = 12.0f;
	Style.ScrollbarRounding = Rounding::Scrollbar;
	Style.GrabMinSize = 10.0f;
	Style.GrabRounding = Rounding::Grab;
	Style.ImageRounding = Rounding::Image;
	Style.TabRounding = Rounding::Tab;
	Style.MenuItemRounding = Rounding::MenuItem;
	Style.DragDropTargetRounding = Rounding::DragDropTarget;
	Style.TabBorderSize = 0.0f;
	Style.TabBarBorderSize = 1.0f;
	Style.DockingSeparatorSize = 1.0f;
	Style.SeparatorSize = 1.0f;
	Style.DisabledAlpha = 0.55f;

	ImVec4* const Palette = Style.Colors;
	Palette[ImGuiCol_Text] = ToFloatColor(Colors::TextPrimary);
	Palette[ImGuiCol_TextDisabled] = ToFloatColor(Colors::TextMuted);
	Palette[ImGuiCol_WindowBg] = WithAlpha(Colors::Surface0, 0.94f);
	Palette[ImGuiCol_ChildBg] = WithAlpha(Colors::Surface1, 0.90f);
	Palette[ImGuiCol_PopupBg] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_Border] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_BorderShadow] = WithAlpha(Colors::Canvas, 0.0f);
	Palette[ImGuiCol_FrameBg] = ToFloatColor(Colors::Surface1);
	Palette[ImGuiCol_FrameBgHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_FrameBgActive] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_TitleBg] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_TitleBgActive] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_TitleBgCollapsed] = ToFloatColor(Colors::Surface0);
	Palette[ImGuiCol_MenuBarBg] = ToFloatColor(Colors::Surface0);
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
	Palette[ImGuiCol_ResizeGrip] = WithAlpha(Colors::Canvas, 0.0f);
	Palette[ImGuiCol_ResizeGripHovered] = WithAlpha(Colors::AccentHover, 0.65f);
	Palette[ImGuiCol_ResizeGripActive] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_InputTextCursor] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TabHovered] = ToFloatColor(Colors::SurfaceHover);
	Palette[ImGuiCol_Tab] = WithAlpha(Colors::Surface0, 0.60f);
	Palette[ImGuiCol_TabSelected] = WithAlpha(Colors::Surface2, 0.72f);
	Palette[ImGuiCol_TabSelectedOverline] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TabDimmed] = WithAlpha(Colors::Canvas, 0.45f);
	Palette[ImGuiCol_TabDimmedSelected] = WithAlpha(Colors::Surface1, 0.68f);
	Palette[ImGuiCol_TabDimmedSelectedOverline] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_DockingPreview] = WithAlpha(Colors::TextPrimary, 0.15f);
	Palette[ImGuiCol_DockingEmptyBg] = WithAlpha(Colors::Canvas, 0.0f);
	Palette[ImGuiCol_PlotLines] = ToFloatColor(Colors::TextMuted);
	Palette[ImGuiCol_PlotLinesHovered] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_PlotHistogram] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_PlotHistogramHovered] = ToFloatColor(Colors::AccentHover);
	Palette[ImGuiCol_TableHeaderBg] = ToFloatColor(Colors::Surface2);
	Palette[ImGuiCol_TableBorderStrong] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_TableBorderLight] = ToFloatColor(Colors::BorderSoft);
	Palette[ImGuiCol_TableRowBg] = WithAlpha(Colors::Canvas, 0.0f);
	Palette[ImGuiCol_TableRowBgAlt] = WithAlpha(Colors::Surface2, 0.45f);
	Palette[ImGuiCol_TextLink] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_TextSelectedBg] = WithAlpha(Colors::Accent, 0.35f);
	Palette[ImGuiCol_TreeLines] = ToFloatColor(Colors::Border);
	Palette[ImGuiCol_DragDropTarget] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_DragDropTargetBg] = WithAlpha(Colors::Accent, 0.15f);
	Palette[ImGuiCol_UnsavedMarker] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_NavCursor] = ToFloatColor(Colors::Accent);
	Palette[ImGuiCol_NavWindowingHighlight] = ToFloatColor(Colors::TextPrimary);
	Palette[ImGuiCol_NavWindowingDimBg] = WithAlpha(Colors::Canvas, 0.72f);
	Palette[ImGuiCol_ModalWindowDimBg] = WithAlpha(Colors::Canvas, 0.82f);
	ApplyInteractiveColors(Style, Colors::Accent);
}

void ApplyInteractiveColors(ImGuiStyle& Style, const ImU32 AccentColor)
{
	ImVec4* const Palette = Style.Colors;
	Palette[ImGuiCol_FrameBgHovered] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::HoverTint);
	Palette[ImGuiCol_FrameBgActive] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_ScrollbarGrabHovered] = AdditiveTint(Colors::Border, AccentColor, Interaction::HoverTint);
	Palette[ImGuiCol_ScrollbarGrabActive] = AdditiveTint(Colors::Border, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_CheckMark] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_CheckboxSelectedBg] = AdditiveTint(Colors::Surface2, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_SliderGrab] = AdditiveTint(Colors::Border, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_SliderGrabActive] = AdditiveTint(Colors::Border, AccentColor, Interaction::StrongTint);
	Palette[ImGuiCol_ButtonHovered] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::HoverTint);
	Palette[ImGuiCol_ButtonActive] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_Header] = AdditiveTint(Colors::Surface2, AccentColor, Interaction::SubtleTint);
	Palette[ImGuiCol_HeaderHovered] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::HoverTint);
	Palette[ImGuiCol_HeaderActive] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_SeparatorHovered] = AdditiveTint(Colors::Border, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_SeparatorActive] = AdditiveTint(Colors::Border, AccentColor, Interaction::StrongTint);
	Palette[ImGuiCol_ResizeGripHovered] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::ActiveTint);
	Palette[ImGuiCol_ResizeGripActive] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::StrongTint);
	Palette[ImGuiCol_InputTextCursor] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_TabHovered] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::HoverTint);
	Palette[ImGuiCol_TabSelected] = AdditiveTint(Colors::Surface1, AccentColor, Interaction::SubtleTint);
	Palette[ImGuiCol_TabSelectedOverline] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_DockingPreview] = WithAlpha(AccentColor, 0.15f);
	Palette[ImGuiCol_PlotLinesHovered] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_PlotHistogram] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_PlotHistogramHovered] = AdditiveTint(AccentColor, Colors::TextPrimary, Interaction::ActiveTint);
	Palette[ImGuiCol_TextLink] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_TextSelectedBg] = WithAlpha(AccentColor, 0.35f);
	Palette[ImGuiCol_DragDropTarget] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_DragDropTargetBg] = WithAlpha(AccentColor, 0.15f);
	Palette[ImGuiCol_UnsavedMarker] = ToFloatColor(AccentColor);
	Palette[ImGuiCol_NavCursor] = ToFloatColor(AccentColor);
}
}
