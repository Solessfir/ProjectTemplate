#include "UI/ApplicationTheme.h"
#include "UI/GlassPanels.h"

#include <doctest/doctest.h>
#include <imgui_internal.h>

TEST_CASE("Glass precedes foreground content and samples the matching flipped viewport region")
{
	ImGuiContext* const Context = ImGui::CreateContext();
	{
		ImDrawList DrawList(ImGui::GetDrawListSharedData());
		DrawList._ResetForNewFrame();
		DrawList.PushClipRectFullScreen();
		DrawList.PushTexture(ImTextureRef(ImTextureID{1}));
		DrawList.AddRectFilled({20.0f, 30.0f}, {30.0f, 40.0f}, IM_COL32_WHITE);
		const int ForegroundIndexCount = DrawList.IdxBuffer.Size;
		ImGuiViewport Viewport;
		Viewport.Pos = {10.0f, 20.0f};
		Viewport.Size = {200.0f, 100.0f};
		ProjectTemplate::PrependGlassSurface(DrawList, Viewport, {30.0f, 30.0f}, {130.0f, 80.0f}, 0.0f, 0.5f, 42);
		REQUIRE(DrawList.CmdBuffer.Size >= 3);
		CHECK(DrawList.CmdBuffer.front().GetTexID() == ImTextureID{42});
		const ImDrawVert& FirstVertex = DrawList.VtxBuffer[DrawList.IdxBuffer.front()];
		CHECK(FirstVertex.uv.x == doctest::Approx(0.10f));
		CHECK(FirstVertex.uv.y == doctest::Approx(0.90f));
		CHECK(DrawList.IdxBuffer[DrawList.IdxBuffer.Size - ForegroundIndexCount] == 0);
		CHECK(DrawList.VtxBuffer[0].col == IM_COL32_WHITE);
		const ImU32 TintColor = DrawList.VtxBuffer[8].col;
		CHECK((TintColor & ~IM_COL32_A_MASK) == (ProjectTemplate::Theme::Colors::Surface0 & ~IM_COL32_A_MASK));
		CHECK((TintColor >> IM_COL32_A_SHIFT) == 128);
		// A later foreground draw must still append indices to the merged buffer.
		DrawList.AddRectFilled({40.0f, 40.0f}, {50.0f, 50.0f}, IM_COL32_WHITE);
		CHECK(DrawList.IdxBuffer.Size == ForegroundIndexCount + 18);
	}
	ImGui::DestroyContext(Context);
}

TEST_CASE("Zero opacity and zero blur leave foreground colors unchanged")
{
	ImGuiContext* const Context = ImGui::CreateContext();
	{
		ImDrawList DrawList(ImGui::GetDrawListSharedData());
		DrawList._ResetForNewFrame();
		DrawList.PushClipRectFullScreen();
		DrawList.PushTexture(ImTextureRef(ImTextureID{1}));
		DrawList.AddRectFilled({20.0f, 30.0f}, {30.0f, 40.0f}, IM_COL32_WHITE);
		ImGuiViewport Viewport;
		Viewport.Size = {200.0f, 100.0f};
		ProjectTemplate::PrependGlassSurface(DrawList, Viewport, {0.0f, 0.0f}, {100.0f, 100.0f}, 0.0f, 0.0f, 0);
		CHECK(DrawList.VtxBuffer.Size == 4);
		CHECK(DrawList.IdxBuffer.Size == 6);
		CHECK(DrawList.VtxBuffer[0].col == IM_COL32_WHITE);
		ImGuiStyle Style;
		ProjectTemplate::Theme::ApplyApplicationTheme(Style);
		ProjectTemplate::Theme::ApplyGlassColors(Style, 0.0f);
		CHECK(Style.Colors[ImGuiCol_Button].w == 0.0f);
		CHECK(Style.Colors[ImGuiCol_FrameBg].w == 0.0f);
		CHECK(Style.Colors[ImGuiCol_Text].w == 1.0f);
	}
	ImGui::DestroyContext(Context);
}

TEST_CASE("Collapsed opaque panels retain a surface while explicit background-free windows stay clear")
{
	ImGuiContext* const Context = ImGui::CreateContext();
	ImGuiIO& IO = ImGui::GetIO();
	IO.IniFilename = nullptr;
	IO.DisplaySize = {800.0f, 600.0f};
	IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	ProjectTemplate::Theme::ApplyApplicationTheme(ImGui::GetStyle());
	ImGui::NewFrame();
	ImGui::SetNextWindowSize({240.0f, 200.0f});
	ImGui::SetNextWindowCollapsed(true);
	ImGui::Begin("Opaque panel");
	ImGuiWindow* const Panel = ImGui::GetCurrentWindow();
	ImGui::End();
	ImGui::Begin("Background-free host", nullptr, ImGuiWindowFlags_NoBackground);
	ImGuiWindow* const Host = ImGui::GetCurrentWindow();
	ImGui::End();
	ImGui::EndFrame();
	const int PanelVertexCount = Panel->DrawList->VtxBuffer.Size;
	const int HostVertexCount = Host->DrawList->VtxBuffer.Size;
	ProjectTemplate::DrawGlassPanels(ProjectTemplate::EPanelTransparencyMode::Disabled, 0.0f, 42);
	CHECK(Panel->DrawList->VtxBuffer.Size > PanelVertexCount);
	CHECK(Host->DrawList->VtxBuffer.Size == HostVertexCount);
	const ImDrawVert& SurfaceVertex = Panel->DrawList->VtxBuffer[Panel->DrawList->IdxBuffer.front()];
	CHECK(SurfaceVertex.col == ProjectTemplate::Theme::Colors::Surface0);
	for (int Index = PanelVertexCount; Index < Panel->DrawList->VtxBuffer.Size; ++Index)
	{
		CHECK(Panel->DrawList->VtxBuffer[Index].pos.y <= Panel->Pos.y + Panel->Size.y + 1.0f);
	}
	ImGui::DestroyContext(Context);
}

TEST_CASE("Docked panels receive glass on their host draw list")
{
	ImGuiContext* const Context = ImGui::CreateContext();
	ImGuiIO& IO = ImGui::GetIO();
	IO.IniFilename = nullptr;
	IO.DisplaySize = {800.0f, 600.0f};
	IO.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ProjectTemplate::Theme::ApplyApplicationTheme(ImGui::GetStyle());
	for (int Frame = 0; Frame < 2; ++Frame)
	{
		ImGui::NewFrame();
		const ImGuiID Dockspace = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::SetNextWindowDockID(Dockspace, ImGuiCond_Always);
		ImGui::Begin("Start");
		ImGuiWindow* const Panel = ImGui::GetCurrentWindow();
		ImGui::TextUnformatted("Foreground");
		ImGui::End();
		ImGui::EndFrame();
		for (ImTextureData* const Texture : IO.Fonts->TexList)
		{
			Texture->SetTexID(1);
			Texture->SetStatus(ImTextureStatus_OK);
		}
		REQUIRE(Panel->DockIsActive);
		REQUIRE(Panel->DockNode->HostWindow != nullptr);
		ImDrawList& DrawList = *Panel->DockNode->HostWindow->DrawList;
		ProjectTemplate::DrawGlassPanels(ProjectTemplate::EPanelTransparencyMode::All, 0.5f, 42);
		if (Frame > 0)
		{
			CHECK_FALSE(Panel->Hidden);
			CHECK(DrawList.CmdBuffer.front().GetTexID() == ImTextureID{42});
		}
		ImGui::Render();
	}
	ImGui::DestroyContext(Context);
}
