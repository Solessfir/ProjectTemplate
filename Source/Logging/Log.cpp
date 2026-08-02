#include "Logging/Log.h"

#include <algorithm>
#include <print>

namespace ProjectTemplate
{
FLogBuffer::FLogBuffer(const std::size_t InCapacity)
	: Capacity(std::max<std::size_t>(1, InCapacity))
{
}

std::uint64_t FLogBuffer::Append(const ELogLevel Level, const std::string_view Category, std::string Message)
{
	FLogEntry Entry;
	Entry.Level = Level;
	Entry.Category = Category;
	Entry.Message = std::move(Message);

	const std::scoped_lock Lock(Mutex);
	Entry.ElapsedTime = std::chrono::duration_cast<std::chrono::microseconds>(FClock::now() - StartTime);
	Entry.Sequence = NextSequence++;
	const std::uint64_t Sequence = Entry.Sequence;
	Entries.push_back(std::move(Entry));
	while (Entries.size() > Capacity)
	{
		Entries.pop_front();
	}

	return Sequence;
}

FLogReadResult FLogBuffer::Read(const FLogCursor Cursor) const
{
	const std::scoped_lock Lock(Mutex);
	FLogReadResult Result;
	Result.Cursor = { Generation, NextSequence };
	Result.bReset = Cursor.Generation != Generation;

	if (Result.bReset)
	{
		Result.Entries.assign(Entries.begin(), Entries.end());
		return Result;
	}

	if (Entries.empty())
	{
		return Result;
	}

	const std::uint64_t FirstAvailableSequence = Entries.front().Sequence;
	if (Cursor.NextSequence < FirstAvailableSequence)
	{
		Result.bHistoryTruncated = true;
		Result.Entries.assign(Entries.begin(), Entries.end());
		return Result;
	}

	const auto FirstUnreadEntry = std::lower_bound(
		Entries.begin(),
		Entries.end(),
		Cursor.NextSequence,
		[](const FLogEntry& Entry, const std::uint64_t Sequence)
		{
			return Entry.Sequence < Sequence;
		});
	Result.Entries.assign(FirstUnreadEntry, Entries.end());
	return Result;
}

void FLogBuffer::Clear()
{
	const std::scoped_lock Lock(Mutex);
	Entries.clear();
	Generation++;
}

std::size_t FLogBuffer::GetCapacity() const noexcept
{
	return Capacity;
}

std::size_t FLogBuffer::GetSize() const
{
	const std::scoped_lock Lock(Mutex);
	return Entries.size();
}

namespace Log
{
FLogBuffer& GetBuffer()
{
	static FLogBuffer Buffer;
	return Buffer;
}

void Write(const ELogLevel Level, const std::string_view Category, std::string Message)
{
	std::println(stderr, "[{}] [{}] {}", GetLogLevelName(Level), Category, Message);
	(void)GetBuffer().Append(Level, Category, std::move(Message));
}
}
}
