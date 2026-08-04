#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <format>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ProjectTemplate
{
enum class ELogLevel : std::uint8_t
{
	Trace,
	Debug,
	Info,
	Warning,
	Error,
	Count
};

struct FLogEntry
{
	std::uint64_t Sequence = 0;
	std::chrono::microseconds ElapsedTime = {};
	ELogLevel Level = ELogLevel::Info;
	std::string Category;
	std::string Message;
};

struct FLogCursor
{
	std::uint64_t Generation = 0;
	std::uint64_t NextSequence = 0;
};

struct FLogReadResult
{
	FLogCursor Cursor;
	std::vector<FLogEntry> Entries;
	bool bReset = false;
	bool bHistoryTruncated = false;
};

class FLogBuffer
{
public:
	explicit FLogBuffer(std::size_t Capacity = 8192);

	[[nodiscard]] std::uint64_t Append(ELogLevel Level, std::string_view Category, std::string Message);
	[[nodiscard]] FLogReadResult Read(FLogCursor Cursor) const;
	void Clear();

	[[nodiscard]] std::size_t GetCapacity() const noexcept;
	[[nodiscard]] std::size_t GetSize() const;

private:
	using FClock = std::chrono::steady_clock;

	mutable std::mutex Mutex;
	std::deque<FLogEntry> Entries;
	FClock::time_point StartTime = FClock::now();
	std::size_t Capacity = 0;
	std::uint64_t Generation = 1;
	std::uint64_t NextSequence = 1;
};

[[nodiscard]] constexpr std::string_view GetLogLevelName(const ELogLevel Level) noexcept
{
	switch (Level)
	{
		case ELogLevel::Trace:
			return "Trace";
		case ELogLevel::Debug:
			return "Debug";
		case ELogLevel::Info:
			return "Info";
		case ELogLevel::Warning:
			return "Warning";
		case ELogLevel::Error:
			return "Error";
		case ELogLevel::Count:
			break;
	}

	return "Unknown";
}

namespace Log
{
[[nodiscard]] FLogBuffer& GetBuffer();
void Write(ELogLevel Level, std::string_view Category, std::string Message);

template <typename... TArguments>
void Write(const ELogLevel Level, const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(Level, Category, std::format(Format, std::forward<TArguments>(Arguments)...));
}

template <typename... TArguments>
void Trace(const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(ELogLevel::Trace, Category, Format, std::forward<TArguments>(Arguments)...);
}

template <typename... TArguments>
void Debug(const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(ELogLevel::Debug, Category, Format, std::forward<TArguments>(Arguments)...);
}

template <typename... TArguments>
void Info(const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(ELogLevel::Info, Category, Format, std::forward<TArguments>(Arguments)...);
}

template <typename... TArguments>
void Warning(const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(ELogLevel::Warning, Category, Format, std::forward<TArguments>(Arguments)...);
}

template <typename... TArguments>
void Error(const std::string_view Category, const std::format_string<TArguments...> Format, TArguments&&... Arguments)
{
	Write(ELogLevel::Error, Category, Format, std::forward<TArguments>(Arguments)...);
}
}
}
