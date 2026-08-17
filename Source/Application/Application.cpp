#include "Application/Application.h"

#include "Application/ApplicationPaths.h"
#include "Assets/AssetProvider.h"
#include "Logging/Log.h"
#include "Rendering/BackgroundRenderer.h"
#include "UI/ApplicationTheme.h"
#include "UI/OutputLog.h"
#include "UI/TitleBarLayout.h"
#include "UI/WorkspaceLayout.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <imgui.h>
#include <imgui_internal.h>
#include <limits>
#include <optional>
#include <print>
#include <string>
#include <system_error>

namespace ProjectTemplate
{
namespace
{
constexpr const char* ApplicationTitle = "Project Template";
constexpr const char* MainDockspaceName = "MainDockspaceV3";
constexpr std::array<std::string_view, 4> OutputLogCommands = {"clear", "components", "help", "licenses"};
constexpr int DefaultWindowWidth = 1280;
constexpr int DefaultWindowHeight = 720;
constexpr int InitialWindowWorkAreaPercent = 80;

#ifdef PROJECT_DEBUG
constexpr const char* BuildConfiguration = "Debug";
#elifdef PROJECT_DEVELOPMENT
constexpr const char* BuildConfiguration = "Development";
#elifdef PROJECT_SHIPPING
constexpr const char* BuildConfiguration = "Shipping";
#else
constexpr const char* BuildConfiguration = "Unknown";
#endif

struct FWindowState
{
	FTitleBarHitTestState TitleBarHitTest;
	float UiScale = 1.0f;
	double CursorX = 0.0;
	double CursorY = 0.0;
	bool bFocused = true;
	bool bCursorInside = false;
	bool bWayland = false;
};

struct FApplicationFonts
{
	ImFont* Regular = nullptr;
	ImFont* Medium = nullptr;
};

struct FApplicationResources
{
	FApplicationFonts Fonts;
	std::string FreeTypeLicense;
	std::string RobotoLicense;
};

struct FUiState
{
	char ApplicationName[64] = "Native App"; // NOLINT(modernize-avoid-c-arrays) ImGui edits this fixed C buffer in place.
	ImVec4 BackgroundColor = ImGui::ColorConvertU32ToFloat4(Theme::Background::Presets[Theme::Background::DefaultPreset].Color);
	float GradientHeight = Theme::Background::DefaultGradientHeight;
	float BackgroundSaturation = Theme::Background::DefaultSaturation;
	float BackgroundIntensity = Theme::Background::DefaultIntensity;
	int BackgroundPreset = Theme::Background::DefaultPreset;
	EPanelTransparencyMode PanelTransparencyMode = EPanelTransparencyMode::All;
	bool bStartPanelDocked = true;
	bool bOutputLogDocked = true;
	bool bDemoPanelDocked = false;
	bool bShowOutputLog = true;
	bool bShowDemoWindow = false;
	bool bOpenLicenses = false;
};

struct FApplicationRuntime
{
	FWindowState Window;
	FUiState Ui;
	FOutputLogPanel OutputLog;
	FBackgroundRenderer BackgroundRenderer;
	const FApplicationResources* Resources = nullptr;
	ImGuiStyle BaseStyle;
	float PreviousContentScale = 0.0f;
	bool bRendererReady = false;
	bool bRenderingFrame = false;
};

struct FPanelTransparencyOption
{
	EPanelTransparencyMode Mode;
	const char* Label;
};

constexpr std::array PanelTransparencyOptions = {
    FPanelTransparencyOption{EPanelTransparencyMode::All, "All panels"},
    FPanelTransparencyOption{EPanelTransparencyMode::FloatingOnly, "Floating panels only"},
    FPanelTransparencyOption{EPanelTransparencyMode::DockedOnly, "Docked panels only"},
    FPanelTransparencyOption{EPanelTransparencyMode::Disabled, "Disabled"}};

[[nodiscard]] constexpr const char* GetPanelTransparencyLabel(const EPanelTransparencyMode Mode) noexcept
{
	for (const FPanelTransparencyOption& Option : PanelTransparencyOptions)
	{
		if (Option.Mode == Mode)
		{
			return Option.Label;
		}
	}

	return "Unknown";
}

void SetNextPanelTransparency(const EPanelTransparencyMode Mode, const bool bDocked)
{
	ImGui::SetNextWindowBgAlpha(IsPanelTransparent(Mode, bDocked) ? 0.0f : 1.0f);
}

void EnsureDefaultDockLayout(const ImGuiID DockspaceId, const ImVec2 DockspaceSize)
{
	// Bump MainDockspaceName when a structural default changes. Existing layouts remain user-owned between versions.
	if (ImGui::DockBuilderGetNode(DockspaceId) != nullptr)
	{
		return;
	}

	ImGui::DockBuilderAddNode(DockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(DockspaceId, DockspaceSize);
	ImGuiID CenterDockId = DockspaceId;
	ImGuiID OutputLogDockId = 0;
	ImGui::DockBuilderSplitNode(DockspaceId, ImGuiDir_Down, 0.27f, &OutputLogDockId, &CenterDockId);
	ImGui::DockBuilderDockWindow("Start", CenterDockId);
	ImGui::DockBuilderDockWindow("Output Log", OutputLogDockId);
	ImGui::DockBuilderFinish(DockspaceId);
}

[[nodiscard]] ImFont* LoadFont(FAssetProvider& AssetProvider, ImGuiIO& IO, const std::string_view VirtualPath)
{
	const std::expected<std::span<const std::byte>, FAssetLoadError> FontData = AssetProvider.Load(VirtualPath);
	if (!FontData)
	{
		std::println(stderr, "Could not load font '{}': {}", VirtualPath, FontData.error().Message);
		return nullptr;
	}

	if (FontData->size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
	{
		std::println(stderr, "Font is too large for Dear ImGui: {}", VirtualPath);
		return nullptr;
	}

	ImFontConfig FontConfig;
	// ImGui keeps the font source for dynamic glyph generation, so the asset provider owns it until context shutdown.
	FontConfig.FontDataOwnedByAtlas = false;
	return IO.Fonts->AddFontFromMemoryTTF(
	    const_cast<std::byte*>(FontData->data()),
	    static_cast<int>(FontData->size()),
	    0.0f,
	    &FontConfig);
}

[[nodiscard]] std::expected<std::string, FAssetLoadError> LoadTextAsset(FAssetProvider& AssetProvider, const std::string_view VirtualPath)
{
	const std::expected<std::span<const std::byte>, FAssetLoadError> Data = AssetProvider.Load(VirtualPath);
	if (!Data)
	{
		return std::unexpected(Data.error());
	}

	std::string Text;
	Text.reserve(Data->size());
	for (const std::byte Byte : *Data)
	{
		Text.push_back(static_cast<char>(std::to_integer<unsigned char>(Byte)));
	}

	return Text;
}

[[nodiscard]] std::optional<FApplicationResources> LoadApplicationResources(FAssetProvider& AssetProvider, ImGuiIO& IO)
{
	FApplicationResources Resources;
	Resources.Fonts.Regular = LoadFont(AssetProvider, IO, "Fonts/Roboto/Roboto-Regular.ttf");
	Resources.Fonts.Medium = LoadFont(AssetProvider, IO, "Fonts/Roboto/Roboto-Medium.ttf");
	if (Resources.Fonts.Regular == nullptr || Resources.Fonts.Medium == nullptr)
	{
		return std::nullopt;
	}

	// Redistributed fonts and statically linked FreeType require their notices to remain available in binary builds.
	std::expected<std::string, FAssetLoadError> RobotoLicense = LoadTextAsset(AssetProvider, "Fonts/Roboto/OFL.txt");
	if (!RobotoLicense)
	{
		std::println(stderr, "Could not load Roboto license: {}", RobotoLicense.error().Message);
		return std::nullopt;
	}
	Resources.RobotoLicense = std::move(*RobotoLicense);

	std::expected<std::string, FAssetLoadError> FreeTypeLicense = LoadTextAsset(AssetProvider, "Licenses/FreeType.txt");
	if (!FreeTypeLicense)
	{
		std::println(stderr, "Could not load FreeType license acknowledgement: {}", FreeTypeLicense.error().Message);
		return std::nullopt;
	}
	Resources.FreeTypeLicense = std::move(*FreeTypeLicense);

	IO.FontDefault = Resources.Fonts.Regular;
	return Resources;
}

void GlfwErrorCallback(const int Error, const char* const Description)
{
	Log::Error("GLFW", "Error {}: {}", Error, Description);
}

[[nodiscard]] FApplicationRuntime* GetApplicationRuntime(GLFWwindow* const Window)
{
	return static_cast<FApplicationRuntime*>(glfwGetWindowUserPointer(Window));
}

[[nodiscard]] FWindowState* GetWindowState(GLFWwindow* const Window)
{
	FApplicationRuntime* const Runtime = GetApplicationRuntime(Window);
	return Runtime == nullptr ? nullptr : &Runtime->Window;
}

void UpdateTitleBarUiCaptureRegions(FWindowState& State)
{
	FTitleBarHitTestState& HitTestState = State.TitleBarHitTest;
	HitTestState.UiCaptureRegionCount = 0;
	HitTestState.bUiCapturesEntireTitleBar = false;

	const ImGuiViewport& Viewport = *ImGui::GetMainViewport();
	const FTitleBarLayout& Layout = HitTestState.Layout;
	const ImRect TitleBarRect{
	    Viewport.Pos,
	    {Viewport.Pos.x + static_cast<float>(Layout.WindowWidth), Viewport.Pos.y + static_cast<float>(Layout.TitleBarHeight)}};
	const ImGuiContext& Context = *ImGui::GetCurrentContext();
	for (const ImGuiWindow* const UiWindow : Context.Windows)
	{
		if (!UiWindow->Active || UiWindow->Hidden || UiWindow->Viewport != &Viewport || (UiWindow->Flags & ImGuiWindowFlags_NoMouseInputs) != 0)
		{
			continue;
		}

		const ImRect Intersection{
		    {std::max(UiWindow->OuterRectClipped.Min.x, TitleBarRect.Min.x), std::max(UiWindow->OuterRectClipped.Min.y, TitleBarRect.Min.y)},
		    {std::min(UiWindow->OuterRectClipped.Max.x, TitleBarRect.Max.x), std::min(UiWindow->OuterRectClipped.Max.y, TitleBarRect.Max.y)}};
		if (Intersection.Min.x >= Intersection.Max.x || Intersection.Min.y >= Intersection.Max.y)
		{
			continue;
		}

		if (HitTestState.UiCaptureRegionCount >= HitTestState.UiCaptureRegions.size())
		{
			// Losing caption dragging is safer than sending a UI click to native window chrome.
			HitTestState.UiCaptureRegionCount = 0;
			HitTestState.bUiCapturesEntireTitleBar = true;
			break;
		}

		HitTestState.UiCaptureRegions[HitTestState.UiCaptureRegionCount++] = {
		    .MinimumX = std::clamp(static_cast<int>(std::floor(Intersection.Min.x - Viewport.Pos.x)), 0, Layout.WindowWidth),
		    .MinimumY = std::clamp(static_cast<int>(std::floor(Intersection.Min.y - Viewport.Pos.y)), 0, Layout.TitleBarHeight),
		    .MaximumX = std::clamp(static_cast<int>(std::ceil(Intersection.Max.x - Viewport.Pos.x)), 0, Layout.WindowWidth),
		    .MaximumY = std::clamp(static_cast<int>(std::ceil(Intersection.Max.y - Viewport.Pos.y)), 0, Layout.TitleBarHeight)};
	}
}

void UpdateTitleBarScale(FWindowState& State, const float ContentScale)
{
	// Wayland window and cursor coordinates are already logical units.
	State.UiScale = ResolveTitleBarUiScale(State.bWayland, ContentScale);
	State.TitleBarHitTest.Layout.TitleBarHeight = ScaleTitleBarMetric(DefaultTitleBarHeight, State.UiScale);
	State.TitleBarHitTest.Layout.ButtonWidth = ScaleTitleBarMetric(46, State.UiScale);
	State.TitleBarHitTest.Layout.ResizeBorder = ScaleTitleBarMetric(6, State.UiScale);
}

void WindowSizeCallback(GLFWwindow* const Window, const int Width, const int Height)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->TitleBarHitTest.Layout.WindowWidth = Width;
		State->TitleBarHitTest.Layout.WindowHeight = Height;
	}
}

void WindowContentScaleCallback(GLFWwindow* const Window, const float XScale, const float YScale)
{
	(void)XScale;
	if (FWindowState* const State = GetWindowState(Window))
	{
		UpdateTitleBarScale(*State, YScale);
	}
}

void WindowMaximizeCallback(GLFWwindow* const Window, const int bMaximized)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->TitleBarHitTest.Layout.bMaximized = bMaximized == GLFW_TRUE;
	}
}

