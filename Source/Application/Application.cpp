#include "Application/Application.h"

#include "Assets/AssetProvider.h"
#include "UI/ApplicationTheme.h"
#include "UI/TitleBarLayout.h"

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
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

struct FWindowState
{
	FTitleBarLayout TitleBar;
	float UiScale = 1.0f;
	double CursorX = 0.0;
	double CursorY = 0.0;
	bool bFocused = true;
	bool bCursorInside = false;
	bool bUiCapturesMouse = false;
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
	std::string RobotoLicense;
};

struct FUiState
{
	char ApplicationName[64] = "Native App";
	bool bShowDemoWindow = false;
	bool bOpenLicenses = false;
};

[[nodiscard]] ImFont* LoadFont(
	FAssetProvider& AssetProvider,
	ImGuiIO& IO,
	const std::string_view VirtualPath)
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

[[nodiscard]] std::optional<FApplicationResources> LoadApplicationResources(FAssetProvider& AssetProvider, ImGuiIO& IO)
{
	FApplicationResources Resources;
	Resources.Fonts.Regular = LoadFont(AssetProvider, IO, "Fonts/Roboto/Roboto-Regular.ttf");
	Resources.Fonts.Medium = LoadFont(AssetProvider, IO, "Fonts/Roboto/Roboto-Medium.ttf");
	if (Resources.Fonts.Regular == nullptr || Resources.Fonts.Medium == nullptr)
	{
		return std::nullopt;
	}

	// Roboto is redistributed with the app, so its OFL text must remain available through the license viewer.
	const std::expected<std::span<const std::byte>, FAssetLoadError> LicenseData = AssetProvider.Load("Fonts/Roboto/OFL.txt");
	if (!LicenseData)
	{
		std::println(stderr, "Could not load Roboto license: {}", LicenseData.error().Message);
		return std::nullopt;
	}

	Resources.RobotoLicense.reserve(LicenseData->size());
	for (const std::byte Byte : *LicenseData)
	{
		Resources.RobotoLicense.push_back(static_cast<char>(std::to_integer<unsigned char>(Byte)));
	}

	IO.FontDefault = Resources.Fonts.Regular;
	return Resources;
}

void GlfwErrorCallback(const int Error, const char* const Description)
{
	std::println(stderr, "GLFW error {}: {}", Error, Description);
}

[[nodiscard]] FWindowState* GetWindowState(GLFWwindow* const Window)
{
	return static_cast<FWindowState*>(glfwGetWindowUserPointer(Window));
}

void UpdateTitleBarScale(FWindowState& State, const float ContentScale)
{
	// Wayland window and cursor coordinates are already logical units.
	State.UiScale = ResolveTitleBarUiScale(State.bWayland, ContentScale);
	State.TitleBar.TitleBarHeight = ScaleTitleBarMetric(40, State.UiScale);
	State.TitleBar.ButtonWidth = ScaleTitleBarMetric(46, State.UiScale);
	State.TitleBar.ResizeBorder = ScaleTitleBarMetric(6, State.UiScale);
}

void WindowSizeCallback(GLFWwindow* const Window, const int Width, const int Height)
{
	if (FWindowState* const State = GetWindowState(Window))
	{
		State->TitleBar.WindowWidth = Width;
		State->TitleBar.WindowHeight = Height;
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
		State->TitleBar.bMaximized = bMaximized == GLFW_TRUE;
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
	case ETitleBarHitRegion::Client: return GLFW_HIT_TEST_CLIENT;
	case ETitleBarHitRegion::Caption: return GLFW_HIT_TEST_CAPTION;
	case ETitleBarHitRegion::ResizeLeft: return GLFW_HIT_TEST_RESIZE_LEFT;
	case ETitleBarHitRegion::ResizeRight: return GLFW_HIT_TEST_RESIZE_RIGHT;
	case ETitleBarHitRegion::ResizeTop: return GLFW_HIT_TEST_RESIZE_TOP;
	case ETitleBarHitRegion::ResizeBottom: return GLFW_HIT_TEST_RESIZE_BOTTOM;
	case ETitleBarHitRegion::ResizeTopLeft: return GLFW_HIT_TEST_RESIZE_TOP_LEFT;
	case ETitleBarHitRegion::ResizeTopRight: return GLFW_HIT_TEST_RESIZE_TOP_RIGHT;
	case ETitleBarHitRegion::ResizeBottomLeft: return GLFW_HIT_TEST_RESIZE_BOTTOM_LEFT;
	case ETitleBarHitRegion::ResizeBottomRight: return GLFW_HIT_TEST_RESIZE_BOTTOM_RIGHT;
	case ETitleBarHitRegion::SystemMenu: return GLFW_HIT_TEST_SYSTEM_MENU;
	case ETitleBarHitRegion::MinimizeButton: return GLFW_HIT_TEST_MINIMIZE_BUTTON;
	case ETitleBarHitRegion::MaximizeButton: return GLFW_HIT_TEST_MAXIMIZE_BUTTON;
	case ETitleBarHitRegion::CloseButton: return GLFW_HIT_TEST_CLOSE_BUTTON;
	}

	return GLFW_HIT_TEST_CLIENT;
}

