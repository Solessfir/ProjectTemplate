#include "Application/Application.h"

#include "UI/TitleBarLayout.h"

#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <print>
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
	bool bWayland = false;
};

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
	// Native hit testing must not query ImGui or make synchronous window-system calls.
	const FWindowState* const State = GetWindowState(Window);
	return State == nullptr
		? GLFW_HIT_TEST_CLIENT
		: ToGlfwHitTest(HitTestTitleBar(State->TitleBar, X, Y));
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
		static_cast<int>(State.CursorY));
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

	const ImU32 Color = bCloseButton ? IM_COL32(196, 43, 28, 255) : IM_COL32(65, 68, 77, 255);
	DrawList.AddRectFilled({ Left, Top }, { Left + Width, Top + Height }, Color);
}

void DrawTitleBar(const FWindowState& State)
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
		State.bFocused ? IM_COL32(28, 30, 36, 255) : IM_COL32(38, 40, 46, 255));

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

	constexpr ImU32 GlyphColor = IM_COL32(230, 232, 236, 255);
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
		DrawList.AddRectFilled({ 0.0f, 0.0f }, { Height, Height }, IM_COL32(65, 68, 77, 255));
	}
	DrawList.AddCircleFilled({ IconCenter, IconCenter }, 8.0f * Scale, IM_COL32(118, 91, 255, 255));
	DrawList.AddCircle({ IconCenter, IconCenter }, 4.0f * Scale, IM_COL32(232, 228, 255, 255), 0, LineThickness);

	const ImVec2 TextSize = ImGui::CalcTextSize(ApplicationTitle);
	DrawList.AddText(
		{ (Width - TextSize.x) * 0.5f, (Height - TextSize.y) * 0.5f },
		State.bFocused ? IM_COL32(236, 237, 240, 255) : IM_COL32(175, 177, 184, 255),
		ApplicationTitle);
}

void DrawWorkspace(const FTitleBarLayout& Layout, bool& bShowDemoWindow)
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
	ImGui::DockSpace(
		ImGui::GetID("MainDockspace"),
		{ 0.0f, 0.0f },
		ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	ImGui::Begin("Welcome");
	ImGui::TextUnformatted("C++23 native application starter");
	ImGui::Separator();
	ImGui::TextUnformatted("GLFW custom title bar + Dear ImGui docking + OpenGL 3.3");
	ImGui::Checkbox("Show Dear ImGui demo", &bShowDemoWindow);
	ImGui::End();

	if (bShowDemoWindow)
	{
		ImGui::ShowDemoWindow(&bShowDemoWindow);
	}
}
}

int RunApplication(const bool bSmokeTest, const EWindowPlatform WindowPlatform)
{
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

	bool bShowDemoWindow = false;
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

		DrawTitleBar(WindowState);
		DrawWorkspace(WindowState.TitleBar, bShowDemoWindow);

		ImGui::Render();
		int FramebufferWidth = 0;
		int FramebufferHeight = 0;
		glfwGetFramebufferSize(Window, &FramebufferWidth, &FramebufferHeight);
		glViewport(0, 0, FramebufferWidth, FramebufferHeight);
		glClearColor(0.07f, 0.075f, 0.09f, 1.0f);
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