void WindowFocusCallback(GLFWwindow* const Window, const int bFocused)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->bFocused = bFocused == GLFW_TRUE;
	}
}

void CursorPositionCallback(GLFWwindow* const Window, const double X, const double Y)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->CursorX = X;
		State->CursorY = Y;
	}
}

void SynchronizeMousePositionBeforeButtonCallback(GLFWwindow* const Window, const int Button, const int Action, const int Modifiers)
{
	(void)Button;
	(void)Action;
	(void)Modifiers;
	double CursorX = 0.0;
	double CursorY = 0.0;
	glfwGetCursorPos(Window, &CursorX, &CursorY);
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->CursorX = CursorX;
		State->CursorY = CursorY;
	}

	// ImGui chains this callback before its button event, keeping the press paired with the cursor sample that produced it.
	ImGui::GetIO().AddMousePosEvent(static_cast<float>(CursorX), static_cast<float>(CursorY));
}

void CursorEnterCallback(GLFWwindow* const Window, const int bEntered)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->bCursorInside = bEntered == GLFW_TRUE;
	}
}

[[nodiscard]] int ToGlfwHitTest(const ETitleBarHitRegion Region)
{
	switch (Region)
	{
		case ETitleBarHitRegion::Client:
			return GLFW_HIT_TEST_CLIENT;
		case ETitleBarHitRegion::Caption:
			return GLFW_HIT_TEST_CAPTION;
		case ETitleBarHitRegion::ResizeLeft:
			return GLFW_HIT_TEST_RESIZE_LEFT;
		case ETitleBarHitRegion::ResizeRight:
			return GLFW_HIT_TEST_RESIZE_RIGHT;
		case ETitleBarHitRegion::ResizeTop:
			return GLFW_HIT_TEST_RESIZE_TOP;
		case ETitleBarHitRegion::ResizeBottom:
			return GLFW_HIT_TEST_RESIZE_BOTTOM;
		case ETitleBarHitRegion::ResizeTopLeft:
			return GLFW_HIT_TEST_RESIZE_TOP_LEFT;
		case ETitleBarHitRegion::ResizeTopRight:
			return GLFW_HIT_TEST_RESIZE_TOP_RIGHT;
		case ETitleBarHitRegion::ResizeBottomLeft:
			return GLFW_HIT_TEST_RESIZE_BOTTOM_LEFT;
		case ETitleBarHitRegion::ResizeBottomRight:
			return GLFW_HIT_TEST_RESIZE_BOTTOM_RIGHT;
		case ETitleBarHitRegion::SystemMenu:
			return GLFW_HIT_TEST_SYSTEM_MENU;
		case ETitleBarHitRegion::ApplicationMenu:
			return GLFW_HIT_TEST_CLIENT;
		case ETitleBarHitRegion::MinimizeButton:
			return GLFW_HIT_TEST_MINIMIZE_BUTTON;
		case ETitleBarHitRegion::MaximizeButton:
			return GLFW_HIT_TEST_MAXIMIZE_BUTTON;
		case ETitleBarHitRegion::CloseButton:
			return GLFW_HIT_TEST_CLOSE_BUTTON;
	}

	return GLFW_HIT_TEST_CLIENT;
}