int TitleBarHitTestCallback(GLFWwindow* const Window, const int X, const int Y)
{
	// Native hit testing may run while GLFW dispatches events, so it uses capture state cached by the last ImGui frame.
	const FWindowState* const State = GetWindowState(Window);
	return State == nullptr
		? GLFW_HIT_TEST_CLIENT
		: ToGlfwHitTest(HitTestTitleBar(State->TitleBar, X, Y, State->bUiCapturesMouse));
}

[[nodiscard]] ETitleBarHitRegion GetHoveredRegion(const FWindowState& State)
{
	if (!State.bCursorInside)
	{
		return ETitleBarHitRegion::Client;
	}

	return HitTestTitleBar(
		State.TitleBar,
		static_cast<int>(State.CursorX),
		static_cast<int>(State.CursorY),
		State.bUiCapturesMouse);
}

void DrawSystemButtonBackground(
	ImDrawList& DrawList,
	const float Left,
	const float Top,
	const float Width,
	const float Height,
	const bool bHovered,
	const bool bCloseButton)
{
	if (!bHovered)
	{
		return;
	}

	const ImU32 Color = bCloseButton ? Theme::Colors::CloseHover : Theme::Colors::SurfaceHover;
	DrawList.AddRectFilled({ Left, Top }, { Left + Width, Top + Height }, Color);
}

void DrawTitleBar(const FWindowState& State, const FApplicationFonts& Fonts)
{
	const FTitleBarLayout& Layout = State.TitleBar;
	ImDrawList& DrawList = *ImGui::GetBackgroundDrawList();
	const float Width = static_cast<float>(Layout.WindowWidth);
	const float Height = static_cast<float>(Layout.TitleBarHeight);
	const float ButtonWidth = static_cast<float>(Layout.ButtonWidth);
	const float Scale = Height / 40.0f;
	const ETitleBarHitRegion HoveredRegion = GetHoveredRegion(State);

	DrawList.AddRectFilled(
		{ 0.0f, 0.0f },
		{ Width, Height },
		State.bFocused ? Theme::Colors::Canvas : Theme::Colors::Surface1);
	DrawList.AddLine(
		{ 0.0f, Height - 1.0f },
		{ Width, Height - 1.0f },
		Theme::Colors::BorderSoft);

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
		{ MinimizeCenterX - 5.0f * Scale, CenterY + 3.0f * Scale },
		{ MinimizeCenterX + 5.0f * Scale, CenterY + 3.0f * Scale },
		GlyphColor,
		LineThickness);

	const float MaximizeCenterX = Width - ButtonWidth * 1.5f;
	if (Layout.bMaximized)
	{
		DrawList.AddRect(
			{ MaximizeCenterX - 3.0f * Scale, CenterY - 5.0f * Scale },
			{ MaximizeCenterX + 5.0f * Scale, CenterY + 3.0f * Scale },
			GlyphColor,
			0.0f,
			0,
			LineThickness);
		DrawList.AddRect(
			{ MaximizeCenterX - 5.0f * Scale, CenterY - 3.0f * Scale },
			{ MaximizeCenterX + 3.0f * Scale, CenterY + 5.0f * Scale },
			GlyphColor,
			0.0f,
			0,
			LineThickness);
	}
	else
	{
		DrawList.AddRect(
			{ MaximizeCenterX - 5.0f * Scale, CenterY - 5.0f * Scale },
			{ MaximizeCenterX + 5.0f * Scale, CenterY + 5.0f * Scale },
			GlyphColor,
			0.0f,
			0,
			LineThickness);
	}

	const float CloseCenterX = Width - ButtonWidth * 0.5f;
	DrawList.AddLine(
		{ CloseCenterX - 5.0f * Scale, CenterY - 5.0f * Scale },
		{ CloseCenterX + 5.0f * Scale, CenterY + 5.0f * Scale },
		GlyphColor,
		LineThickness);
	DrawList.AddLine(
		{ CloseCenterX + 5.0f * Scale, CenterY - 5.0f * Scale },
		{ CloseCenterX - 5.0f * Scale, CenterY + 5.0f * Scale },
		GlyphColor,
		LineThickness);

	const float IconCenter = Height * 0.5f;
	if (HoveredRegion == ETitleBarHitRegion::SystemMenu)
	{
		DrawList.AddRectFilled({ 0.0f, 0.0f }, { Height, Height }, Theme::Colors::SurfaceHover);
	}
	DrawList.AddCircleFilled({ IconCenter, IconCenter }, 8.0f * Scale, Theme::Colors::Accent);
	DrawList.AddCircle({ IconCenter, IconCenter }, 4.0f * Scale, Theme::Colors::TextPrimary, 0, LineThickness);

	ImGui::PushFont(Fonts.Medium, 0.0f);
	const ImVec2 TextSize = ImGui::CalcTextSize(ApplicationTitle);
	DrawList.AddText(
		{ (Width - TextSize.x) * 0.5f, (Height - TextSize.y) * 0.5f },
		State.bFocused ? Theme::Colors::TextPrimary : Theme::Colors::TextMuted,
		ApplicationTitle);
	ImGui::PopFont();
}

