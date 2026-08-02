#pragma once

#include <filesystem>

namespace ProjectTemplate
{
struct FApplicationPaths
{
	std::filesystem::path RootDirectory;
	std::filesystem::path AssetDirectory;
	std::filesystem::path SavedDirectory;
};

[[nodiscard]] FApplicationPaths ResolveApplicationPaths(const std::filesystem::path& ExecutablePath, const std::filesystem::path& WorkingDirectory);
}
