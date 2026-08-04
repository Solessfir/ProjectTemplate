#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace ProjectTemplate
{
[[nodiscard]] constexpr std::optional<std::string> NormalizeVirtualAssetPath(const std::string_view Path)
{
	if (Path.empty())
	{
		return std::nullopt;
	}

	std::string NormalizedPath(Path);
	for (char& Character : NormalizedPath)
	{
		if (Character == '\\')
		{
			Character = '/';
		}
		else if (Character == ':' || static_cast<unsigned char>(Character) < 0x20)
		{
			return std::nullopt;
		}
	}

	if (NormalizedPath.front() == '/' || NormalizedPath.back() == '/')
	{
		return std::nullopt;
	}

	std::size_t SegmentStart = 0;
	while (SegmentStart < NormalizedPath.size())
	{
		const std::size_t SegmentEnd = NormalizedPath.find('/', SegmentStart);
		const std::size_t SegmentLength =
		    (SegmentEnd == std::string::npos ? NormalizedPath.size() : SegmentEnd) - SegmentStart;
		const std::string_view Segment(NormalizedPath.data() + SegmentStart, SegmentLength);

		if (Segment.empty() || Segment == "." || Segment == "..")
		{
			return std::nullopt;
		}

		if (SegmentEnd == std::string::npos)
		{
			break;
		}
		SegmentStart = SegmentEnd + 1;
	}

	return NormalizedPath;
}
}