[[nodiscard]] bool DrawPrimaryButton(const char* const Label, const FApplicationFonts& Fonts)
{
	ImGui::PushFont(Fonts.Medium, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 100.0f);
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255));
	ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 255, 255, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(232, 232, 232, 255));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(210, 210, 210, 255));
	const bool bPressed = ImGui::Button(Label);
	ImGui::PopStyleColor(4);
	ImGui::PopStyleVar();
	ImGui::PopFont();
	return bPressed;
}

void DrawFeatureCard(FUiState& State, const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 16.0f * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 24.0f * InterfaceScale, 22.0f * InterfaceScale });
	ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::Colors::GradientViolet);
	ImGui::BeginChild(
		"FeatureCard",
		{ 0.0f, 192.0f * InterfaceScale },
		ImGuiChildFlags_AlwaysUseWindowPadding,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	ImDrawList& DrawList = *ImGui::GetWindowDrawList();
	const ImVec2 CardMin = ImGui::GetWindowPos();
	const ImVec2 CardSize = ImGui::GetWindowSize();
	const ImVec2 CardMax = { CardMin.x + CardSize.x, CardMin.y + CardSize.y };
	DrawList.PushClipRect(CardMin, CardMax, true);
	DrawList.AddCircleFilled(
		{ CardMax.x - 120.0f * InterfaceScale, CardMin.y + 90.0f * InterfaceScale },
		110.0f * InterfaceScale,
		Theme::Colors::GradientMagenta,
		64);
	DrawList.AddCircleFilled(
		{ CardMin.x + 110.0f * InterfaceScale, CardMax.y + 110.0f * InterfaceScale },
		140.0f * InterfaceScale,
		Theme::Colors::GradientCoral,
		64);
	DrawList.PopClipRect();

	ImGui::PushFont(Fonts.Medium, 13.0f);
	ImGui::TextUnformatted("PROJECTTEMPLATE / NATIVE C++23");
	ImGui::PopFont();
	ImGui::Dummy({ 0.0f, 4.0f * InterfaceScale });
	ImGui::PushFont(Fonts.Medium, 28.0f);
	ImGui::TextUnformatted("Build something native.");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(242, 238, 255, 230));
	ImGui::TextWrapped("A focused Windows and Linux starting point with modern C++, native window behavior, and a UI ready to shape.");
	ImGui::PopStyleColor();
	ImGui::Dummy({ 0.0f, 8.0f * InterfaceScale });

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
	ImGui::PopStyleColor();
	ImGui::PopStyleVar(2);
}

