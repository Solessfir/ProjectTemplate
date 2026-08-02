#include <doctest/doctest.h>

#include "UI/WorkspaceLayout.h"

using ProjectTemplate::ResolveToolbarRightX;
using ProjectTemplate::ResolveCenteredWindowPlacement;
using ProjectTemplate::ShouldShowToolbarStatus;
using ProjectTemplate::EPanelTransparencyMode;
using ProjectTemplate::IsPanelTransparent;

TEST_CASE("Panel transparency mode distinguishes docked and floating windows")
{
	CHECK(IsPanelTransparent(EPanelTransparencyMode::All, false));
	CHECK(IsPanelTransparent(EPanelTransparencyMode::All, true));
	CHECK(IsPanelTransparent(EPanelTransparencyMode::FloatingOnly, false));
	CHECK_FALSE(IsPanelTransparent(EPanelTransparencyMode::FloatingOnly, true));
	CHECK_FALSE(IsPanelTransparent(EPanelTransparencyMode::DockedOnly, false));
	CHECK(IsPanelTransparent(EPanelTransparencyMode::DockedOnly, true));
	CHECK_FALSE(IsPanelTransparent(EPanelTransparencyMode::Disabled, false));
	CHECK_FALSE(IsPanelTransparent(EPanelTransparencyMode::Disabled, true));
}

TEST_CASE("Toolbar status respects available logical width")
{
	CHECK_FALSE(ShouldShowToolbarStatus(719.0f, 1.0f));
	CHECK(ShouldShowToolbarStatus(720.0f, 1.0f));
	CHECK_FALSE(ShouldShowToolbarStatus(1439.0f, 2.0f));
	CHECK(ShouldShowToolbarStatus(1440.0f, 2.0f));
	CHECK_FALSE(ShouldShowToolbarStatus(720.0f, 0.0f));
}

TEST_CASE("Toolbar actions stay right aligned without overlapping left content")
{
	CHECK(ResolveToolbarRightX(1280.0f, 220.0f, 320.0f, 16.0f) == 944.0f);
	CHECK(ResolveToolbarRightX(480.0f, 220.0f, 300.0f, 16.0f) == 220.0f);
}

TEST_CASE("Initial window occupies 80 percent of the monitor work area and stays centered")
{
	const ProjectTemplate::FWindowPlacement Placement = ResolveCenteredWindowPlacement(1920, 40, 1600, 900, 80);
	CHECK(Placement.X == 2080);
	CHECK(Placement.Y == 130);
	CHECK(Placement.Width == 1280);
	CHECK(Placement.Height == 720);
}
