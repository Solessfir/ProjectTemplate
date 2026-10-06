#include "UI/GlassPanels.h"

#include "UI/ApplicationTheme.h"

#include <algorithm>
#include <imgui_internal.h>

namespace ProjectTemplate
{
void PrependGlassSurface(ImDrawList& DrawList, const ImGuiViewport& Viewport, const ImVec2 Minimum, const ImVec2 Maximum, const float Rounding, const float Opacity, const unsigned int BlurTexture)
{
	// Keep the existing foreground in channel 1 so the glass precedes decorations, widgets, and text.
	ImVector<ImDrawCmd> ForegroundCommands;
	ImVector<ImDrawIdx> ForegroundIndices;
	ForegroundCommands.swap(DrawList.CmdBuffer);
	ForegroundIndices.swap(DrawList.IdxBuffer);
	DrawList.AddDrawCmd();
	ImDrawListSplitter Splitter;
	Splitter.Split(&DrawList, 2);
	DrawList.PushClipRect(Minimum, Maximum, false);
	if (BlurTexture != 0 && Viewport.Size.x > 0.0f && Viewport.Size.y > 0.0f)
	{
		const ImVec2 UvMinimum = {(Minimum.x - Viewport.Pos.x) / Viewport.Size.x, 1.0f - (Minimum.y - Viewport.Pos.y) / Viewport.Size.y};
		const ImVec2 UvMaximum = {(Maximum.x - Viewport.Pos.x) / Viewport.Size.x, 1.0f - (Maximum.y - Viewport.Pos.y) / Viewport.Size.y};
		DrawList.AddImageRounded(ImTextureRef(static_cast<ImTextureID>(BlurTexture)), Minimum, Maximum, UvMinimum, UvMaximum, IM_COL32_WHITE, Rounding);
	}
	ImVec4 Tint = ImGui::ColorConvertU32ToFloat4(Theme::Colors::Surface0);
	Tint.w = std::clamp(Opacity, 0.0f, 1.0f);
	DrawList.AddRectFilled(Minimum, Maximum, ImGui::ColorConvertFloat4ToU32(Tint), Rounding);
	DrawList.PopClipRect();
	Splitter.SetCurrentChannel(&DrawList, 1);
	ForegroundCommands.swap(DrawList.CmdBuffer);
	ForegroundIndices.swap(DrawList.IdxBuffer);
	Splitter.Merge(&DrawList);
}

void DrawGlassPanels(const EPanelTransparencyMode Mode, const float Opacity, const unsigned int BlurTexture)
{
	for (ImGuiWindow* const Window : ImGui::GetCurrentContext()->Windows)
	{
		if (!Window->Active || Window->Hidden)
		{
			continue;
		}
		const bool bPopup = (Window->Flags & (ImGuiWindowFlags_Popup | ImGuiWindowFlags_Tooltip)) != 0;
		const bool bPanel = (Window->Flags & (ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_DockNodeHost)) == 0 && ((Window->Flags & ImGuiWindowFlags_ChildWindow) == 0 || Window->DockIsActive);
		ImGuiDockNode* const Node = Window->DockNode;
		if ((!bPopup && !bPanel) || (Node && Node->VisibleWindow != Window))
		{
			continue;
		}
		const bool bHasDockHost = Node && Node->HostWindow;
		ImDrawList* const DrawList = bHasDockHost ? Node->HostWindow->DrawList : Window->DrawList;
		const ImVec2 Minimum = bHasDockHost ? Node->Pos : Window->Pos;
		const ImVec2 Size = bHasDockHost ? Node->Size : Window->Size;
		const ImVec2 Maximum = {Minimum.x + Size.x, Minimum.y + Size.y};
		const bool bTransparent = bPopup || IsPanelTransparent(Mode, Window->DockIsActive);
		PrependGlassSurface(*DrawList, *Window->Viewport, Minimum, Maximum, bHasDockHost ? 0.0f : Window->WindowRounding, bTransparent ? Opacity : 1.0f, bTransparent ? BlurTexture : 0);
	}
}
}