void DrawProjectCard(FUiState& State, const FApplicationFonts& Fonts)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 16.0f * InterfaceScale, 16.0f * InterfaceScale });
	ImGui::BeginChild(
		"ProjectCard",
		{ 0.0f, 180.0f * InterfaceScale },
		ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	ImGui::PushFont(Fonts.Medium, 16.0f);
	ImGui::TextUnformatted("Template identity");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("Give the derived application a working name before replacing the template identifiers.");
	ImGui::PopStyleColor();
	ImGui::Dummy({ 0.0f, 4.0f * InterfaceScale });
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
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f * InterfaceScale);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 16.0f * InterfaceScale, 16.0f * InterfaceScale });
	ImGui::BeginChild(
		"BuildProfileCard",
		{ 0.0f, 180.0f * InterfaceScale },
		ImGuiChildFlags_Borders | ImGuiChildFlags_AlwaysUseWindowPadding);

	ImGui::PushFont(Fonts.Medium, 16.0f);
	ImGui::TextUnformatted("Build profile");
	ImGui::PopFont();
	ImGui::PushStyleColor(ImGuiCol_Text, Theme::Colors::TextSecondary);
	ImGui::TextWrapped("Production-shaped defaults without framework weight.");
	ImGui::PopStyleColor();
	ImGui::Dummy({ 0.0f, 4.0f * InterfaceScale });

	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 0.0f, 3.0f * InterfaceScale });
	if (ImGui::BeginTable("BuildProfile", 2, ImGuiTableFlags_SizingStretchProp))
	{
		constexpr const char* Rows[][2] = {
			{ "Language", "C++23" },
			{ "Window", "GLFW native title bar" },
			{ "Interface", "Dear ImGui docking" },
			{ "Shipping", "Embedded assets" }
		};
		for (const auto& Row : Rows)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", Row[0]);
			ImGui::TableNextColumn();
			ImGui::TextWrapped("%s", Row[1]);
		}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();

	ImGui::EndChild();
	ImGui::PopStyleVar(2);
}

