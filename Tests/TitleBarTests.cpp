#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "UI/TitleBarLayout.h"

#include <doctest/doctest.h>

using ProjectTemplate::ETitleBarHitRegion;
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

	CHECK(HitTestTitleBar(Layout, 500, 30, false) == ETitleBarHitRegion::Caption);
	CHECK(HitTestTitleBar(Layout, 1190, 30, false) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(Layout, 1100, 30, false) == ETitleBarHitRegion::MaximizeButton);
	CHECK(HitTestTitleBar(Layout, 1030, 30, false) == ETitleBarHitRegion::MinimizeButton);
	CHECK(HitTestTitleBar(Layout, 20, 30, false) == ETitleBarHitRegion::SystemMenu);
	CHECK(HitTestTitleBar(Layout, 80, 30, false) == ETitleBarHitRegion::ApplicationMenu);
	CHECK(HitTestTitleBar(Layout, 120, 30, false) == ETitleBarHitRegion::Caption);
}

TEST_CASE("Resize borders take priority over caption controls")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, false);

	CHECK(HitTestTitleBar(Layout, 0, 0, false) == ETitleBarHitRegion::ResizeTopLeft);
	CHECK(HitTestTitleBar(Layout, 999, 0, false) == ETitleBarHitRegion::ResizeTopRight);
	CHECK(HitTestTitleBar(Layout, 0, 699, false) == ETitleBarHitRegion::ResizeBottomLeft);
	CHECK(HitTestTitleBar(Layout, 999, 699, false) == ETitleBarHitRegion::ResizeBottomRight);
	CHECK(HitTestTitleBar(Layout, 500, 699, false) == ETitleBarHitRegion::ResizeBottom);
}

TEST_CASE("Maximized windows do not expose resize borders")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, true);

	CHECK(HitTestTitleBar(Layout, 0, 0, false) == ETitleBarHitRegion::SystemMenu);
	CHECK(HitTestTitleBar(Layout, 999, 0, false) == ETitleBarHitRegion::CloseButton);
	CHECK(HitTestTitleBar(Layout, 500, 699, false) == ETitleBarHitRegion::Client);
}

TEST_CASE("Non-resizable windows keep client edges")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, false, false);

	CHECK(HitTestTitleBar(Layout, 0, 500, false) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, 999, 500, false) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, -1, 10, false) == ETitleBarHitRegion::Client);
}

TEST_CASE("ImGui input takes priority over native title bar regions")
{
	constexpr auto Layout = MakeTitleBarLayout(1000, 700, 1.0f, true, false);

	CHECK(HitTestTitleBar(Layout, 500, 20, true) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, 980, 20, true) == ETitleBarHitRegion::Client);
	CHECK(HitTestTitleBar(Layout, 0, 0, true) == ETitleBarHitRegion::Client);
}