int TitleBarHitTestCallback(GLFWwindow* const Window, const int X, const int Y)
{
	const FWindowState* const State = GetWindowState(Window);
	return State == nullptr
	           ? GLFW_HIT_TEST_CLIENT
	           : ToGlfwHitTest(HitTestTitleBar(State->TitleBarHitTest, X, Y));
}

[[nodiscard]] ETitleBarHitRegion GetHoveredRegion(const FWindowState& State)
{
	if (!State.bCursorInside)
	{
		return ETitleBarHitRegion::Client;
	}

	return HitTestTitleBar(State.TitleBarHitTest, static_cast<int>(State.CursorX), static_cast<int>(State.CursorY));
}

[[nodiscard]] ImU32 ApplySaturation(const ImU32 Color, const float SaturationScale)
{
	ImVec4 Result = ImGui::ColorConvertU32ToFloat4(Color);
	float Hue = 0.0f;
	float Saturation = 0.0f;
	float Value = 0.0f;
	ImGui::ColorConvertRGBtoHSV(Result.x, Result.y, Result.z, Hue, Saturation, Value);
	ImGui::ColorConvertHSVtoRGB(Hue, Saturation * SaturationScale, Value, Result.x, Result.y, Result.z);
	return ImGui::ColorConvertFloat4ToU32(Result);
}

[[nodiscard]] ImU32 ResolveBackgroundAccent(const FUiState& State)
{
	return ApplySaturation(ImGui::ColorConvertFloat4ToU32(State.BackgroundColor), State.BackgroundSaturation);
}

[[nodiscard]] FBackgroundGradient ResolveApplicationBackground(const FUiState& State, const bool bFocused)
{
	const float FocusSaturation = bFocused ? State.BackgroundSaturation : State.BackgroundSaturation * Theme::Background::UnfocusedSaturationRatio;
	const ImU32 GradientColor = ApplySaturation(ImGui::ColorConvertFloat4ToU32(State.BackgroundColor), FocusSaturation);
	const ImVec4 Accent = ImGui::ColorConvertU32ToFloat4(GradientColor);
	return {
	    .AccentRed = Accent.x,
	    .AccentGreen = Accent.y,
	    .AccentBlue = Accent.z,
	    .HeightRatio = State.GradientHeight,
	    .Intensity = State.BackgroundIntensity,
	    .TrailingIntensityRatio = Theme::Background::TrailingIntensityRatio};
}

void DrawSystemButtonBackground(ImDrawList& DrawList, const float Left, const float Top, const float Width, const float Height, const bool bHovered, const bool bCloseButton)
{
	if (!bHovered)
	{
		return;
	}

	const ImU32 Color = bCloseButton ? Theme::Colors::CloseHover : Theme::Colors::TitleBarControlHover;
	DrawList.AddRectFilled(
	    {Left, Top},
	    {Left + Width, Top + Height},
	    Color,
	    Theme::Rounding::TitleBarControlRounding * Height / 40.0f);
}

void DrawRestoreGlyph(ImDrawList& DrawList, const ImVec2 Center, const float Scale, const ImU32 Color, const float Thickness)
{
	const float BackLeft = Center.x - 3.0f * Scale;
	const float BackTop = Center.y - 5.0f * Scale;
	const float BackRight = Center.x + 5.0f * Scale;
	const float BackBottom = Center.y + 3.0f * Scale;
	const float FrontLeft = Center.x - 5.0f * Scale;
	const float FrontTop = Center.y - 3.0f * Scale;
	const float FrontRight = Center.x + 3.0f * Scale;
	const float FrontBottom = Center.y + 5.0f * Scale;

	DrawList.AddLine({BackLeft, BackTop}, {BackRight, BackTop}, Color, Thickness);
	DrawList.AddLine({BackRight, BackTop}, {BackRight, BackBottom}, Color, Thickness);
	DrawList.AddLine({BackLeft, BackTop}, {BackLeft, FrontTop}, Color, Thickness);
	DrawList.AddLine({FrontRight, BackBottom}, {BackRight, BackBottom}, Color, Thickness);
	DrawList.AddRect({FrontLeft, FrontTop}, {FrontRight, FrontBottom}, Color, 0.0f, 0, Thickness);
}

