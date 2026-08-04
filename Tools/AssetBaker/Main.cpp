#include "AssetBake.h"

#include <exception>
#include <filesystem>
#include <print>
#include <string_view>

namespace
{
struct FArguments
{
	std::filesystem::path ManifestPath;
	std::filesystem::path AssetRoot;
	std::filesystem::path OutputPath;
};

[[nodiscard]] bool ParseArguments(const int ArgumentCount, char** const Arguments, FArguments& OutArguments)
{
	for (int Index = 1; Index < ArgumentCount; Index++)
	{
		const std::string_view Argument = Arguments[Index];
		if (Index + 1 >= ArgumentCount)
		{
			return false;
		}

		const std::filesystem::path Value = Arguments[++Index];
		if (Argument == "--manifest")
		{
			OutArguments.ManifestPath = Value;
		}
		else if (Argument == "--asset-root")
		{
			OutArguments.AssetRoot = Value;
		}
		else if (Argument == "--output")
		{
			OutArguments.OutputPath = Value;
		}
		else
		{
			return false;
		}
	}

	return !OutArguments.ManifestPath.empty() && !OutArguments.AssetRoot.empty() && !OutArguments.OutputPath.empty();
}

[[nodiscard]] int RunAssetBaker(const int ArgumentCount, char** const Arguments)
{
	FArguments ParsedArguments;
	if (!ParseArguments(ArgumentCount, Arguments, ParsedArguments))
	{
		std::println(stderr, "Usage: AssetBaker --manifest <path> --asset-root <path> --output <path>");
		return 1;
	}

	const std::expected<void, ProjectTemplate::AssetBake::FError> Result = ProjectTemplate::AssetBake::Bake(
	    ParsedArguments.ManifestPath,
	    ParsedArguments.AssetRoot,
	    ParsedArguments.OutputPath);
	if (!Result)
	{
		std::println(stderr, "Asset baking failed: {}", Result.error().Message);
		return 1;
	}

	std::println("Embedded assets generated: {}", ParsedArguments.OutputPath.string());
	return 0;
}
}

int main(const int ArgumentCount, char** const Arguments)
{
	try
	{
		return RunAssetBaker(ArgumentCount, Arguments);
	}
	catch (const std::exception& Error)
	{
		std::println(stderr, "Asset baking failed with an exception: {}", Error.what());
	}
	catch (...)
	{
		std::println(stderr, "Asset baking failed with an unknown exception.");
	}

	return 1;
}
