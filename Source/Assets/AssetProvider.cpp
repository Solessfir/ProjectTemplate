#include "Assets/AssetProvider.h"

#include "Assets/AssetPath.h"

#ifdef PROJECTTEMPLATE_EMBEDDED_ASSETS
#include "Assets/EmbeddedAssets.h"
#endif

#include <fstream>
#include <utility>

namespace ProjectTemplate
{
FAssetProvider::FAssetProvider(std::filesystem::path AssetRoot)
	: AssetRoot(std::move(AssetRoot))
{
}

std::expected<std::span<const std::byte>, FAssetLoadError> FAssetProvider::Load(const std::string_view VirtualPath)
{
	const std::optional<std::string> NormalizedPath = NormalizeVirtualAssetPath(VirtualPath);
	if (!NormalizedPath)
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::InvalidPath,
			.Message = "Invalid virtual asset path: " + std::string(VirtualPath)
		});
	}

#ifdef PROJECTTEMPLATE_EMBEDDED_ASSETS
	const std::optional<std::span<const std::byte>> Asset = Private::FindEmbeddedAsset(*NormalizedPath);
	if (!Asset)
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::NotFound,
			.Message = "Embedded asset was not found: " + *NormalizedPath
		});
	}

	return *Asset;
#else
	if (const auto ExistingAsset = LooseAssetCache.find(*NormalizedPath); ExistingAsset != LooseAssetCache.end())
	{
		return std::as_bytes(std::span(ExistingAsset->second));
	}

	const std::filesystem::path SourcePath = AssetRoot / std::filesystem::path(*NormalizedPath);
	std::ifstream Stream(SourcePath, std::ios::binary | std::ios::ate);
	if (!Stream)
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::NotFound,
			.Message = "Loose asset was not found: " + SourcePath.string()
		});
	}

	const std::streampos EndPosition = Stream.tellg();
	if (EndPosition < 0)
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::ReadFailed,
			.Message = "Could not determine asset size: " + SourcePath.string()
		});
	}

	std::vector<char> Bytes(static_cast<std::size_t>(EndPosition));
	Stream.seekg(0, std::ios::beg);
	if (!Bytes.empty() && !Stream.read(Bytes.data(), static_cast<std::streamsize>(Bytes.size())))
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::ReadFailed,
			.Message = "Could not read loose asset: " + SourcePath.string()
		});
	}

	auto [Iterator, bInserted] = LooseAssetCache.emplace(*NormalizedPath, std::move(Bytes));
	if (!bInserted)
	{
		return std::unexpected(FAssetLoadError{
			.Code = EAssetLoadError::ReadFailed,
			.Message = "Could not cache loose asset: " + *NormalizedPath
		});
	}

	return std::as_bytes(std::span(Iterator->second));
#endif
}
}