void DrawLicenseModal(FUiState& State, const std::string& RobotoLicense)
{
	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	if (State.bOpenLicenses)
	{
		ImGui::OpenPopup("Open-source licenses");
		State.bOpenLicenses = false;
	}

	ImGui::SetNextWindowSize({ 720.0f * InterfaceScale, 520.0f * InterfaceScale }, ImGuiCond_Appearing);
	if (ImGui::BeginPopupModal("Open-source licenses", nullptr, ImGuiWindowFlags_NoSavedSettings))
	{
		ImGui::TextUnformatted("Roboto");
		ImGui::SameLine();
		ImGui::TextDisabled("SIL Open Font License 1.1");
		ImGui::Separator();
		ImGui::BeginChild(
			"RobotoLicense",
			{ 0.0f, -(ImGui::GetFrameHeightWithSpacing() + 8.0f * InterfaceScale) },
			ImGuiChildFlags_Borders);
		ImGui::TextUnformatted(RobotoLicense.data(), RobotoLicense.data() + RobotoLicense.size());
		ImGui::EndChild();
		if (ImGui::Button("Close"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void DrawWorkspace(const FTitleBarLayout& Layout, const FApplicationResources& Resources, FUiState& State)
{
	const ImGuiIO& IO = ImGui::GetIO();
	const float TitleBarHeight = static_cast<float>(Layout.TitleBarHeight);
	const ImVec2 WorkspaceSize = {
		IO.DisplaySize.x,
		std::max(0.0f, IO.DisplaySize.y - TitleBarHeight)
	};

	ImGui::SetNextWindowPos({ 0.0f, TitleBarHeight });
	ImGui::SetNextWindowSize(WorkspaceSize);
	ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
	constexpr ImGuiWindowFlags HostFlags =
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoBringToFrontOnFocus |
		ImGuiWindowFlags_NoNavFocus;

	ImGui::Begin("WorkspaceHost", nullptr, HostFlags);
	ImGui::PopStyleVar(3);
	const ImGuiID MainDockspaceId = ImGui::GetID("MainDockspace");
	ImGui::DockSpace(
		MainDockspaceId,
		{ 0.0f, 0.0f },
		ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	const ImVec2 WelcomeSize = {
		std::min(880.0f * InterfaceScale, std::max(320.0f, WorkspaceSize.x - 64.0f * InterfaceScale)),
		std::min(590.0f * InterfaceScale, std::max(300.0f, WorkspaceSize.y - 64.0f * InterfaceScale))
	};
	ImGui::SetNextWindowSize(WelcomeSize, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowDockID(MainDockspaceId, ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(
		{ WorkspaceSize.x * 0.5f, TitleBarHeight + WorkspaceSize.y * 0.5f },
		ImGuiCond_FirstUseEver,
		{ 0.5f, 0.5f });
	ImGui::Begin("Start");
	DrawFeatureCard(State, Resources.Fonts);
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
		ImGui::Dummy({ 0.0f, 8.0f * InterfaceScale });
		DrawBuildProfileCard(Resources.Fonts);
	}
	ImGui::Dummy({ 0.0f, 4.0f * InterfaceScale });
	ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(Theme::Colors::Accent), "●");
	ImGui::SameLine();
	ImGui::TextUnformatted("Windows and Linux workspace ready");
	ImGui::SameLine();
	ImGui::TextDisabled("  |  Roboto  |  OpenGL 3.3");
	ImGui::End();

	if (State.bShowDemoWindow)
	{
		ImGui::ShowDemoWindow(&State.bShowDemoWindow);
	}
	DrawLicenseModal(State, Resources.RobotoLicense);
}
}

int RunApplication(const bool bSmokeTest, const EWindowPlatform WindowPlatform)
{
	FAssetProvider AssetProvider("Assets");
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
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SCALE_FRAMEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_TITLEBAR, GLFW_FALSE);

	GLFWwindow* const Window = glfwCreateWindow(1280, 720, ApplicationTitle, nullptr, nullptr);
	if (Window == nullptr)
	{
		glfwTerminate();
		return 1;
	}

	FWindowState WindowState;
	WindowState.bWayland = glfwGetPlatform() == GLFW_PLATFORM_WAYLAND;
	glfwGetWindowSize(
		Window,
		&WindowState.TitleBar.WindowWidth,
		&WindowState.TitleBar.WindowHeight);
	WindowState.TitleBar.bResizable = glfwGetWindowAttrib(Window, GLFW_RESIZABLE) == GLFW_TRUE;
	WindowState.TitleBar.bMaximized = glfwGetWindowAttrib(Window, GLFW_MAXIMIZED) == GLFW_TRUE;
	WindowState.bFocused = glfwGetWindowAttrib(Window, GLFW_FOCUSED) == GLFW_TRUE;
	WindowState.bCursorInside = glfwGetWindowAttrib(Window, GLFW_HOVERED) == GLFW_TRUE;
	glfwGetCursorPos(Window, &WindowState.CursorX, &WindowState.CursorY);
	float InitialContentScale = 1.0f;
	glfwGetWindowContentScale(Window, nullptr, &InitialContentScale);
	UpdateTitleBarScale(WindowState, InitialContentScale);

	glfwSetWindowUserPointer(Window, &WindowState);
	glfwSetWindowSizeCallback(Window, WindowSizeCallback);
	glfwSetWindowContentScaleCallback(Window, WindowContentScaleCallback);
	glfwSetWindowMaximizeCallback(Window, WindowMaximizeCallback);
	glfwSetWindowFocusCallback(Window, WindowFocusCallback);
	glfwSetCursorEnterCallback(Window, CursorEnterCallback);
	glfwSetCursorPosCallback(Window, CursorPositionCallback);
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
	IO.IniFilename = "Saved/ImGui.ini";
	IO.LogFilename = "Saved/ImGuiLog.txt";
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

	const ImGuiStyle BaseStyle = ImGui::GetStyle();

	std::error_code DirectoryError;
	std::filesystem::create_directories("Saved", DirectoryError);
	if (DirectoryError)
	{
		std::println(stderr, "Could not create Saved directory: {}", DirectoryError.message());
	}

	const bool bGlfwBackendInitialized = ImGui_ImplGlfw_InitForOpenGL(Window, true);
	const bool bOpenGlBackendInitialized = bGlfwBackendInitialized && ImGui_ImplOpenGL3_Init("#version 330");
	if (!bOpenGlBackendInitialized)
	{
		if (bGlfwBackendInitialized) ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		glfwDestroyWindow(Window);
		glfwTerminate();
		return 1;
	}

	FUiState UiState;
	float PreviousContentScale = 0.0f;
	int FrameCount = 0;
	while (glfwWindowShouldClose(Window) == GLFW_FALSE)
	{
		glfwPollEvents();

		const float ContentScale = WindowState.UiScale;
		if (std::abs(ContentScale - PreviousContentScale) > 0.001f)
		{
			ImGui::GetStyle() = BaseStyle;
			ImGui::GetStyle().ScaleAllSizes(ContentScale);
			glfwSetWindowSizeLimits(
				Window,
				ScaleTitleBarMetric(480, ContentScale),
				ScaleTitleBarMetric(320, ContentScale),
				GLFW_DONT_CARE,
				GLFW_DONT_CARE);
			PreviousContentScale = ContentScale;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		WindowState.bUiCapturesMouse = IO.WantCaptureMouse;

		DrawTitleBar(WindowState, Resources->Fonts);
		DrawWorkspace(WindowState.TitleBar, *Resources, UiState);

		ImGui::Render();
		int FramebufferWidth = 0;
		int FramebufferHeight = 0;
		glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
		glViewport(0, 0, FramebufferWidth, FramebufferHeight);
		glClearColor(0.035f, 0.035f, 0.035f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(Window);

		FrameCount++;
		if (bSmokeTest && FrameCount >= 3)
		{
			glfwSetWindowShouldClose(Window, GLFW_TRUE);
		}
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(Window);
	glfwTerminate();
	return 0;
}
}
