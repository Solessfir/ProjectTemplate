#pragma once

#include "Logging/Log.h"
#include "UI/LogTextSelection.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ProjectTemplate
{
[[nodiscard]] constexpr std::uint32_t HashLogCategory(const std::string_view Category) noexcept
{
	std::uint32_t Hash = 2166136261u;
	for (const char Character : Category)
	{
		Hash ^= static_cast<unsigned char>(Character);
		Hash *= 16777619u;
	}

	return Hash;
}

[[nodiscard]] constexpr bool HasLogLevelColorOverride(const ELogLevel Level) noexcept
{
	return Level == ELogLevel::Warning || Level == ELogLevel::Error;
}

[[nodiscard]] constexpr bool ShouldScrollOutputLog(const bool bReceivedEntries, const bool bAutoScroll, const bool bWasAtBottom, const bool bScrollRequested) noexcept
{
	return bScrollRequested || (bReceivedEntries && bAutoScroll && bWasAtBottom);
}

[[nodiscard]] constexpr bool IsCommandPrefix(const std::string_view Command, const std::string_view Input) noexcept
{
	if (Input.empty() || Input.size() >= Command.size())
	{
		return false;
	}

	for (std::size_t Index = 0; Index < Input.size(); Index++)
	{
		const char CommandCharacter = Command[Index] >= 'A' && Command[Index] <= 'Z' ? static_cast<char>(Command[Index] + ('a' - 'A')) : Command[Index];
		const char InputCharacter = Input[Index] >= 'A' && Input[Index] <= 'Z' ? static_cast<char>(Input[Index] + ('a' - 'A')) : Input[Index];
		if (CommandCharacter != InputCharacter)
		{
			return false;
		}
	}

	return true;
}

class FOutputLogPanel
{
public:
	[[nodiscard]] std::optional<std::string> Draw(FLogBuffer& Buffer, bool* bOpen, bool& bDocked, std::span<const std::string_view> Commands);
	void Clear(FLogBuffer& Buffer);

private:
	[[nodiscard]] bool Synchronize(FLogBuffer& Buffer);
	void ApplyCommandSuggestion(std::string_view Suggestion);
	void RebuildCommandSuggestions(std::span<const std::string_view> Commands, std::string_view Input);
	void RebuildVisibleEntries();

	FLogCursor Cursor;
	std::vector<FLogEntry> Entries;
	std::vector<std::size_t> VisibleEntries;
	std::vector<std::string> VisibleLines;
	std::vector<std::string> CommandHistory;
	std::vector<std::string_view> CommandSuggestions;
	std::array<char, 256> SearchBuffer = {};
	std::array<char, 512> CommandBuffer = {};
	std::array<bool, static_cast<std::size_t>(ELogLevel::Count)> bLevelVisible = {true, true, true, true, true};
	FLogTextSelection TextSelection;
	int CommandHistoryIndex = -1;
	int CommandSuggestionIndex = -1;
	bool bFilterDirty = true;
	bool bAutoScroll = true;
	bool bColorizeCategories = true;
	bool bPaused = false;
	bool bReclaimCommandFocus = false;
	bool bScrollToBottom = true;
};
}