void DrawTitleBar(const FWindowState& State, const FApplicationFonts& Fonts)
{
	const FTitleBarLayout& Layout = State.TitleBarHitTest.Layout;
	ImDrawList& DrawList = *ImGui::GetBackgroundDrawList();
	const float Width = static_cast<float>(Layout.WindowWidth);
	const float Height = static_cast<float>(Layout.TitleBarHeight);
	const float ButtonWidth = static_cast<float>(Layout.ButtonWidth);
	const float Scale = Height / 40.0f;
	const ETitleBarHitRegion HoveredRegion = GetHoveredRegion(State);

	DrawSystemButtonBackground(
	    DrawList,
	    Width - ButtonWidth * 3.0f,
	    0.0f,
	    ButtonWidth,
	    Height,
	    HoveredRegion == ETitleBarHitRegion::MinimizeButton,
	    false);

	DrawSystemButtonBackground(
	    DrawList,
	    Width - ButtonWidth * 2.0f,
	    0.0f,
	    ButtonWidth,
	    Height,
	    HoveredRegion == ETitleBarHitRegion::MaximizeButton,
	    false);

	DrawSystemButtonBackground(
	    DrawList,
	    Width - ButtonWidth,
	    0.0f,
	    ButtonWidth,
	    Height,
	    HoveredRegion == ETitleBarHitRegion::CloseButton,
	    true);

	constexpr ImU32 GlyphColor = Theme::Colors::TextPrimary;
	const float LineThickness = std::max(1.0f, Scale);
	const float MinimizeCenterX = Width - ButtonWidth * 2.5f;
	const float CenterY = Height * 0.5f;

	DrawList.AddLine(
	    {MinimizeCenterX - 5.0f * Scale, CenterY + 3.0f * Scale},
	    {MinimizeCenterX + 5.0f * Scale, CenterY + 3.0f * Scale},
	    GlyphColor,
	    LineThickness);

	const float MaximizeCenterX = Width - ButtonWidth * 1.5f;
	if (Layout.bMaximized)
	{
		DrawRestoreGlyph(DrawList, {MaximizeCenterX, CenterY}, Scale, GlyphColor, LineThickness);
	}
	else
	{
		DrawList.AddRect(
		    {MaximizeCenterX - 5.0f * Scale, CenterY - 5.0f * Scale},
		    {MaximizeCenterX + 5.0f * Scale, CenterY + 5.0f * Scale},
		    GlyphColor,
		    0.0f,
		    0,
		    LineThickness);
	}

	const float CloseCenterX = Width - ButtonWidth * 0.5f;
	DrawList.AddLine(
	    {CloseCenterX - 5.0f * Scale, CenterY - 5.0f * Scale},
	    {CloseCenterX + 5.0f * Scale, CenterY + 5.0f * Scale},
	    GlyphColor,
	    LineThickness);

	DrawList.AddLine(
	    {CloseCenterX + 5.0f * Scale, CenterY - 5.0f * Scale},
	    {CloseCenterX - 5.0f * Scale, CenterY + 5.0f * Scale},
	    GlyphColor,
	    LineThickness);

	const float IconCenter = Height * 0.5f;
	if (HoveredRegion == ETitleBarHitRegion::SystemMenu)
	{
		DrawList.AddRectFilled(
		    {0.0f, 0.0f},
		    {Height, Height},
		    Theme::Colors::TitleBarControlHover,
		    Theme::Rounding::TitleBarControlRounding * Scale);
	}
	DrawList.AddCircleFilled({IconCenter, IconCenter}, 8.0f * Scale, Theme::Colors::Accent);
	DrawList.AddCircle({IconCenter, IconCenter}, 4.0f * Scale, Theme::Colors::Canvas, 0, LineThickness);

	ImGui::PushFont(Fonts.Medium, 0.0f);
	const ImVec2 TextSize = ImGui::CalcTextSize(ApplicationTitle);

	DrawList.PushClipRect({Height * 2.0f, 0.0f}, {Width - ButtonWidth * 3.0f, Height}, true);
	DrawList.AddText(
	    {Height * 2.0f + 10.0f * Scale, (Height - TextSize.y) * 0.5f},
	    State.bFocused ? Theme::Colors::TextPrimary : Theme::Colors::TextMuted,
	    ApplicationTitle);
	DrawList.PopClipRect();
	ImGui::PopFont();
}

void DrawApplicationMenu(GLFWwindow* const Window, const FTitleBarLayout& Layout, FUiState& State)
{
	const float Height = static_cast<float>(Layout.TitleBarHeight);
	const float Scale = Height / 40.0f;
	const ImVec2 ButtonPosition = {Height, 0.0f};
	const ImVec2 ButtonSize = {Height, Height};

	ImGui::SetNextWindowPos(ButtonPosition);
	ImGui::SetNextWindowSize(ButtonSize);
	ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	constexpr ImGuiWindowFlags HostFlags =
	    ImGuiWindowFlags_NoDecoration |
	    ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoSavedSettings |
	    ImGuiWindowFlags_NoDocking |
	    ImGuiWindowFlags_NoBringToFrontOnFocus |
	    ImGuiWindowFlags_NoNavFocus |
	    ImGuiWindowFlags_NoScrollbar |
	    ImGuiWindowFlags_NoScrollWithMouse |
	    ImGuiWindowFlags_NoBackground;
	ImGui::Begin("ApplicationMenuHost", nullptr, HostFlags);
	ImGui::PopStyleVar(3);

	if (ImGui::InvisibleButton("ApplicationMenuButton", ButtonSize))
	{
		ImGui::OpenPopup("ApplicationMenu");
	}

	ImDrawList& DrawList = *ImGui::GetWindowDrawList();
	const ImVec2 ButtonMin = ImGui::GetItemRectMin();
	const ImVec2 ButtonMax = ImGui::GetItemRectMax();
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
	{
		DrawList.AddRectFilled(
		    ButtonMin,
		    ButtonMax,
		    Theme::Colors::TitleBarControlHover,
		    Theme::Rounding::TitleBarControlRounding * Scale);
	}

	const ImU32 GlyphColor = Theme::Colors::TextPrimary;
	const float LineThickness = std::max(1.0f, Scale);
	const float CenterX = (ButtonMin.x + ButtonMax.x) * 0.5f;
	const float CenterY = (ButtonMin.y + ButtonMax.y) * 0.5f;
	const float LineLeft = CenterX - 7.0f * Scale;
	const float LineRight = CenterX + 7.0f * Scale;
	for (const float Offset : {-5.0f, 0.0f, 5.0f})
	{
		const float Y = CenterY + Offset * Scale;
		DrawList.AddLine({LineLeft, Y}, {LineRight, Y}, GlyphColor, LineThickness);
	}

	ImGui::SetNextWindowPos({ButtonPosition.x, Height}, ImGuiCond_Appearing);
	if (ImGui::BeginPopup("ApplicationMenu"))
	{
		ImGui::MenuItem("Output Log", nullptr, &State.bShowOutputLog);

		if (ImGui::MenuItem("Components"))
		{
			State.bShowDemoWindow = true;
		}

		if (ImGui::MenuItem("Licenses"))
		{
			State.bOpenLicenses = true;
		}

		ImGui::Separator();
		if (ImGui::MenuItem("Exit"))
		{
			glfwSetWindowShouldClose(Window, GLFW_TRUE);
		}
		ImGui::EndPopup();
	}

	ImGui::End();
}

[[nodiscard]] bool DrawPrimaryButton(const char* const Label, const FApplicationFonts& Fonts)
{
	ImGui::PushFont(Fonts.Medium, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Theme::Rounding::PrimaryButton);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::Canvas);
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(232, 232, 232, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 210, 210, 255));
	const bool bPressed = ImGui::Button(Label);
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar(2);
	ImGui::PopFont();
	return bPressed;
}

