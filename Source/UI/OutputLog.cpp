#include "UI/OutputLog.h"

#include "UI/ApplicationTheme.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <format>
#include <iterator>
#include <string_view>

namespace ProjectTemplate
{
namespace
{
constexpr std::array CategoryColors = {
	IM_COL32(126, 200, 255, 255),
	IM_COL32(142, 220, 182, 255),
	IM_COL32(244, 202, 128, 255),
	IM_COL32(203, 166, 255, 255),
	IM_COL32(255, 157, 170, 255),
	IM_COL32(115, 218, 224, 255),
	IM_COL32(192, 215, 128, 255),
	IM_COL32(240, 166, 219, 255),
	IM_COL32(166, 184, 255, 255),
	IM_COL32(235, 186, 151, 255),
	IM_COL32(135, 210, 154, 255),
	IM_COL32(225, 168, 255, 255)
};
constexpr std::size_t MaximumCommandHistory = 64;

[[nodiscard]] constexpr char FoldAscii(const char Character) noexcept
{
	return Character >= 'A' && Character <= 'Z' ? static_cast<char>(Character + ('a' - 'A')) : Character;
}

[[nodiscard]] bool ContainsCaseInsensitive(const std::string_view Text, const std::string_view Search)
{
	if (Search.empty())
	{
		return true;
	}

	return std::search(
		Text.begin(),
		Text.end(),
		Search.begin(),
		Search.end(),
		[](const char Left, const char Right)
		{
			return FoldAscii(Left) == FoldAscii(Right);
		}) != Text.end();
}

[[nodiscard]] bool MatchesSearch(const FLogEntry& Entry, const std::string_view Search)
{
	return ContainsCaseInsensitive(Entry.Category, Search) ||
		ContainsCaseInsensitive(Entry.Message, Search) ||
		ContainsCaseInsensitive(GetLogLevelName(Entry.Level), Search);
}

[[nodiscard]] ImU32 ResolveCategoryColor(const std::string_view Category)
{
	return CategoryColors[HashLogCategory(Category) % CategoryColors.size()];
}

[[nodiscard]] ImU32 ResolveLevelColor(const ELogLevel Level)
{
	switch (Level)
	{
	case ELogLevel::Trace:
		return Theme::Colors::TextMuted;
	case ELogLevel::Debug:
		return Theme::Colors::TextSecondary;
	case ELogLevel::Info:
		return Theme::Colors::TextPrimary;
	case ELogLevel::Warning:
		return IM_COL32(246, 196, 101, 255);
	case ELogLevel::Error:
		return IM_COL32(255, 125, 125, 255);
	case ELogLevel::Count:
		break;
	}

	return Theme::Colors::TextPrimary;
}

[[nodiscard]] std::string_view TrimWhitespace(std::string_view Text)
{
	while (!Text.empty() && std::isspace(static_cast<unsigned char>(Text.front())) != 0)
	{
		Text.remove_prefix(1);
	}

	while (!Text.empty() && std::isspace(static_cast<unsigned char>(Text.back())) != 0)
	{
		Text.remove_suffix(1);
	}

	return Text;
}

[[nodiscard]] std::string FormatEntry(const FLogEntry& Entry)
{
	const double Seconds = static_cast<double>(Entry.ElapsedTime.count()) / 1'000'000.0;
	return std::format("[{:.3f}] [{}] [{}] {}", Seconds, GetLogLevelName(Entry.Level), Entry.Category, Entry.Message);
}

[[nodiscard]] std::string JoinLines(const std::span<const std::string> Lines)
{
	std::string Text;
	for (std::size_t LineIndex = 0; LineIndex < Lines.size(); LineIndex++)
	{
		if (LineIndex > 0)
		{
			Text.push_back('\n');
		}

		Text += Lines[LineIndex];
	}

	return Text;
}

[[nodiscard]] std::size_t GetNextUtf8Boundary(const std::string_view Text, const std::size_t ByteOffset) noexcept
{
	if (ByteOffset >= Text.size())
	{
		return Text.size();
	}

	const unsigned char FirstByte = static_cast<unsigned char>(Text[ByteOffset]);
	std::size_t CodePointSize = 1;
	if ((FirstByte & 0xE0u) == 0xC0u)
	{
		CodePointSize = 2;
	}
	else if ((FirstByte & 0xF0u) == 0xE0u)
	{
		CodePointSize = 3;
	}
	else if ((FirstByte & 0xF8u) == 0xF0u)
	{
		CodePointSize = 4;
	}

	return std::min(Text.size(), ByteOffset + CodePointSize);
}

[[nodiscard]] float MeasureTextPrefix(const std::string_view Text, const std::size_t ByteCount)
{
	const std::size_t ClampedByteCount = std::min(ByteCount, Text.size());
	return ImGui::CalcTextSize(Text.data(), Text.data() + ClampedByteCount, false).x;
}

[[nodiscard]] std::size_t FindByteAtX(const std::string_view Text, const float LocalX)
{
	if (LocalX <= 0.0f)
	{
		return 0;
	}

	float TextX = 0.0f;
	for (std::size_t ByteOffset = 0; ByteOffset < Text.size();)
	{
		const std::size_t NextByteOffset = GetNextUtf8Boundary(Text, ByteOffset);
		const float CharacterWidth = ImGui::CalcTextSize(Text.data() + ByteOffset, Text.data() + NextByteOffset, false).x;
		if (LocalX < TextX + CharacterWidth * 0.5f)
		{
			return ByteOffset;
		}

		TextX += CharacterWidth;
		ByteOffset = NextByteOffset;
	}

	return Text.size();
}

[[nodiscard]] FLogTextPosition HitTestText(const std::span<const std::string> Lines, const ImVec2 TextOrigin, const float LineHeight, const ImVec2 MousePosition)
{
	if (Lines.empty())
	{
		return {};
	}

	std::size_t LineIndex = 0;
	if (MousePosition.y > TextOrigin.y)
	{
		LineIndex = static_cast<std::size_t>((MousePosition.y - TextOrigin.y) / LineHeight);
		LineIndex = std::min(LineIndex, Lines.size() - 1);
	}

	const float LocalX = MousePosition.x - TextOrigin.x;
	return { LineIndex, FindByteAtX(Lines[LineIndex], LocalX) };
}
}

bool FOutputLogPanel::Synchronize(FLogBuffer& Buffer)
{
	FLogReadResult Result = Buffer.Read(Cursor);
	Cursor = Result.Cursor;
	if (Result.bReset || Result.bHistoryTruncated)
	{
		Entries.clear();
		TextSelection.Clear();
	}

	if (Result.Entries.empty())
	{
		if (Result.bReset || Result.bHistoryTruncated)
		{
			bFilterDirty = true;
		}
		return false;
	}

	Entries.insert(
		Entries.end(),
		std::make_move_iterator(Result.Entries.begin()),
		std::make_move_iterator(Result.Entries.end()));
	if (Entries.size() > Buffer.GetCapacity())
	{
		const std::size_t RemoveCount = Entries.size() - Buffer.GetCapacity();
		Entries.erase(Entries.begin(), Entries.begin() + static_cast<std::ptrdiff_t>(RemoveCount));
		TextSelection.Clear();
	}

	bFilterDirty = true;
	return true;
}

void FOutputLogPanel::RebuildVisibleEntries()
{
	VisibleEntries.clear();
	VisibleEntries.reserve(Entries.size());
	VisibleLines.clear();
	VisibleLines.reserve(Entries.size());
	const std::string_view Search = SearchBuffer.data();
	for (std::size_t Index = 0; Index < Entries.size(); Index++)
	{
		const FLogEntry& Entry = Entries[Index];
		const std::size_t LevelIndex = static_cast<std::size_t>(Entry.Level);
		if (LevelIndex < bLevelVisible.size() && bLevelVisible[LevelIndex] && MatchesSearch(Entry, Search))
		{
			VisibleEntries.push_back(Index);
			VisibleLines.push_back(FormatEntry(Entry));
		}
	}

	TextSelection.ClampTo(VisibleLines);
	bFilterDirty = false;
}

void FOutputLogPanel::Clear(FLogBuffer& Buffer)
{
	Buffer.Clear();
	Cursor = {};
	Entries.clear();
	VisibleEntries.clear();
	VisibleLines.clear();
	TextSelection.Clear();
	bFilterDirty = false;
	bScrollToBottom = false;
}

void FOutputLogPanel::ApplyCommandSuggestion(const std::string_view Suggestion)
{
	const std::size_t CopyLength = std::min(Suggestion.size(), CommandBuffer.size() - 1);
	std::fill(CommandBuffer.begin(), CommandBuffer.end(), '\0');
	std::copy_n(Suggestion.data(), CopyLength, CommandBuffer.data());
	CommandSuggestions.clear();
	CommandSuggestionIndex = -1;
	bReclaimCommandFocus = true;
}

void FOutputLogPanel::RebuildCommandSuggestions(const std::span<const std::string_view> Commands, const std::string_view Input)
{
	CommandSuggestions.clear();
	for (const std::string_view Command : Commands)
	{
		if (IsCommandPrefix(Command, Input))
		{
			CommandSuggestions.push_back(Command);
		}
	}

	if (CommandSuggestions.empty())
	{
		CommandSuggestionIndex = -1;
	}
	else
	{
		CommandSuggestionIndex = std::clamp(CommandSuggestionIndex, 0, static_cast<int>(CommandSuggestions.size()) - 1);
	}
}

std::optional<std::string> FOutputLogPanel::Draw(FLogBuffer& Buffer, bool* const bOpen, bool& bDocked, const std::span<const std::string_view> Commands)
{
	const bool bReceivedEntries = !bPaused && Synchronize(Buffer);
	ImGui::SetNextWindowSize({ 900.0f, 320.0f }, ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Output Log", bOpen))
	{
		bDocked = ImGui::IsWindowDocked();
		ImGui::End();
		return std::nullopt;
	}
	bDocked = ImGui::IsWindowDocked();

	bool bCopyRequested = false;
	if (ImGui::Button("Clear"))
	{
		Clear(Buffer);
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy"))
	{
		bCopyRequested = true;
	}
	ImGui::SameLine();
	if (ImGui::Button("Filter"))
	{
		ImGui::OpenPopup("OutputLogFilter");
	}
	if (ImGui::BeginPopup("OutputLogFilter"))
	{
		for (std::size_t LevelIndex = 0; LevelIndex < bLevelVisible.size(); LevelIndex++)
		{
			const ELogLevel Level = static_cast<ELogLevel>(LevelIndex);
			if (ImGui::MenuItem(GetLogLevelName(Level).data(), nullptr, &bLevelVisible[LevelIndex]))
			{
				bFilterDirty = true;
				TextSelection.Clear();
			}
		}
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	if (ImGui::Button("Options"))
	{
		ImGui::OpenPopup("OutputLogOptions");
	}
	if (ImGui::BeginPopup("OutputLogOptions"))
	{
		ImGui::MenuItem("Auto-scroll", nullptr, &bAutoScroll);
		ImGui::MenuItem("Pause", nullptr, &bPaused);
		ImGui::MenuItem("Colorize categories", nullptr, &bColorizeCategories);
		ImGui::EndPopup();
	}

	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##OutputLogSearch", "Search messages, categories, and verbosity", SearchBuffer.data(), SearchBuffer.size()))
	{
		bFilterDirty = true;
		TextSelection.Clear();
	}

	if (bFilterDirty)
	{
		RebuildVisibleEntries();
	}

	if (bCopyRequested)
	{
		const std::string Clipboard = TextSelection.HasSelection() ? TextSelection.Copy(VisibleLines) : JoinLines(VisibleLines);
		ImGui::SetClipboardText(Clipboard.c_str());
	}

	const float InterfaceScale = ImGui::GetFontSize() / 15.0f;
	const float FooterHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 8.0f * InterfaceScale, 4.0f * InterfaceScale });
	if (ImGui::BeginChild("OutputLogEntries", { 0.0f, -FooterHeight }, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_HorizontalScrollbar))
	{
		const bool bWasAtBottom = ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f;
		const bool bShouldScrollToBottom = !VisibleEntries.empty() && ShouldScrollOutputLog(bReceivedEntries, bAutoScroll, bWasAtBottom, bScrollToBottom);
		const float LineHeight = ImGui::GetFontSize() + 3.0f * InterfaceScale;
		const float TextOffsetY = (LineHeight - ImGui::GetFontSize()) * 0.5f;
		const ImVec2 AvailableSize = ImGui::GetContentRegionAvail();
		float ContentWidth = AvailableSize.x;
		for (const std::string& Line : VisibleLines)
		{
			ContentWidth = std::max(ContentWidth, ImGui::CalcTextSize(Line.data(), Line.data() + Line.size(), false).x + 8.0f * InterfaceScale);
		}

		const float ContentHeight = std::max(AvailableSize.y, static_cast<float>(VisibleLines.size()) * LineHeight);
		const ImVec2 TextOrigin = ImGui::GetCursorScreenPos();
		(void)ImGui::InvisibleButton("##OutputLogText", { ContentWidth, ContentHeight }, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_EnableNav);
		if (ImGui::IsItemActivated())
		{
			TextSelection.Begin(HitTestText(VisibleLines, TextOrigin, LineHeight, ImGui::GetMousePos()), ImGui::GetIO().KeyShift);
		}

		if (ImGui::IsItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			const ImVec2 MousePosition = ImGui::GetMousePos();
			TextSelection.Update(HitTestText(VisibleLines, TextOrigin, LineHeight, MousePosition));
			const ImVec2 WindowPosition = ImGui::GetWindowPos();
			const ImVec2 ContentRegionMin = ImGui::GetWindowContentRegionMin();
			const ImVec2 ContentRegionMax = ImGui::GetWindowContentRegionMax();
			const float ScrollStep = 360.0f * ImGui::GetIO().DeltaTime;
			if (MousePosition.y < WindowPosition.y + ContentRegionMin.y)
			{
				ImGui::SetScrollY(std::max(0.0f, ImGui::GetScrollY() - ScrollStep));
			}
			else if (MousePosition.y > WindowPosition.y + ContentRegionMax.y)
			{
				ImGui::SetScrollY(ImGui::GetScrollY() + ScrollStep);
			}
		}

		if (bShouldScrollToBottom)
		{
			ImGui::SetScrollHereY(1.0f);
		}

		if (ImGui::IsWindowFocused() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_A))
		{
			TextSelection.SelectAll(VisibleLines);
		}

		if (ImGui::IsWindowFocused() && TextSelection.HasSelection() && ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_C))
		{
			const std::string Clipboard = TextSelection.Copy(VisibleLines);
			ImGui::SetClipboardText(Clipboard.c_str());
		}

		const auto [SelectionFirst, SelectionLast] = TextSelection.GetOrderedRange();
		const bool bHasSelection = TextSelection.HasSelection();
		const ImU32 SelectionColor = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);
		const float WindowTop = ImGui::GetWindowPos().y;
		const float WindowBottom = WindowTop + ImGui::GetWindowSize().y;
		const std::size_t FirstVisibleLine = TextOrigin.y < WindowTop ? std::min(VisibleLines.size(), static_cast<std::size_t>((WindowTop - TextOrigin.y) / LineHeight)) : 0;
		const std::size_t LastVisibleLine = std::min(VisibleLines.size(), static_cast<std::size_t>(std::max(0.0f, (WindowBottom - TextOrigin.y) / LineHeight)) + 1);
		ImDrawList* const DrawList = ImGui::GetWindowDrawList();
		for (std::size_t LineIndex = FirstVisibleLine; LineIndex < LastVisibleLine; LineIndex++)
		{
			const std::string& Line = VisibleLines[LineIndex];
			const float LineY = TextOrigin.y + static_cast<float>(LineIndex) * LineHeight;
			if (bHasSelection && LineIndex >= SelectionFirst.Line && LineIndex <= SelectionLast.Line)
			{
				const std::size_t FirstByte = LineIndex == SelectionFirst.Line ? SelectionFirst.Byte : 0;
				const std::size_t LastByte = LineIndex == SelectionLast.Line ? SelectionLast.Byte : Line.size();
				const float SelectionX = TextOrigin.x + MeasureTextPrefix(Line, FirstByte);
				float SelectionEndX = TextOrigin.x + MeasureTextPrefix(Line, LastByte);
				if (LineIndex < SelectionLast.Line)
				{
					SelectionEndX += ImGui::GetFontSize() * 0.35f;
				}

				DrawList->AddRectFilled({ SelectionX, LineY }, { std::max(SelectionX + 1.0f, SelectionEndX), LineY + LineHeight }, SelectionColor);
			}

			const FLogEntry& Entry = Entries[VisibleEntries[LineIndex]];
			const ImU32 CategoryColor = ResolveCategoryColor(Entry.Category);
			const ImU32 LevelColor = ResolveLevelColor(Entry.Level);
			const ImU32 LineColor = !HasLogLevelColorOverride(Entry.Level) && bColorizeCategories ? CategoryColor : LevelColor;
			DrawList->AddText({ TextOrigin.x, LineY + TextOffsetY }, LineColor, Line.data(), Line.data() + Line.size());
		}

