#pragma once

#include <expected>
#include <memory>
#include <string>

namespace ProjectTemplate
{
struct FBackgroundGradient
{
	float AccentRed = 0.0f;
	float AccentGreen = 0.0f;
	float AccentBlue = 0.0f;
	float HeightRatio = 0.0f;
	float Intensity = 0.0f;
	float TrailingIntensityRatio = 0.0f;
};

class FBackgroundRenderer
{
public:
	FBackgroundRenderer();
	~FBackgroundRenderer();

	FBackgroundRenderer(const FBackgroundRenderer&) = delete;
	FBackgroundRenderer& operator=(const FBackgroundRenderer&) = delete;

	[[nodiscard]] std::expected<void, std::string> Initialize();
	void Shutdown();
	void Render(int Width, int Height, const FBackgroundGradient& Gradient) const;

private:
	struct FImplementation;
	std::unique_ptr<FImplementation> Implementation;
};
}
