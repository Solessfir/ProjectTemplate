#pragma once

#include <cstddef>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ProjectTemplate
{
enum class EAssetLoadError
{
	InvalidPath,
	NotFound,
	ReadFailed
};

struct FAssetLoadError
{
	EAssetLoadError Code;
	std::string Message;
};

class FAssetProvider
{
public:
	explicit FAssetProvider(std::filesystem::path AssetRoot);

	[[nodiscard]] std::expected<std::span<const std::byte>, FAssetLoadError> Load(std::string_view VirtualPath);

private:
	std::filesystem::path AssetRoot;
	std::unordered_map<std::string, std::vector<char>> LooseAssetCache;
};
}