		bScrollToBottom = false;
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();

	std::optional<std::string> SubmittedCommand;
	struct FCommandInputContext
	{
		FOutputLogPanel* Panel;
		std::span<const std::string_view> Commands;
	};
	FCommandInputContext CommandInputContext = { this, Commands };
	const auto HistoryCallback = [](ImGuiInputTextCallbackData* const Data)
	{
		FCommandInputContext& Context = *static_cast<FCommandInputContext*>(Data->UserData);
		FOutputLogPanel& Panel = *Context.Panel;
		if (Data->EventFlag == ImGuiInputTextFlags_CallbackEdit)
		{
			Panel.CommandSuggestionIndex = 0;
			Panel.RebuildCommandSuggestions(Context.Commands, Data->Buf);
			return 0;
		}

		if (Data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
		{
			if (Panel.CommandSuggestionIndex >= 0 && Panel.CommandSuggestionIndex < static_cast<int>(Panel.CommandSuggestions.size()))
			{
				const std::string_view Suggestion = Panel.CommandSuggestions[static_cast<std::size_t>(Panel.CommandSuggestionIndex)];
				Data->DeleteChars(0, Data->BufTextLen);
				Data->InsertChars(0, Suggestion.data(), Suggestion.data() + Suggestion.size());
				Panel.CommandSuggestions.clear();
				Panel.CommandSuggestionIndex = -1;
			}
			return 0;
		}

		if (!Panel.CommandSuggestions.empty())
		{
			if (Data->EventKey == ImGuiKey_UpArrow)
			{
				Panel.CommandSuggestionIndex = std::max(0, Panel.CommandSuggestionIndex - 1);
			}
			else if (Data->EventKey == ImGuiKey_DownArrow)
			{
				Panel.CommandSuggestionIndex = std::min(static_cast<int>(Panel.CommandSuggestions.size()) - 1, Panel.CommandSuggestionIndex + 1);
			}
			return 0;
		}

		if (Panel.CommandHistory.empty())
		{
			return 0;
		}

		if (Data->EventKey == ImGuiKey_UpArrow)
		{
			if (Panel.CommandHistoryIndex < 0)
			{
				Panel.CommandHistoryIndex = static_cast<int>(Panel.CommandHistory.size()) - 1;
			}
			else if (Panel.CommandHistoryIndex > 0)
			{
				Panel.CommandHistoryIndex--;
			}
		}
		else if (Data->EventKey == ImGuiKey_DownArrow && Panel.CommandHistoryIndex >= 0)
		{
			Panel.CommandHistoryIndex++;
			if (Panel.CommandHistoryIndex >= static_cast<int>(Panel.CommandHistory.size()))
			{
				Panel.CommandHistoryIndex = -1;
			}
		}

		const std::string_view Command = Panel.CommandHistoryIndex >= 0 ? Panel.CommandHistory[static_cast<std::size_t>(Panel.CommandHistoryIndex)] : std::string_view{};
		Data->DeleteChars(0, Data->BufTextLen);
		if (!Command.empty())
		{
			Data->InsertChars(0, Command.data(), Command.data() + Command.size());
		}
		Panel.CommandSuggestions.clear();
		Panel.CommandSuggestionIndex = -1;
		return 0;
	};

	const auto SubmitCommand = [this, &SubmittedCommand]
	{
		const std::string_view Command = TrimWhitespace(CommandBuffer.data());
		if (Command.empty())
		{
			return;
		}

		SubmittedCommand = std::string(Command);
		if (CommandHistory.empty() || CommandHistory.back() != Command)
		{
			CommandHistory.emplace_back(Command);
			if (CommandHistory.size() > MaximumCommandHistory)
			{
				CommandHistory.erase(CommandHistory.begin());
			}
		}
		CommandHistoryIndex = -1;
		CommandSuggestions.clear();
		CommandSuggestionIndex = -1;
		CommandBuffer[0] = '\0';
		bReclaimCommandFocus = true;
		bScrollToBottom = bAutoScroll;
	};

	constexpr ImGuiInputTextFlags CommandFlags =
		ImGuiInputTextFlags_EnterReturnsTrue |
		ImGuiInputTextFlags_CallbackCompletion |
		ImGuiInputTextFlags_CallbackEdit |
		ImGuiInputTextFlags_CallbackHistory;
	constexpr char SubmitLabel[] = "Submit";
	const float SubmitButtonWidth = ImGui::CalcTextSize(SubmitLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::SetNextItemWidth(-(SubmitButtonWidth + ImGui::GetStyle().ItemSpacing.x));
	if (ImGui::InputTextWithHint("##OutputLogCommand", "Enter command, or type help", CommandBuffer.data(), CommandBuffer.size(), CommandFlags, HistoryCallback, &CommandInputContext))
	{
		SubmitCommand();
	}
	const ImVec2 CommandInputMin = ImGui::GetItemRectMin();
	const ImVec2 CommandInputMax = ImGui::GetItemRectMax();
	if (bReclaimCommandFocus)
	{
		ImGui::SetKeyboardFocusHere(-1);
		bReclaimCommandFocus = false;
	}
	ImGui::SameLine();
	ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, { 0.5f, 0.5f });
	if (ImGui::Button(SubmitLabel, { SubmitButtonWidth, 0.0f }))
	{
		SubmitCommand();
	}
	ImGui::PopStyleVar();

	std::optional<std::string_view> ClickedSuggestion;
	if (!CommandSuggestions.empty())
	{
		const ImGuiViewport* const Viewport = ImGui::GetMainViewport();
		const std::size_t VisibleSuggestionCount = std::min<std::size_t>(6, CommandSuggestions.size());
		const float PopupPadding = 4.0f * InterfaceScale;
		const float PopupHeight = static_cast<float>(VisibleSuggestionCount) * ImGui::GetFrameHeight() + PopupPadding * 2.0f;
		const float PopupY = std::max(Viewport->WorkPos.y, CommandInputMin.y - PopupHeight);
		ImGui::SetNextWindowPos({ CommandInputMin.x, PopupY }, ImGuiCond_Always);
		ImGui::SetNextWindowSize({ CommandInputMax.x - CommandInputMin.x, PopupHeight }, ImGuiCond_Always);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { PopupPadding, PopupPadding });
		constexpr ImGuiWindowFlags SuggestionFlags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoSavedSettings;
		if (ImGui::Begin("Command suggestions###OutputLogCommandSuggestions", nullptr, SuggestionFlags))
		{
			for (std::size_t SuggestionIndex = 0; SuggestionIndex < CommandSuggestions.size(); SuggestionIndex++)
			{
				const std::string_view Suggestion = CommandSuggestions[SuggestionIndex];
				ImGui::PushID(static_cast<int>(SuggestionIndex));
				if (ImGui::Selectable(Suggestion.data(), CommandSuggestionIndex == static_cast<int>(SuggestionIndex)))
				{
					ClickedSuggestion = Suggestion;
				}
				if (ImGui::IsItemHovered())
				{
					CommandSuggestionIndex = static_cast<int>(SuggestionIndex);
				}
				ImGui::PopID();
			}
		}
		ImGui::End();
		ImGui::PopStyleVar();
	}

	if (ClickedSuggestion)
	{
		ApplyCommandSuggestion(*ClickedSuggestion);
	}

	ImGui::End();
	return SubmittedCommand;
}
}