void DrawWorkspaceIntro(FUiState& State, const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.0f * InterfaceScale, 20.0f * InterfaceScale});
	ImGui::BeginChild(
	    "WorkspaceIntro",
	    {0.0f, 188.0f * InterfaceScale},
	    ImGuiChildFlags_AlwaysUseWindowPadding,
	    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImGui::PushFont(Fonts.Medium, 13.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::Accent);
	ImGui::TextUnformatted("PROJECT TEMPLATE / NATIVE C++23");
	ImGui::PopStyleColor();
	ImGui::PopFont();
	ImGui::Dummy({0.0f, 6.0f * InterfaceScale});
	ImGui::PushFont(Fonts.Medium, 30.0f);
	ImGui::TextUnformatted("Build something native.");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("A focused Windows and Linux starting point with modern C++, native window behavior, and a UI ready to shape.");
	ImGui::PopStyleColor();
	ImGui::Dummy({0.0f, 10.0f * InterfaceScale});

	if (DrawPrimaryButton("Explore components", Fonts))
	{
		State.bShowDemoWindow = true;
	}

	ImGui::SameLine();
	if (ImGui::Button("View licenses"))
	{
		State.bOpenLicenses = true;
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
}

void DrawProjectCard(FUiState& State, const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Theme::Rounding::Child * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.0f * InterfaceScale, 16.0f * InterfaceScale});
	ImGui::BeginChild(
	    "ProjectCard",
	    {0.0f, 180.0f * InterfaceScale},
	    ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	ImGui::PushFont(Fonts.Medium, 16.0f);
	ImGui::TextUnformatted("Template identity");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("Give the derived application a working name before replacing the template identifiers.");
	ImGui::PopStyleColor();
	ImGui::Dummy({0.0f, 4.0f * InterfaceScale});
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::InputTextWithHint("##ApplicationName", "Application name", State.ApplicationName, sizeof(State.ApplicationName));
	if (ImGui::Button("Copy name"))
	{
		ImGui::SetClipboardText(State.ApplicationName);
	}
	ImGui::SameLine();
	ImGui::TextDisabled("Ready for README replacement");

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void DrawBuildProfileCard(const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Theme::Rounding::Child * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.0f * InterfaceScale, 16.0f * InterfaceScale});
	ImGui::BeginChild(
	    "BuildProfileCard",
	    {0.0f, 180.0f * InterfaceScale},
	    ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	ImGui::PushFont(Fonts.Medium, 16.0f);
	ImGui::TextUnformatted("Build profile");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("Production-shaped defaults without framework weight.");
	ImGui::PopStyleColor();
	ImGui::Dummy({0.0f, 4.0f * InterfaceScale});

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0.0f, 3.0f * InterfaceScale});
	if (ImGui::BeginTable("BuildProfile", 2, ImGuiTableFlags_SizingStretchProp))
	{
		struct FBuildProfileRow
		{
			const char* Name;
			const char* Value;
		};

		constexpr std::array Rows = {
		    FBuildProfileRow{"Language", "C++23"},
		    FBuildProfileRow{"Window", "GLFW native title bar"},
		    FBuildProfileRow{"Interface", "Dear ImGui docking"},
		    FBuildProfileRow{"Shipping", "Embedded assets"}};

		for (const auto& Row : Rows)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", Row.Name);
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", Row.Value);
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void DrawAppearanceCard(FUiState& State, const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	const int ColumnCount = ImGui::GetContentRegionAvail().x >= 600.0f * InterfaceScale ? 3 : 1;
	const int RowCount = (Theme::Background::PresetCount + ColumnCount - 1) / ColumnCount;
	const float CardHeight = (326.0f + static_cast<float>(RowCount) * 30.0f) * InterfaceScale;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Theme::Rounding::Child * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16.0f * InterfaceScale, 16.0f * InterfaceScale});
	ImGui::BeginChild(
	    "AppearanceCard",
	    {0.0f, CardHeight},
	    ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	ImGui::PushFont(Fonts.Medium, 16.0f);
	ImGui::TextUnformatted("Background appearance");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("Choose an accent and control how far its gradient reaches into the workspace.");
	ImGui::PopStyleColor();
	ImGui::Dummy({0.0f, 4.0f * InterfaceScale});

	ImGui::TextUnformatted("Panel transparency");
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::BeginCombo("##PanelTransparency", GetPanelTransparencyLabel(State.PanelTransparencyMode)))
	{
		for (const FPanelTransparencyOption& Option : PanelTransparencyOptions)
		{
			const bool bSelected = State.PanelTransparencyMode == Option.Mode;
			if (ImGui::Selectable(Option.Label, bSelected))
			{
				State.PanelTransparencyMode = Option.Mode;
			}

			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	ImGui::TextUnformatted("Gradient height");
	int GradientHeightPercent = static_cast<int>(std::lround(State.GradientHeight * 100.0f));
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderInt("##GradientHeight", &GradientHeightPercent, 10, 100, "%d%%"))
	{
		State.GradientHeight = static_cast<float>(GradientHeightPercent) / 100.0f;
	}
	ImGui::TextUnformatted("Color saturation");
	int SaturationPercent = static_cast<int>(std::lround(State.BackgroundSaturation * 100.0f));
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderInt("##BackgroundSaturation", &SaturationPercent, 0, 100, "%d%%"))
	{
		State.BackgroundSaturation = static_cast<float>(SaturationPercent) / 100.0f;
	}
	ImGui::TextUnformatted("Color intensity");
	int IntensityPercent = static_cast<int>(std::lround(State.BackgroundIntensity * 100.0f));
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderInt("##BackgroundIntensity", &IntensityPercent, 0, 100, "%d%%"))
	{
		State.BackgroundIntensity = static_cast<float>(IntensityPercent) / 100.0f;
	}

	ImGui::Dummy({0.0f, 4.0f * InterfaceScale});
	ImGui::TextUnformatted("Color");
	constexpr ImGuiColorEditFlags ColorEditFlags =
	    ImGuiColorEditFlags_NoAlpha |
	    ImGuiColorEditFlags_PickerHueWheel |
	    ImGuiColorEditFlags_DisplayRGB |
	    ImGuiColorEditFlags_InputRGB |
	    ImGuiColorEditFlags_Uint8;
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::ColorEdit3(
	        State.BackgroundPreset < 0 ? "Custom color (Current)##BackgroundCustomColor" : "Custom color##BackgroundCustomColor",
	        &State.BackgroundColor.x,
	        ColorEditFlags))
	{
		State.BackgroundPreset = -1;
	}
	ImGui::Dummy({0.0f, 2.0f * InterfaceScale});
	if (ImGui::BeginTable("BackgroundPresets", ColumnCount, ImGuiTableFlags_SizingStretchSame))
	{
		for (int PresetIndex = 0; PresetIndex < Theme::Background::PresetCount; ++PresetIndex)
		{
			const Theme::Background::FPreset& Preset = Theme::Background::Presets[PresetIndex];
			const bool bSelected = PresetIndex == State.BackgroundPreset;
			char Label[64] = {}; // NOLINT(modernize-avoid-c-arrays) ImGui requires a stable null-terminated label.
			const int WrittenLength = std::snprintf(Label, sizeof(Label), bSelected ? "%s (Current)" : "%s", Preset.Name);
			if (WrittenLength < 0)
			{
				Label[0] = '\0';
			}
			else if (static_cast<std::size_t>(WrittenLength) >= sizeof(Label))
			{
				Label[sizeof(Label) - 1] = '\0';
			}

			ImGui::TableNextColumn();
			ImGui::PushID(PresetIndex);
			if (ImGui::ColorButton(
			        "##Swatch",
			        ImGui::ColorConvertU32ToFloat4(Preset.Color),
			        ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoDragDrop,
			        {20.0f * InterfaceScale, 20.0f * InterfaceScale}))
			{
				State.BackgroundPreset = PresetIndex;
				State.BackgroundColor = ImGui::ColorConvertU32ToFloat4(Preset.Color);
			}
			ImGui::SameLine();
			if (ImGui::Selectable(Label, bSelected, ImGuiSelectableFlags_None, {0.0f, 20.0f * InterfaceScale}))
			{
				State.BackgroundPreset = PresetIndex;
				State.BackgroundColor = ImGui::ColorConvertU32ToFloat4(Preset.Color);
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void DrawLicenseModal(FUiState& State, const FApplicationResources& Resources)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	if (State.bOpenLicenses)
	{
		ImGui::OpenPopup("Open-source licenses");
		State.bOpenLicenses = false;
	}

	ImGui::SetNextWindowSize({720.0f * InterfaceScale, 520.0f * InterfaceScale}, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Open-source licenses", nullptr, ImGuiWindowFlags_NoSavedSettings))
	{
		const ImVec2 LicenseSize = {0.0f, -(ImGui::GetFrameHeightWithSpacing() + 8.0f * InterfaceScale)};
		if (ImGui::BeginTabBar("LicenseTabs"))
		{
			if (ImGui::BeginTabItem("Roboto"))
			{
				ImGui::TextDisabled("SIL Open Font License 1.1");
				ImGui::BeginChild("RobotoLicense", LicenseSize, ImGuiChildFlags_Borders);
				ImGui::TextUnformatted(Resources.RobotoLicense.data(), Resources.RobotoLicense.data() + Resources.RobotoLicense.size());
				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("FreeType"))
			{
				ImGui::TextDisabled("FreeType Project License");
				ImGui::BeginChild("FreeTypeLicense", LicenseSize, ImGuiChildFlags_Borders);
				ImGui::TextUnformatted(Resources.FreeTypeLicense.data(), Resources.FreeTypeLicense.data() + Resources.FreeTypeLicense.size());
				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		if (ImGui::Button("Close"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void DrawWorkspaceToolbar(FUiState& State, const FApplicationFonts& Fonts, const float ToolbarHeight)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::BeginChild(
	    "WorkspaceToolbar",
	    {0.0f, ToolbarHeight},
	    ImGuiChildFlags_None,
	    ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const float FrameHeight = ImGui::GetFrameHeight();
	const float ContentY = std::max(0.0f, (ToolbarHeight - FrameHeight) * 0.5f);
	ImGui::SetCursorPos({16.0f * InterfaceScale, ContentY});
	ImGui::AlignTextToFramePadding();
	ImGui::PushFont(Fonts.Medium, 0.0f);
	ImGui::TextUnformatted("Workspace");
	ImGui::PopFont();
	ImGui::SameLine();
	ImGui::TextDisabled("Native C++23");
	const float MinimumRightX = ImGui::GetCursorPosX() + 24.0f * InterfaceScale;

	constexpr const char* ComponentsLabel = "Components";
	constexpr const char* LicensesLabel = "Licenses";
	constexpr const char* OutputLogLabel = "Output Log";
	const ImGuiStyle& Style = ImGui::GetStyle();
	const bool bShowStatus = ShouldShowToolbarStatus(ImGui::GetWindowWidth(), InterfaceScale);
	const bool bShowOutputLogAction = ImGui::GetWindowWidth() >= 620.0f * InterfaceScale;
	const float StatusWidth = bShowStatus ? ImGui::CalcTextSize(BuildConfiguration).x + ImGui::CalcTextSize(" | x64").x : 0.0f;
	const float OutputLogWidth = bShowOutputLogAction ? ImGui::CalcTextSize(OutputLogLabel).x + Style.FramePadding.x * 2.0f : 0.0f;
	const float ComponentsWidth = ImGui::CalcTextSize(ComponentsLabel).x + Style.FramePadding.x * 2.0f;
	const float LicensesWidth = ImGui::CalcTextSize(LicensesLabel).x + Style.FramePadding.x * 2.0f;
	const float RightGroupWidth = StatusWidth + OutputLogWidth + ComponentsWidth + LicensesWidth + Style.ItemSpacing.x * (1.0f + static_cast<float>(bShowStatus) + static_cast<float>(bShowOutputLogAction));
	const float RightX = ResolveToolbarRightX(ImGui::GetWindowWidth(), MinimumRightX, RightGroupWidth, 16.0f * InterfaceScale);
	ImGui::SetCursorPos({RightX, ContentY});
	if (bShowStatus)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s | x64", BuildConfiguration);
		ImGui::SameLine();
	}

	if (bShowOutputLogAction)
	{
		if (ImGui::Button("Output Log##Toolbar"))
		{
			State.bShowOutputLog = true;
		}
		ImGui::SameLine();
	}
	if (ImGui::Button("Components##Toolbar"))
	{
		State.bShowDemoWindow = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Licenses##Toolbar"))
	{
		State.bOpenLicenses = true;
	}

	ImGui::EndChild();
}

void ExecuteOutputLogCommand(const std::string_view Command, FUiState& State, FOutputLogPanel& OutputLog)
{
	if (Command == "clear")
	{
		OutputLog.Clear(Log::GetBuffer());
		return;
	}

	Log::Info("Command", "> {}", Command);
	if (Command == "help")
	{
		Log::Info("Command", "Commands: clear, components, help, licenses");
	}
	else if (Command == "components")
	{
		State.bShowDemoWindow = true;
		Log::Info("Command", "Opened the Dear ImGui components window");
	}
	else if (Command == "licenses")
	{
		State.bOpenLicenses = true;
		Log::Info("Command", "Opened third-party licenses");
	}
	else
	{
		Log::Warning("Command", "Unknown command '{}'. Type help for available commands", Command);
	}
}

void DrawWorkspace(const FTitleBarLayout& Layout, const FApplicationResources& Resources, FUiState& State, FOutputLogPanel& OutputLog)
{
	const ImGuiIO& IO = ImGui::GetIO();
	const float TitleBarHeight = static_cast<float>(Layout.TitleBarHeight);
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	const float ToolbarHeight = DefaultToolbarHeight * InterfaceScale;
	const ImVec2 WorkspaceSize = {
	    IO.DisplaySize.x,
	    std::max(0.0f, IO.DisplaySize.y - TitleBarHeight)};
	const float DockspaceHeight = std::max(0.0f, WorkspaceSize.y - ToolbarHeight);

	ImGui::SetNextWindowPos({0.0f, TitleBarHeight});
	ImGui::SetNextWindowSize(WorkspaceSize);
	ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
	constexpr ImGuiWindowFlags HostFlags =
	    ImGuiWindowFlags_NoDecoration |
	    ImGuiWindowFlags_NoMove |
	    ImGuiWindowFlags_NoSavedSettings |
	    ImGuiWindowFlags_NoBringToFrontOnFocus |
	    ImGuiWindowFlags_NoNavFocus |
	    ImGuiWindowFlags_NoBackground;

	ImGui::Begin("WorkspaceHost", nullptr, HostFlags);
	ImGui::PopStyleVar(3);
	DrawWorkspaceToolbar(State, Resources.Fonts, ToolbarHeight);
	ImGui::SetCursorPos({0.0f, ToolbarHeight});
	const ImGuiID MainDockspaceId = ImGui::GetID(MainDockspaceName);
	EnsureDefaultDockLayout(MainDockspaceId, {WorkspaceSize.x, DockspaceHeight});
	ImVec4 DockspaceBackground = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
	DockspaceBackground.w = IsPanelTransparent(State.PanelTransparencyMode, true) ? 0.0f : 1.0f;
	ImGui::PushStyleColor(ImGuiCol_WindowBg, DockspaceBackground);
	ImGui::DockSpace(
	    MainDockspaceId,
	    {0.0f, 0.0f},
	    ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar);
	ImGui::PopStyleColor();
	ImGui::End();

	const ImVec2 WelcomeSize = {
	    std::min(880.0f * InterfaceScale, std::max(320.0f, WorkspaceSize.x - 64.0f * InterfaceScale)),
	    std::min(590.0f * InterfaceScale, std::max(300.0f, DockspaceHeight - 64.0f * InterfaceScale))};
	ImGui::SetNextWindowSize(WelcomeSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(
	    {WorkspaceSize.x * 0.5f, TitleBarHeight + ToolbarHeight + DockspaceHeight * 0.5f},
	    ImGuiCond_FirstUseEver,
	    {0.5f, 0.5f});
	SetNextPanelTransparency(State.PanelTransparencyMode, State.bStartPanelDocked);
	ImGui::Begin("Start");
	State.bStartPanelDocked = ImGui::IsWindowDocked();
	DrawWorkspaceIntro(State, Resources.Fonts);

	if (ImGui::GetContentRegionAvail().x >= 720.0f * InterfaceScale &&
	    ImGui::BeginTable("StarterCards", 2, ImGuiTableFlags_SizingStretchSame))
	{
		ImGui::TableNextColumn();
		DrawProjectCard(State, Resources.Fonts);
		ImGui::TableNextColumn();
		DrawBuildProfileCard(Resources.Fonts);
		ImGui::EndTable();
	}
	else
	{
		DrawProjectCard(State, Resources.Fonts);
		ImGui::Dummy({0.0f, 8.0f * InterfaceScale});
		DrawBuildProfileCard(Resources.Fonts);
	}

	ImGui::Dummy({0.0f, 8.0f * InterfaceScale});
	DrawAppearanceCard(State, Resources.Fonts);
	ImGui::Dummy({0.0f, 4.0f * InterfaceScale});
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::Colors::Accent), "●");
	ImGui::SameLine();
	ImGui::TextUnformatted("Windows and Linux workspace ready");
	ImGui::SameLine();
	ImGui::TextDisabled("  |  Roboto  |  OpenGL 3.3");
	ImGui::End();

	if (State.bShowOutputLog)
	{
		SetNextPanelTransparency(State.PanelTransparencyMode, State.bOutputLogDocked);
		const std::optional<std::string> Command = OutputLog.Draw(Log::GetBuffer(), &State.bShowOutputLog, State.bOutputLogDocked, OutputLogCommands);
		if (Command)
		{
			ExecuteOutputLogCommand(*Command, State, OutputLog);
		}
	}

	if (State.bShowDemoWindow)
	{
		SetNextPanelTransparency(State.PanelTransparencyMode, State.bDemoPanelDocked);
		ImGui::ShowDemoWindow(&State.bShowDemoWindow);
		// The demo owns its Begin/End pair, so its dock state is only available through the pinned ImGui internals.
		if (const ImGuiWindow* DemoWindow = ImGui::FindWindowByName("Dear ImGui Demo"))
		{
			State.bDemoPanelDocked = DemoWindow->DockIsActive;
		}
	}
	DrawLicenseModal(State, Resources);
}

void RenderApplicationFrame(GLFWwindow* const Window, FApplicationRuntime& Runtime)
{
	if (!Runtime.bRendererReady || Runtime.bRenderingFrame || glfwGetWindowAttrib(Window, GLFW_ICONIFIED) == GLFW_TRUE)
	{
		return;
	}

	// Refresh callbacks can arrive from GLFW calls made while rendering, so recursive frames must be ignored.
	Runtime.bRenderingFrame = true;
	const float ContentScale = Runtime.Window.UiScale;
	if (std::abs(ContentScale - Runtime.PreviousContentScale) > 0.001f)
	{
		ImGui::GetStyle() = Runtime.BaseStyle;
		ImGui::GetStyle().ScaleAllSizes(ContentScale);
		glfwSetWindowSizeLimits(
		    Window,
		    ScaleTitleBarMetric(480, ContentScale),
		    ScaleTitleBarMetric(320, ContentScale),
		    GLFW_DONT_CARE,
		    GLFW_DONT_CARE);
		Runtime.PreviousContentScale = ContentScale;
	}
	Theme::ApplyInteractiveColors(ImGui::GetStyle(), ResolveBackgroundAccent(Runtime.Ui));

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	DrawTitleBar(Runtime.Window, Runtime.Resources->Fonts);
	DrawWorkspace(Runtime.Window.TitleBarHitTest.Layout, *Runtime.Resources, Runtime.Ui, Runtime.OutputLog);
	DrawApplicationMenu(Window, Runtime.Window.TitleBarHitTest.Layout, Runtime.Ui);
	UpdateTitleBarUiCaptureRegions(Runtime.Window);

	ImGui::Render();
	int FramebufferWidth = 0;
	int FramebufferHeight = 0;
	glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
	glViewport(0, 0, FramebufferWidth, FramebufferHeight);
	glClearColor(18.0f / 255.0f, 18.0f / 255.0f, 19.0f / 255.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	Runtime.BackgroundRenderer.Render(
	    FramebufferWidth,
	    FramebufferHeight,
	    ResolveApplicationBackground(Runtime.Ui, Runtime.Window.bFocused));
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(Window);
	Runtime.bRenderingFrame = false;
}

void WindowRefreshCallback(GLFWwindow* const Window)
{
	if (FApplicationRuntime* const Runtime = GetApplicationRuntime(Window))
	{
		RenderApplicationFrame(Window, *Runtime);
	}
}

}

int RunApplication(const std::filesystem::path& ExecutablePath, const bool bSmokeTest, const EWindowPlatform WindowPlatform)
{
	Log::GetBuffer().Clear();
	Log::Info("Application", "Starting {} in {} configuration", ApplicationTitle, BuildConfiguration);
	std::error_code WorkingDirectoryError;
	const std::filesystem::path WorkingDirectory = std::filesystem::current_path(WorkingDirectoryError);
	if (WorkingDirectoryError)
	{
		Log::Warning("Filesystem", "Could not resolve the working directory: {}", WorkingDirectoryError.message());
	}

	const FApplicationPaths Paths = ResolveApplicationPaths(ExecutablePath, WorkingDirectory);
	FAssetProvider AssetProvider(Paths.AssetDirectory);
	const std::string ImGuiIniPath = (Paths.SavedDirectory / "ImGui.ini").string();
	const std::string ImGuiLogPath = (Paths.SavedDirectory / "ImGuiLog.txt").string();
	glfwSetErrorCallback(GlfwErrorCallback);

#ifdef __linux__
	if (WindowPlatform == EWindowPlatform::X11)
	{
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
	}
	else if (WindowPlatform == EWindowPlatform::Wayland)
	{
		glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
	}
#else
	(void)WindowPlatform;
#endif

	if (glfwInit() != GLFW_TRUE)
	{
		Log::Error("GLFW", "Initialization failed");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_TITLEBAR, GLFW_FALSE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	FWindowPlacement InitialWindowPlacement = {0, 0, DefaultWindowWidth, DefaultWindowHeight};
	bool bHasMonitorWorkArea = false;
	if (GLFWmonitor* const PrimaryMonitor = glfwGetPrimaryMonitor())
	{
		int WorkAreaX = 0;
		int WorkAreaY = 0;
		int WorkAreaWidth = 0;
		int WorkAreaHeight = 0;
		glfwGetMonitorWorkarea(PrimaryMonitor, &WorkAreaX, &WorkAreaY, &WorkAreaWidth, &WorkAreaHeight);
		if (WorkAreaWidth > 0 && WorkAreaHeight > 0)
		{
			InitialWindowPlacement = ResolveCenteredWindowPlacement(
			    WorkAreaX,
			    WorkAreaY,
			    WorkAreaWidth,
			    WorkAreaHeight,
			    InitialWindowWorkAreaPercent);
			bHasMonitorWorkArea = true;
		}
	}

	GLFWwindow* const Window = glfwCreateWindow(InitialWindowPlacement.Width, InitialWindowPlacement.Height, ApplicationTitle, nullptr, nullptr);
	if (Window == nullptr)
	{
		Log::Error("Window", "Could not create the main window");
		glfwTerminate();
		return 1;
	}

	if (bHasMonitorWorkArea && glfwGetPlatform() != GLFW_PLATFORM_WAYLAND)
	{
		glfwSetWindowPos(Window, InitialWindowPlacement.X, InitialWindowPlacement.Y);
	}

	FApplicationRuntime Runtime;
	FWindowState& WindowState = Runtime.Window;
	WindowState.bWayland = glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
	glfwGetWindowSize(
	    Window,
	    &WindowState.TitleBarHitTest.Layout.WindowWidth,
	    &WindowState.TitleBarHitTest.Layout.WindowHeight);
	WindowState.TitleBarHitTest.Layout.bResizable = glfwGetWindowAttrib(Window, GLFW_RESIZABLE) == GLFW_TRUE;
	WindowState.TitleBarHitTest.Layout.bMaximized = glfwGetWindowAttrib(Window, GLFW_MAXIMIZED) == GLFW_TRUE;
	WindowState.bFocused = glfwGetWindowAttrib(Window, GLFW_FOCUSED) == GLFW_TRUE;
	WindowState.bCursorInside = glfwGetWindowAttrib(Window, GLFW_HOVERED) == GLFW_TRUE;
	glfwGetCursorPos(Window, &WindowState.CursorX, &WindowState.CursorY);
	float InitialContentScale = 1.0f;
	glfwGetWindowContentScale(Window, nullptr, &InitialContentScale);
	UpdateTitleBarScale(WindowState, InitialContentScale);

	glfwSetWindowUserPointer(Window, &Runtime);
	glfwSetWindowSizeCallback(Window, WindowSizeCallback);
	glfwSetWindowContentScaleCallback(Window, WindowContentScaleCallback);
	glfwSetWindowMaximizeCallback(Window, WindowMaximizeCallback);
	glfwSetWindowFocusCallback(Window, WindowFocusCallback);
	glfwSetCursorEnterCallback(Window, CursorEnterCallback);
	glfwSetCursorPosCallback(Window, CursorPositionCallback);
	glfwSetMouseButtonCallback(Window, SynchronizeMousePositionBeforeButtonCallback);
	glfwSetWindowSizeLimits(
	    Window,
	    ScaleTitleBarMetric(480, WindowState.UiScale),
	    ScaleTitleBarMetric(320, WindowState.UiScale),
	    GLFW_DONT_CARE,
	    GLFW_DONT_CARE);
	glfwSetWindowHitTestCallback(Window, TitleBarHitTestCallback);
	glfwMakeContextCurrent(Window);
	glfwSwapInterval(1);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& IO = ImGui::GetIO();
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	IO.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
	IO.ConfigDpiScaleFonts = true;
	IO.IniFilename = ImGuiIniPath.c_str();
	IO.LogFilename = ImGuiLogPath.c_str();
	ImGui::StyleColorsDark();
	Theme::ApplyApplicationTheme(ImGui::GetStyle());
	const std::optional<FApplicationResources> Resources = LoadApplicationResources(AssetProvider, IO);
	if (!Resources)
	{
		ImGui::DestroyContext();
		glfwDestroyWindow(Window);
		glfwTerminate();
		return 1;
	}

	Runtime.BaseStyle = ImGui::GetStyle();
	Runtime.Resources = &*Resources;

	std::error_code DirectoryError;
	std::filesystem::create_directories(Paths.SavedDirectory, DirectoryError);
	if (DirectoryError)
	{
		Log::Warning("Filesystem", "Could not create Saved directory: {}", DirectoryError.message());
	}

	const bool bGlfwBackendInitialized = ImGui_ImplGlfw_InitForOpenGL(Window, true);
	const bool bOpenGlBackendInitialized = bGlfwBackendInitialized && ImGui_ImplOpenGL3_Init("#version 330");
	if (!bOpenGlBackendInitialized)
	{
		Log::Error("Renderer", "Could not initialize the Dear ImGui OpenGL backend");
		if (bGlfwBackendInitialized)
		{
			ImGui_ImplGlfw_Shutdown();
		}
		ImGui::DestroyContext();
		glfwDestroyWindow(Window);
		glfwTerminate();
		return 1;
	}
	const std::expected<void, std::string> BackgroundRendererInitialized = Runtime.BackgroundRenderer.Initialize();
	if (!BackgroundRendererInitialized)
	{
		Log::Error("Renderer", "Could not initialize the background renderer: {}", BackgroundRendererInitialized.error());
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(Window);
		glfwTerminate();
		return 1;
	}

	Log::Info("Window", "Created a {} x {} main window", WindowState.TitleBarHitTest.Layout.WindowWidth, WindowState.TitleBarHitTest.Layout.WindowHeight);
	Log::Info("Renderer", "OpenGL 3.3 rendering initialized");
	Log::Info("UI", "Dear ImGui docking and custom title bar initialized");
	Log::Info("Application", "Workspace ready. Type help in the Output Log for commands");
	Runtime.bRendererReady = true;
	glfwSetWindowRefreshCallback(Window, WindowRefreshCallback);
	glfwShowWindow(Window);
	int FrameCount = 0;
	while (glfwWindowShouldClose(Window) == GLFW_FALSE)
	{
		if (glfwGetWindowAttrib(Window, GLFW_ICONIFIED) == GLFW_TRUE)
		{
			glfwWaitEvents();
			continue;
		}
		glfwPollEvents();
		RenderApplicationFrame(Window, Runtime);

		FrameCount++;
		if (bSmokeTest && FrameCount >= 3)
		{
			glfwSetWindowShouldClose(Window, GLFW_TRUE);
		}
	}

	Runtime.bRendererReady = false;
	glfwSetWindowRefreshCallback(Window, nullptr);
	Runtime.BackgroundRenderer.Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;
}
}
