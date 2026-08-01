#pragma once

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ProjectTemplate::AssetBake
{
struct FError
{
	std::string Message;
};

struct FAsset
{
	std::string VirtualPath;
	std::vector<char> Bytes;
};

[[nodiscard]] std::expected<std::vector<std::string>, FError> ParseManifest(std::string_view ManifestText);
[[nodiscard]] std::expected<std::string, FError> GenerateSource(std::span<const FAsset> Assets);
[[nodiscard]] std::expected<void, FError> Bake(
	const std::filesystem::path& ManifestPath,
	const std::filesystem::path& AssetRoot,
	const std::filesystem::path& OutputPath);
}
