#pragma once

#include "UI/WorkspaceLayout.h"

#include <imgui.h>

namespace ProjectTemplate
{
void PrependGlassSurface(ImDrawList& DrawList, const ImGuiViewport& Viewport, ImVec2 Minimum, ImVec2 Maximum, float Rounding, float Opacity, unsigned int BlurTexture);
void DrawGlassPanels(EPanelTransparencyMode Mode, float Opacity, unsigned int BlurTexture);
}
