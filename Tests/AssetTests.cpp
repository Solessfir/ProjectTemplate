#include <doctest/doctest.h>

#include "AssetBake.h"
#include "Assets/AssetPath.h"
#include "Assets/AssetProvider.h"

#include <array>
#include <string>

using ProjectTemplate::EAssetLoadError;
using ProjectTemplate::FAssetProvider;
using ProjectTemplate::NormalizeVirtualAssetPath;
using ProjectTemplate::AssetBake::FAsset;
using ProjectTemplate::AssetBake::GenerateSource;
using ProjectTemplate::AssetBake::ParseManifest;

TEST_CASE("Virtual asset paths are normalized without allowing traversal")
{
	CHECK(NormalizeVirtualAssetPath("Fonts\\Roboto\\Regular.ttf") == "Fonts/Roboto/Regular.ttf");
	CHECK_FALSE(NormalizeVirtualAssetPath("../Secrets.txt"));
	CHECK_FALSE(NormalizeVirtualAssetPath("Fonts//Roboto.ttf"));
	CHECK_FALSE(NormalizeVirtualAssetPath("C:/Windows/Fonts/Roboto.ttf"));
}

TEST_CASE("Asset manifests ignore comments and produce stable order")
{
	const auto Result = ParseManifest(
		"# Embedded application resources\n"
		"Icons/App.svg\n"
		"Fonts/Roboto.ttf\r\n");

	REQUIRE(Result);
	REQUIRE(Result->size() == 2);
	CHECK((*Result)[0] == "Fonts/Roboto.ttf");
	CHECK((*Result)[1] == "Icons/App.svg");
}

TEST_CASE("Asset manifests reject duplicate paths after normalization")
{
	const auto Result = ParseManifest("Icons\\App.svg\nIcons/App.svg\n");

	REQUIRE_FALSE(Result);
	CHECK(Result.error().Message == "Duplicate asset path in manifest: Icons/App.svg");
}

TEST_CASE("Generated asset source is deterministic and preserves binary bytes")
{
	const std::array Assets = {
		FAsset{ .VirtualPath = "Second.bin", .Bytes = { static_cast<char>(0xff), static_cast<char>(0x7f) } },
		FAsset{ .VirtualPath = "First.bin", .Bytes = { static_cast<char>(0x00), static_cast<char>(0x01) } }
	};
	const std::array ReversedAssets = { Assets[1], Assets[0] };

	const auto FirstSource = GenerateSource(Assets);
	const auto SecondSource = GenerateSource(ReversedAssets);

	REQUIRE(FirstSource);
	REQUIRE(SecondSource);
	CHECK(*FirstSource == *SecondSource);
	CHECK(FirstSource->find("0x00, 0x01") != std::string::npos);
	CHECK(FirstSource->find("0xff, 0x7f") != std::string::npos);
	CHECK(FirstSource->find("First.bin") < FirstSource->find("Second.bin"));
}

TEST_CASE("Loose asset provider reports invalid and missing paths")
{
	FAssetProvider Provider("Assets");

	const auto InvalidResult = Provider.Load("../Outside.bin");
	REQUIRE_FALSE(InvalidResult);
	CHECK(InvalidResult.error().Code == EAssetLoadError::InvalidPath);

	const auto MissingResult = Provider.Load("Tests/DefinitelyMissing.bin");
	REQUIRE_FALSE(MissingResult);
	CHECK(MissingResult.error().Code == EAssetLoadError::NotFound);
}

TEST_CASE("Roboto font sources are available through the loose asset provider")
{
	FAssetProvider Provider("Assets");

	const auto RegularFont = Provider.Load("Fonts/Roboto/Roboto-Regular.ttf");
	const auto MediumFont = Provider.Load("Fonts/Roboto/Roboto-Medium.ttf");
	const auto License = Provider.Load("Fonts/Roboto/OFL.txt");

	REQUIRE(RegularFont);
	REQUIRE(MediumFont);
	REQUIRE(License);
	CHECK(RegularFont->size() == 159108);
	CHECK(MediumFont->size() == 159296);
	CHECK(License->size() == 4487);
}
