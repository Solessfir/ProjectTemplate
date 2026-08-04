#include "Application/ApplicationPaths.h"

#include <chrono>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace
{
class FTemporaryDirectory
{
public:
	explicit FTemporaryDirectory(std::filesystem::path Path)
	    : Path(std::move(Path))
	{
	}

	~FTemporaryDirectory()
	{
		std::error_code CleanupError;
		std::filesystem::remove_all(Path, CleanupError);
	}

	FTemporaryDirectory(const FTemporaryDirectory&) = delete;
	FTemporaryDirectory& operator=(const FTemporaryDirectory&) = delete;

	std::filesystem::path Path;
};

[[nodiscard]] FTemporaryDirectory MakeTemporaryDirectory()
{
	const auto UniqueSuffix = std::chrono::steady_clock::now().time_since_epoch().count();
	const std::filesystem::path Path = std::filesystem::temp_directory_path() / ("ProjectTemplateApplicationPaths-" + std::to_string(UniqueSuffix));
	return FTemporaryDirectory(Path);
}
}

TEST_CASE("Application paths find the source root from a binary directory")
{
	FTemporaryDirectory TemporaryDirectory = MakeTemporaryDirectory();
	const std::filesystem::path RepositoryRoot = TemporaryDirectory.Path / "ProjectTemplate";
	const std::filesystem::path BinaryDirectory = RepositoryRoot / "Binaries/Windows/x86_64/Development";
	const std::filesystem::path ManifestPath = RepositoryRoot / "Assets/EmbeddedAssets.txt";
	REQUIRE(std::filesystem::create_directories(ManifestPath.parent_path()));
	REQUIRE(std::filesystem::create_directories(BinaryDirectory));

	std::ofstream Manifest(ManifestPath);
	REQUIRE(Manifest.is_open());
	Manifest << "Fonts/Roboto/Roboto-Regular.ttf\n";
	Manifest.close();

	const ProjectTemplate::FApplicationPaths Paths = ProjectTemplate::ResolveApplicationPaths(BinaryDirectory / "StarterApp.exe", BinaryDirectory);
	CHECK(std::filesystem::equivalent(Paths.RootDirectory, RepositoryRoot));
	CHECK(Paths.AssetDirectory == Paths.RootDirectory / "Assets");
	CHECK(Paths.SavedDirectory == Paths.RootDirectory / "Saved");
}

TEST_CASE("Application paths fall back beside a packaged executable")
{
	FTemporaryDirectory TemporaryDirectory = MakeTemporaryDirectory();
	const std::filesystem::path PackageDirectory = TemporaryDirectory.Path / "Package";
	REQUIRE(std::filesystem::create_directories(PackageDirectory));

	const ProjectTemplate::FApplicationPaths Paths = ProjectTemplate::ResolveApplicationPaths(PackageDirectory / "StarterApp.exe", TemporaryDirectory.Path);
	CHECK(std::filesystem::equivalent(Paths.RootDirectory, PackageDirectory));
	CHECK(Paths.AssetDirectory == Paths.RootDirectory / "Assets");
	CHECK(Paths.SavedDirectory == Paths.RootDirectory / "Saved");
}
