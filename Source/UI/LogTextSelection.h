#pragma once

#include <algorithm>
#include <optional>
#include <span>
#include <string>
#include <utility>

namespace ProjectTemplate
{
struct FLogTextPosition
{
	std::size_t Line = 0;
	std::size_t Byte = 0;

	[[nodiscard]] constexpr bool operator==(const FLogTextPosition&) const noexcept = default;
};

class FLogTextSelection
{
public:
	void Begin(const FLogTextPosition Position, const bool bExtend)
	{
		if (!bExtend || !Anchor)
		{
			Anchor = Position;
		}

		Caret = Position;
	}

	void Update(const FLogTextPosition Position)
	{
		if (!Anchor)
		{
			Anchor = Position;
		}

		Caret = Position;
	}

	void Clear()
	{
		Anchor.reset();
		Caret.reset();
	}

	void SelectAll(const std::span<const std::string> Lines)
	{
		if (Lines.empty())
		{
			Clear();
			return;
		}

		Anchor = FLogTextPosition{};
		Caret = FLogTextPosition{ Lines.size() - 1, Lines.back().size() };
	}

	void ClampTo(const std::span<const std::string> Lines)
	{
		if (Lines.empty())
		{
			Clear();
			return;
		}

		const auto ClampPosition = [Lines](FLogTextPosition& Position)
		{
			Position.Line = std::min(Position.Line, Lines.size() - 1);
			Position.Byte = std::min(Position.Byte, Lines[Position.Line].size());
		};

		if (Anchor)
		{
			ClampPosition(*Anchor);
		}

		if (Caret)
		{
			ClampPosition(*Caret);
		}
	}

	[[nodiscard]] bool HasSelection() const noexcept
	{
		return Anchor && Caret && *Anchor != *Caret;
	}

	[[nodiscard]] std::pair<FLogTextPosition, FLogTextPosition> GetOrderedRange() const noexcept
	{
		if (!Anchor || !Caret)
		{
			return {};
		}

		return IsBefore(*Caret, *Anchor) ? std::pair{ *Caret, *Anchor } : std::pair{ *Anchor, *Caret };
	}

	[[nodiscard]] std::string Copy(const std::span<const std::string> Lines) const
	{
		if (!HasSelection() || Lines.empty())
		{
			return {};
		}

		const auto [First, Last] = GetOrderedRange();
		if (First.Line >= Lines.size() || Last.Line >= Lines.size())
		{
			return {};
		}

		if (First.Line == Last.Line)
		{
			return Lines[First.Line].substr(First.Byte, Last.Byte - First.Byte);
		}

		std::string Text = Lines[First.Line].substr(First.Byte);
		for (std::size_t LineIndex = First.Line + 1; LineIndex <= Last.Line; LineIndex++)
		{
			Text.push_back('\n');
			const std::size_t ByteCount = LineIndex == Last.Line ? Last.Byte : Lines[LineIndex].size();
			Text.append(Lines[LineIndex], 0, ByteCount);
		}

		return Text;
	}

private:
	[[nodiscard]] static constexpr bool IsBefore(const FLogTextPosition Left, const FLogTextPosition Right) noexcept
	{
		return Left.Line < Right.Line || (Left.Line == Right.Line && Left.Byte < Right.Byte);
	}

	std::optional<FLogTextPosition> Anchor;
	std::optional<FLogTextPosition> Caret;
};
}
