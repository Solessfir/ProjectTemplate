#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "UI/TitleBarLayout.h"

#include <doctest/doctest.h>

using ProjectTemplate::ETitleBarHitRegion;
using ProjectTemplate::FTitleBarHitTestState;
using ProjectTemplate::FWindowActionCapabilities;
using ProjectTemplate::FWindowControlPolicy;
using ProjectTemplate::HitTestTitleBar;
using ProjectTemplate::MakeTitleBarLayout;
using ProjectTemplate::ResolveTitleBarUiScale;

TEST_CASE("Wayland title bars remain in logical coordinates")
{
	CHECK(ResolveTitleBarUiScale(true, 2.0f) == 1.0f);
	CHECK(ResolveTitleBarUiScale(false, 2.0f) == 2.0f);
	CHECK(ResolveTitleBarUiScale(false, 0.0f) == 1.0f);
}

TEST_CASE("Title bar scales its native hit regions")
{
	constexpr auto Layout = MakeTitleBarLayout(1200, 800, 1.5f, true, false);
	static_assert(Layout.TitleBarHeight == 54);
	static_assert(Layout.ButtonWidth == 69);
	static_assert(Layout.ResizeBorder == 9);

	CHECK(HitTestTitleBar(Layout, 500, 30) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(Layout, 1190, 30) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(Layout, 1100, 30) == ETitleBarHitRegion::MaximizeButton);
	CHECK(HitTestTitleBar(Layout, 1030, 30) == ETitleBarHitRegion::MinimizeButton);
	CHECK(HitTestTitleBar(Layout, 20, 30) == ETitleBarHitRegion::SystemMenu);
	CHECK(HitTestTitleBar(Layout, 80, 30) == ETitleBarHitRegion::ApplicationMenu);
	CHECK(HitTestTitleBar(Layout, 120, 30) == ETitleBarHitRegion::Caption);
}

TEST_CASE("Title bar controls follow advertised window actions")
{
	constexpr auto WaylandLayout = MakeTitleBarLayout(1000, 700, 1.0f, true, false, {.bMinimize = false, .bMaximize = true, .bWindowMenu = false});
	CHECK_FALSE(WaylandLayout.bMinimizeVisible);
	CHECK(WaylandLayout.bMaximizeVisible);
	CHECK_FALSE(WaylandLayout.bSystemMenuEnabled);
	CHECK(HitTestTitleBar(WaylandLayout, 990, 20) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(WaylandLayout, 930, 20) == ETitleBarHitRegion::MaximizeButton);
	CHECK(HitTestTitleBar(WaylandLayout, 880, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(WaylandLayout, 20, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(WaylandLayout, 50, 20) == ETitleBarHitRegion::ApplicationMenu);

	constexpr auto MinimizeOnly = MakeTitleBarLayout(1000, 700, 1.0f, true, false, {.bMinimize = true, .bMaximize = false, .bWindowMenu = false});
	CHECK(HitTestTitleBar(MinimizeOnly, 930, 20) == ETitleBarHitRegion::MinimizeButton);
	CHECK(HitTestTitleBar(MinimizeOnly, 880, 20) == ETitleBarHitRegion::Caption);

	constexpr auto FixedSize = MakeTitleBarLayout(1000, 700, 1.0f, false, false, {.bMinimize = false, .bMaximize = true, .bWindowMenu = false});
	CHECK_FALSE(FixedSize.bMaximizeVisible);
	CHECK(HitTestTitleBar(FixedSize, 930, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(FixedSize, 990, 20) == ETitleBarHitRegion::CloseButton);
}

TEST_CASE("Title bar control policy defaults can hide every control")
{
	constexpr FWindowControlPolicy NoControls{.bShowClose = false, .bShowMinimize = false, .bShowMaximize = false, .bEnableWindowMenu = false};
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, false, FWindowActionCapabilities{}, NoControls);
	CHECK_FALSE(Layout.bCloseVisible);
	CHECK_FALSE(Layout.bMinimizeVisible);
	CHECK_FALSE(Layout.bMaximizeVisible);
	CHECK_FALSE(Layout.bSystemMenuEnabled);
	CHECK(HitTestTitleBar(Layout, 990, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(Layout, 930, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(Layout, 20, 20) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(Layout, 50, 20) == ETitleBarHitRegion::ApplicationMenu);

	constexpr FWindowControlPolicy CloseAndMinimize{.bShowClose = true, .bShowMinimize = true, .bShowMaximize = false, .bEnableWindowMenu = false};
	constexpr auto Selected = MakeTitleBarLayout(1000, 700, 1.0f, true, false, FWindowActionCapabilities{}, CloseAndMinimize);
	CHECK(HitTestTitleBar(Selected, 990, 20) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(Selected, 930, 20) == ETitleBarHitRegion::MinimizeButton);
}

TEST_CASE("Resize borders take priority over caption controls")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, false);

	CHECK(HitTestTitleBar(Layout, 0, 0) == ETitleBarHitRegion::ResizeTopLeft);
	CHECK(HitTestTitleBar(Layout, 999, 0) == ETitleBarHitRegion::ResizeTopRight);
	CHECK(HitTestTitleBar(Layout, 0, 699) == ETitleBarHitRegion::ResizeBottomLeft);
	CHECK(HitTestTitleBar(Layout, 999, 699) == ETitleBarHitRegion::ResizeBottomRight);
	CHECK(HitTestTitleBar(Layout, 500, 699) == ETitleBarHitRegion::ResizeBottom);
}

TEST_CASE("Maximized windows do not expose resize borders")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, true);

	CHECK(HitTestTitleBar(Layout, 0, 0) == ETitleBarHitRegion::SystemMenu);
	CHECK(HitTestTitleBar(Layout, 999, 0) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(Layout, 500, 699) == ETitleBarHitRegion::Client);
}

TEST_CASE("Non-resizable windows keep client edges")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, false, false);

	CHECK(HitTestTitleBar(Layout, 0, 500) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, 999, 500) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, -1, 10) == ETitleBarHitRegion::Client);
}

TEST_CASE("ImGui capture is limited to cached title bar intersections")
{
	FTitleBarHitTestState State;
	State.Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, false);
	State.UiCaptureRegions[0] = {.MinimumX = 200, .MinimumY = 0, .MaximumX = 500, .MaximumY = 36};
	State.UiCaptureRegionCount = 1;

	CHECK(HitTestTitleBar(State, 200, 0) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(State, 499, 35) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(State, 500, 35) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(State, 990, 20) == ETitleBarHitRegion::CloseButton);

	State.bUiCapturesEntireTitleBar = true;
	CHECK(HitTestTitleBar(State, 990, 20) == ETitleBarHitRegion::Client);
}
