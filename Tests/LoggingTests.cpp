#include "Logging/Log.h"
#include "UI/OutputLog.h"

#include <algorithm>
#include <cstddef>
#include <doctest/doctest.h>
#include <string>
#include <thread>
#include <vector>

using ProjectTemplate::ELogLevel;
using ProjectTemplate::FLogBuffer;
using ProjectTemplate::FLogCursor;
using ProjectTemplate::FLogTextPosition;
using ProjectTemplate::FLogTextSelection;
using ProjectTemplate::HashLogCategory;
using ProjectTemplate::HasLogLevelColorOverride;
using ProjectTemplate::IsCommandPrefix;
using ProjectTemplate::ShouldScrollOutputLog;

TEST_CASE("Log category hashes remain stable for colorization")
{
	CHECK(HashLogCategory("Renderer") == 1498036510u);
	CHECK(HashLogCategory("Renderer") != HashLogCategory("Application"));
}

TEST_CASE("Warnings and errors override category colorization")
{
	CHECK_FALSE(HasLogLevelColorOverride(ELogLevel::Trace));
	CHECK_FALSE(HasLogLevelColorOverride(ELogLevel::Debug));
	CHECK_FALSE(HasLogLevelColorOverride(ELogLevel::Info));
	CHECK(HasLogLevelColorOverride(ELogLevel::Warning));
	CHECK(HasLogLevelColorOverride(ELogLevel::Error));
}

TEST_CASE("Output Log follows new entries only while auto-scroll owns the tail")
{
	CHECK(ShouldScrollOutputLog(true, true, true, false));
	CHECK_FALSE(ShouldScrollOutputLog(true, true, false, false));
	CHECK_FALSE(ShouldScrollOutputLog(true, false, true, false));
	CHECK(ShouldScrollOutputLog(false, false, false, true));
}

TEST_CASE("Output Log command completion matches partial commands without replacing exact input")
{
	CHECK(IsCommandPrefix("help", "h"));
	CHECK(IsCommandPrefix("help", "HE"));
	CHECK(IsCommandPrefix("components", "com"));
	CHECK_FALSE(IsCommandPrefix("help", "help"));
	CHECK_FALSE(IsCommandPrefix("help", "hello"));
	CHECK_FALSE(IsCommandPrefix("help", ""));
}

TEST_CASE("Output Log text selection copies ranges across lines")
{
	const std::vector<std::string> Lines = {"alpha", "bravo", "charlie"};
	FLogTextSelection Selection;

	Selection.Begin({0, 2}, false);
	Selection.Update({2, 4});
	CHECK(Selection.HasSelection());
	CHECK(Selection.Copy(Lines) == "pha\nbravo\nchar");

	Selection.Begin({2, 4}, false);
	Selection.Update({0, 2});
	CHECK(Selection.Copy(Lines) == "pha\nbravo\nchar");
}

TEST_CASE("Output Log text selection supports extension and select all")
{
	const std::vector<std::string> Lines = {"alpha", "bravo", "charlie"};
	FLogTextSelection Selection;

	Selection.Begin({1, 2}, false);
	Selection.Begin({2, 3}, true);
	CHECK(Selection.Copy(Lines) == "avo\ncha");

	Selection.SelectAll(Lines);
	CHECK(Selection.Copy(Lines) == "alpha\nbravo\ncharlie");

	Selection.ClampTo({Lines.data(), 1});
	CHECK(Selection.Copy({Lines.data(), 1}) == "alpha");
	Selection.Clear();
	CHECK_FALSE(Selection.HasSelection());
}

TEST_CASE("Log buffer returns ordered incremental batches")
{
	FLogBuffer Buffer(8);
	(void)Buffer.Append(ELogLevel::Info, "Application", "Started");
	(void)Buffer.Append(ELogLevel::Warning, "Assets", "Missing preview");

	const auto InitialBatch = Buffer.Read({});
	REQUIRE(InitialBatch.bReset);
	REQUIRE(InitialBatch.Entries.size() == 2);
	CHECK(InitialBatch.Entries[0].Sequence < InitialBatch.Entries[1].Sequence);
	CHECK(InitialBatch.Entries[0].Category == "Application");
	CHECK(InitialBatch.Entries[1].Level == ELogLevel::Warning);

	(void)Buffer.Append(ELogLevel::Debug, "Application", "Frame ready");
	const auto IncrementalBatch = Buffer.Read(InitialBatch.Cursor);
	CHECK_FALSE(IncrementalBatch.bReset);
	CHECK_FALSE(IncrementalBatch.bHistoryTruncated);
	REQUIRE(IncrementalBatch.Entries.size() == 1);
	CHECK(IncrementalBatch.Entries[0].Message == "Frame ready");
}

TEST_CASE("Log buffer reports records lost to bounded history")
{
	FLogBuffer Buffer(3);
	const FLogCursor InitialCursor = Buffer.Read({}).Cursor;
	for (int Index = 0; Index < 5; Index++)
	{
		(void)Buffer.Append(ELogLevel::Info, "Test", std::to_string(Index));
	}

	const auto Batch = Buffer.Read(InitialCursor);
	CHECK(Batch.bHistoryTruncated);
	REQUIRE(Batch.Entries.size() == 3);
	CHECK(Batch.Entries.front().Message == "2");
	CHECK(Batch.Entries.back().Message == "4");
}

TEST_CASE("Clearing the log invalidates existing readers")
{
	FLogBuffer Buffer(4);
	(void)Buffer.Append(ELogLevel::Info, "Test", "Before clear");
	const FLogCursor Cursor = Buffer.Read({}).Cursor;

	Buffer.Clear();
	(void)Buffer.Append(ELogLevel::Info, "Test", "After clear");
	const auto Batch = Buffer.Read(Cursor);

	CHECK(Batch.bReset);
	REQUIRE(Batch.Entries.size() == 1);
	CHECK(Batch.Entries[0].Message == "After clear");
}

TEST_CASE("Concurrent log producers retain a strictly ordered bounded history")
{
	constexpr std::size_t ThreadCount = 4;
	constexpr std::size_t EntriesPerThread = 128;
	constexpr std::size_t Capacity = 256;
	FLogBuffer Buffer(Capacity);
	std::vector<std::thread> Threads;
	Threads.reserve(ThreadCount);

	for (std::size_t ThreadIndex = 0; ThreadIndex < ThreadCount; ThreadIndex++)
	{
		Threads.emplace_back([&Buffer, ThreadIndex]
		                     {
			                     for (std::size_t EntryIndex = 0; EntryIndex < EntriesPerThread; EntryIndex++)
			                     {
				                     (void)Buffer.Append(ELogLevel::Debug, "Worker", std::format("{}:{}", ThreadIndex, EntryIndex));
			                     }
		                     });
	}

	for (std::thread& Thread : Threads)
	{
		Thread.join();
	}

	const auto Batch = Buffer.Read({});
	REQUIRE(Batch.Entries.size() == Capacity);
	CHECK(std::ranges::is_sorted(Batch.Entries, {}, &ProjectTemplate::FLogEntry::Sequence));
	CHECK(std::ranges::adjacent_find(Batch.Entries, {}, &ProjectTemplate::FLogEntry::Sequence) == Batch.Entries.end());
}
