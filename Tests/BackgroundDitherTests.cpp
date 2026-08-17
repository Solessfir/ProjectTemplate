#include "Rendering/BackgroundDither.h"

#include <array>
#include <doctest/doctest.h>

using ProjectTemplate::BackgroundDitherSize;
using ProjectTemplate::GetBackgroundDitherRank;
using ProjectTemplate::QuantizeBackgroundChannel;

TEST_CASE("Background dither pattern contains every threshold exactly once")
{
	std::array<bool, BackgroundDitherSize * BackgroundDitherSize> SeenRanks = {};
	for (unsigned int Y = 0; Y < BackgroundDitherSize; ++Y)
	{
		for (unsigned int X = 0; X < BackgroundDitherSize; ++X)
		{
			const unsigned int Rank = GetBackgroundDitherRank(X, Y);
			CHECK_FALSE(SeenRanks[Rank]);
			SeenRanks[Rank] = true;
		}
	}

	for (const bool bSeen : SeenRanks)
	{
		CHECK(bSeen);
	}
}

TEST_CASE("Background dither preserves exact UNORM values")
{
	constexpr std::array Values = {0u, 1u, 18u, 127u, 254u, 255u};
	for (const unsigned int Value : Values)
	{
		const float Channel = static_cast<float>(Value) / 255.0f;
		for (unsigned int Y = 0; Y < BackgroundDitherSize; ++Y)
		{
			for (unsigned int X = 0; X < BackgroundDitherSize; ++X)
			{
				CHECK(QuantizeBackgroundChannel(Channel, X, Y) == doctest::Approx(Channel));
			}
		}
	}
}

TEST_CASE("Background dither distributes fractional values between adjacent levels")
{
	constexpr float Channel = 18.5f / 255.0f;
	int LowerCount = 0;
	int UpperCount = 0;
	for (unsigned int Y = 0; Y < BackgroundDitherSize; ++Y)
	{
		for (unsigned int X = 0; X < BackgroundDitherSize; ++X)
		{
			const float Quantized = QuantizeBackgroundChannel(Channel, X, Y);
			LowerCount += Quantized == doctest::Approx(18.0f / 255.0f);
			UpperCount += Quantized == doctest::Approx(19.0f / 255.0f);
		}
	}

	CHECK(LowerCount == 32);
	CHECK(UpperCount == 32);
}
