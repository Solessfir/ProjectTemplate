#include "Application/ApplicationPaths.h"

#include <optional>

namespace ProjectTemplate
{
namespace
{
[[nodiscard]] std::filesystem::path NormalizePath(const std::filesystem::path& Path)
{
	std::error_code PathError;
	const std::filesystem::path AbsolutePath = std::filesystem::absolute(Path, PathError);
	if (PathError)
	{
		return Path.lexically_normal();
	}

	const std::filesystem::path CanonicalPath = std::filesystem::weakly_canonical(AbsolutePath, PathError);
	return PathError ? AbsolutePath.lexically_normal() : CanonicalPath;
}

[[nodiscard]] std::optional<std::filesystem::path> FindSourceRoot(std::filesystem::path Candidate)
{
	if (Candidate.empty())
	{
		return std::nullopt;
	}

	Candidate = NormalizePath(Candidate);
	while (!Candidate.empty())
	{
		std::error_code MarkerError;
		if (std::filesystem::is_regular_file(Candidate / "Assets/EmbeddedAssets.txt", MarkerError))
		{
			return Candidate;
		}

		const std::filesystem::path Parent = Candidate.parent_path();
		if (Parent == Candidate)
		{
			break;
		}

		Candidate = Parent;
	}

	return std::nullopt;
}

[[nodiscard]] FApplicationPaths MakeApplicationPaths(const std::filesystem::path& RootDirectory)
{
	return FApplicationPaths{
		.RootDirectory = RootDirectory,
		.AssetDirectory = RootDirectory / "Assets",
		.SavedDirectory = RootDirectory / "Saved"
	};
}
}

FApplicationPaths ResolveApplicationPaths(const std::filesystem::path& ExecutablePath, const std::filesystem::path& WorkingDirectory)
{
	std::filesystem::path ResolvedExecutablePath = ExecutablePath;
	if (ResolvedExecutablePath.is_relative() && !WorkingDirectory.empty())
	{
		ResolvedExecutablePath = WorkingDirectory / ResolvedExecutablePath;
	}

	const std::filesystem::path ExecutableDirectory = NormalizePath(ResolvedExecutablePath).parent_path();
	if (const std::optional<std::filesystem::path> SourceRoot = FindSourceRoot(ExecutableDirectory))
	{
		return MakeApplicationPaths(*SourceRoot);
	}

	if (const std::optional<std::filesystem::path> SourceRoot = FindSourceRoot(WorkingDirectory))
	{
		return MakeApplicationPaths(*SourceRoot);
	}

	const std::filesystem::path FallbackRoot = !ExecutableDirectory.empty() ? ExecutableDirectory : NormalizePath(WorkingDirectory);
	return MakeApplicationPaths(FallbackRoot);
}
}
