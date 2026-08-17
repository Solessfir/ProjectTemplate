#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

namespace ProjectTemplate
{
inline constexpr int BackgroundDitherSize = 8;
inline constexpr std::array<std::uint8_t, BackgroundDitherSize * BackgroundDitherSize> BackgroundDitherPattern = {
    0, 48, 12, 60, 3, 51, 15, 63, 32, 16, 44, 28, 35, 19, 47, 31, 8, 56, 4, 52, 11, 59, 7, 55, 40, 24, 36, 20, 43, 27, 39, 23, 2, 50, 14, 62, 1, 49, 13, 61, 34, 18, 46, 30, 33, 17, 45, 29, 10, 58, 6, 54, 9, 57, 5, 53, 42, 26, 38, 22, 41, 25, 37, 21};

[[nodiscard]] constexpr std::uint8_t GetBackgroundDitherRank(const unsigned int X, const unsigned int Y) noexcept
{
	return BackgroundDitherPattern[(Y % BackgroundDitherSize) * BackgroundDitherSize + X % BackgroundDitherSize];
}

[[nodiscard]] constexpr float QuantizeBackgroundChannel(const float Value, const unsigned int X, const unsigned int Y) noexcept
{
	const float ScaledValue = std::clamp(Value, 0.0f, 1.0f) * 255.0f;
	const auto LowerValue = static_cast<unsigned int>(ScaledValue);
	const float Fraction = ScaledValue - static_cast<float>(LowerValue);
	const float Threshold = (static_cast<float>(GetBackgroundDitherRank(X, Y)) + 0.5f) / 64.0f;
	const unsigned int QuantizedValue = std::min(LowerValue + static_cast<unsigned int>(Fraction >= Threshold), 255u);
	return static_cast<float>(QuantizedValue) / 255.0f;
}
}
