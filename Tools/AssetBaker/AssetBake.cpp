#include "AssetBake.h"

#include "Assets/AssetPath.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace ProjectTemplate::AssetBake
{
namespace
{
[[nodiscard]] std::string_view Trim(const std::string_view Text)
{
	std::size_t Start = 0;
	while (Start < Text.size() && std::isspace(static_cast<unsigned char>(Text[Start])) != 0)
	{
		Start++;
	}

	std::size_t End = Text.size();
	while (End > Start && std::isspace(static_cast<unsigned char>(Text[End - 1])) != 0)
	{
		End--;
	}

	return Text.substr(Start, End - Start);
}

[[nodiscard]] std::expected<std::string, FError> ReadTextFile(const std::filesystem::path& Path)
{
	std::ifstream Stream(Path, std::ios::binary | std::ios::ate);
	if (!Stream)
	{
		return std::unexpected(FError{ "Could not open manifest: " + Path.string() });
	}

	const std::streampos EndPosition = Stream.tellg();
	if (EndPosition < 0)
	{
		return std::unexpected(FError{ "Could not determine manifest size: " + Path.string() });
	}

	std::string Text(static_cast<std::size_t>(EndPosition), '\0');
	Stream.seekg(0, std::ios::beg);
	if (!Text.empty() && !Stream.read(Text.data(), static_cast<std::streamsize>(Text.size())))
	{
		return std::unexpected(FError{ "Could not read manifest: " + Path.string() });
	}

	return Text;
}

[[nodiscard]] std::expected<std::vector<char>, FError> ReadBinaryFile(const std::filesystem::path& Path)
{
	std::ifstream Stream(Path, std::ios::binary | std::ios::ate);
	if (!Stream)
	{
		return std::unexpected(FError{ "Could not open asset: " + Path.string() });
	}

	const std::streampos EndPosition = Stream.tellg();
	if (EndPosition < 0)
	{
		return std::unexpected(FError{ "Could not determine asset size: " + Path.string() });
	}

	std::vector<char> Bytes(static_cast<std::size_t>(EndPosition));
	Stream.seekg(0, std::ios::beg);
	if (!Bytes.empty() && !Stream.read(Bytes.data(), static_cast<std::streamsize>(Bytes.size())))
	{
		return std::unexpected(FError{ "Could not read asset: " + Path.string() });
	}

	return Bytes;
}

[[nodiscard]] std::string EscapeStringLiteral(const std::string_view Text)
{
	std::string Escaped;
	Escaped.reserve(Text.size());
	for (const char Character : Text)
	{
		if (Character == '\\' || Character == '"')
		{
			Escaped.push_back('\\');
		}
		Escaped.push_back(Character);
	}
	return Escaped;
}

[[nodiscard]] std::expected<void, FError> WriteFileIfChanged(const std::filesystem::path& OutputPath, const std::string_view Contents)
{
	if (const std::expected<std::string, FError> ExistingContents = ReadTextFile(OutputPath);
		ExistingContents && *ExistingContents == Contents)
	{
		return {};
	}

	std::error_code Error;
	const std::filesystem::path ParentPath = OutputPath.parent_path();
	if (!ParentPath.empty())
	{
		std::filesystem::create_directories(ParentPath, Error);
		if (Error)
		{
			return std::unexpected(FError{ "Could not create generated asset directory: " + Error.message() });
		}
	}

	std::filesystem::path TemporaryPath = OutputPath;
	TemporaryPath += ".tmp";
	{
		std::ofstream Stream(TemporaryPath, std::ios::binary | std::ios::trunc);
		if (!Stream || !Stream.write(Contents.data(), static_cast<std::streamsize>(Contents.size())))
		{
			std::filesystem::remove(TemporaryPath, Error);
			return std::unexpected(FError{ "Could not write generated asset source: " + TemporaryPath.string() });
		}
	}

	std::filesystem::remove(OutputPath, Error);
	Error.clear();
	std::filesystem::rename(TemporaryPath, OutputPath, Error);
	if (Error)
	{
		std::filesystem::remove(TemporaryPath, Error);
		return std::unexpected(FError{ "Could not install generated asset source: " + OutputPath.string() });
	}

	return {};
}
}

std::expected<std::vector<std::string>, FError> ParseManifest(const std::string_view ManifestText)
{
	std::vector<std::string> Paths;
	std::unordered_set<std::string> UniquePaths;
	std::size_t LineStart = 0;
	std::size_t LineNumber = 1;

	while (LineStart <= ManifestText.size())
	{
		const std::size_t LineEnd = ManifestText.find('\n', LineStart);
		const std::string_view Line = Trim(ManifestText.substr(
			LineStart,
			LineEnd == std::string_view::npos ? ManifestText.size() - LineStart : LineEnd - LineStart));

		if (!Line.empty() && !Line.starts_with('#'))
		{
			const std::optional<std::string> NormalizedPath = NormalizeVirtualAssetPath(Line);
			if (!NormalizedPath)
			{
				return std::unexpected(FError{
					"Invalid asset path on manifest line " + std::to_string(LineNumber) + ": " + std::string(Line)
				});
			}

			if (!UniquePaths.emplace(*NormalizedPath).second)
			{
				return std::unexpected(FError{ "Duplicate asset path in manifest: " + *NormalizedPath });
			}
			Paths.push_back(*NormalizedPath);
		}

		if (LineEnd == std::string_view::npos)
		{
			break;
		}
		LineStart = LineEnd + 1;
		LineNumber++;
	}

	std::ranges::sort(Paths);
	return Paths;
}

std::expected<std::string, FError> GenerateSource(const std::span<const FAsset> Assets)
{
	std::vector<const FAsset*> SortedAssets;
	SortedAssets.reserve(Assets.size());
	std::unordered_set<std::string> UniquePaths;
	for (const FAsset& Asset : Assets)
	{
		const std::optional<std::string> NormalizedPath = NormalizeVirtualAssetPath(Asset.VirtualPath);
		if (!NormalizedPath || *NormalizedPath != Asset.VirtualPath)
		{
			return std::unexpected(FError{ "Asset path is not normalized: " + Asset.VirtualPath });
		}
		if (!UniquePaths.emplace(Asset.VirtualPath).second)
		{
			return std::unexpected(FError{ "Duplicate asset path: " + Asset.VirtualPath });
		}
		SortedAssets.push_back(&Asset);
	}

	std::ranges::sort(SortedAssets, {}, [](const FAsset* const Asset)
	{
		return Asset->VirtualPath;
	});

	std::ostringstream Source;
	Source.imbue(std::locale::classic());
	Source << "// Generated by AssetBaker. Do not edit.\n"
		<< "#include \"Assets/EmbeddedAssets.h\"\n\n"
		<< "#include <array>\n"
		<< "#include <cstdint>\n\n"
		<< "namespace ProjectTemplate::Private\n"
		<< "{\n"
		<< "namespace\n"
		<< "{\n";

	for (std::size_t AssetIndex = 0; AssetIndex < SortedAssets.size(); AssetIndex++)
	{
		const FAsset& Asset = *SortedAssets[AssetIndex];
		Source << "constexpr std::array<std::uint8_t, " << Asset.Bytes.size() << "> Asset" << AssetIndex << " = {";
		for (std::size_t ByteIndex = 0; ByteIndex < Asset.Bytes.size(); ByteIndex++)
		{
			if (ByteIndex % 16 == 0)
			{
				Source << "\n\t";
			}
			const auto Byte = static_cast<std::uint8_t>(static_cast<unsigned char>(Asset.Bytes[ByteIndex]));
			Source << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(Byte)
				<< std::dec;
			if (ByteIndex + 1 < Asset.Bytes.size())
			{
				Source << ", ";
			}
		}
		if (!Asset.Bytes.empty())
		{
			Source << '\n';
		}
		Source << "};\n\n";
	}

	Source << "}\n\n"
		<< "std::optional<std::span<const std::byte>> FindEmbeddedAsset(const std::string_view VirtualPath) noexcept\n"
		<< "{\n"
		<< "\t(void)VirtualPath;\n";
	for (std::size_t AssetIndex = 0; AssetIndex < SortedAssets.size(); AssetIndex++)
	{
		Source << "\tif (VirtualPath == \"" << EscapeStringLiteral(SortedAssets[AssetIndex]->VirtualPath) << "\")\n"
			<< "\t{\n"
			<< "\t\treturn std::as_bytes(std::span(Asset" << AssetIndex << "));\n"
			<< "\t}\n";
	}
	Source << "\n\treturn std::nullopt;\n"
		<< "}\n"
		<< "}\n";

	return Source.str();
}

std::expected<void, FError> Bake(const std::filesystem::path& ManifestPath, const std::filesystem::path& AssetRoot, const std::filesystem::path& OutputPath)
{
	const std::expected<std::string, FError> ManifestText = ReadTextFile(ManifestPath);
	if (!ManifestText)
	{
		return std::unexpected(ManifestText.error());
	}

	const std::expected<std::vector<std::string>, FError> AssetPaths = ParseManifest(*ManifestText);
	if (!AssetPaths)
	{
		return std::unexpected(AssetPaths.error());
	}

	std::vector<FAsset> Assets;
	Assets.reserve(AssetPaths->size());
	for (const std::string& VirtualPath : *AssetPaths)
	{
		std::expected<std::vector<char>, FError> Bytes = ReadBinaryFile(AssetRoot / std::filesystem::path(VirtualPath));
		if (!Bytes)
		{
			return std::unexpected(Bytes.error());
		}
		Assets.push_back(FAsset{ .VirtualPath = VirtualPath, .Bytes = std::move(*Bytes) });
	}

	const std::expected<std::string, FError> GeneratedSource = GenerateSource(Assets);
	if (!GeneratedSource)
	{
		return std::unexpected(GeneratedSource.error());
	}

	return WriteFileIfChanged(OutputPath, *GeneratedSource);
}
}
