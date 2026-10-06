#include "Rendering/BackgroundRenderer.h"

#include "Rendering/BackgroundDither.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <expected>
#include <format>
#include <string>
#include <vector>

namespace ProjectTemplate
{
namespace
{
constexpr GLenum VertexShaderType = 0x8B31;
constexpr GLenum FragmentShaderType = 0x8B30;
constexpr GLenum CompileStatus = 0x8B81;
constexpr GLenum LinkStatus = 0x8B82;
constexpr GLenum InfoLogLength = 0x8B84;
constexpr GLenum Framebuffer = 0x8D40;
constexpr GLenum ColorAttachment = 0x8CE0;
constexpr GLenum FramebufferComplete = 0x8CD5;
constexpr GLenum Rgba8 = 0x8058;
constexpr GLenum ClampToEdge = 0x812F;
constexpr GLenum Texture0 = 0x84C0;
constexpr GLenum ActiveTexture = 0x84E0;
constexpr std::size_t KawaseLevelCount = 6;

using FCreateShader = GLuint(APIENTRY*)(GLenum);
using FShaderSource = void(APIENTRY*)(GLuint, GLsizei, const char* const*, const GLint*);
using FCompileShader = void(APIENTRY*)(GLuint);
using FGetShaderiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using FGetShaderInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, char*);
using FDeleteShader = void(APIENTRY*)(GLuint);
using FCreateProgram = GLuint(APIENTRY*)();
using FAttachShader = void(APIENTRY*)(GLuint, GLuint);
using FLinkProgram = void(APIENTRY*)(GLuint);
using FGetProgramiv = void(APIENTRY*)(GLuint, GLenum, GLint*);
using FGetProgramInfoLog = void(APIENTRY*)(GLuint, GLsizei, GLsizei*, char*);
using FDeleteProgram = void(APIENTRY*)(GLuint);
using FUseProgram = void(APIENTRY*)(GLuint);
using FGetUniformLocation = GLint(APIENTRY*)(GLuint, const char*);
using FUniform1f = void(APIENTRY*)(GLint, GLfloat);
using FUniform1iv = void(APIENTRY*)(GLint, GLsizei, const GLint*);
using FUniform3f = void(APIENTRY*)(GLint, GLfloat, GLfloat, GLfloat);
using FUniform2f = void(APIENTRY*)(GLint, GLfloat, GLfloat);
using FActiveTexture = void(APIENTRY*)(GLenum);
using FGenFramebuffers = void(APIENTRY*)(GLsizei, GLuint*);
using FDeleteFramebuffers = void(APIENTRY*)(GLsizei, const GLuint*);
using FBindFramebuffer = void(APIENTRY*)(GLenum, GLuint);
using FFramebufferTexture2D = void(APIENTRY*)(GLenum, GLenum, GLenum, GLuint, GLint);
using FCheckFramebufferStatus = GLenum(APIENTRY*)(GLenum);
using FGenVertexArrays = void(APIENTRY*)(GLsizei, GLuint*);
using FDeleteVertexArrays = void(APIENTRY*)(GLsizei, const GLuint*);
using FBindVertexArray = void(APIENTRY*)(GLuint);

struct FOpenGlApi
{
	FCreateShader CreateShader = nullptr;
	FShaderSource ShaderSource = nullptr;
	FCompileShader CompileShader = nullptr;
	FGetShaderiv GetShaderiv = nullptr;
	FGetShaderInfoLog GetShaderInfoLog = nullptr;
	FDeleteShader DeleteShader = nullptr;
	FCreateProgram CreateProgram = nullptr;
	FAttachShader AttachShader = nullptr;
	FLinkProgram LinkProgram = nullptr;
	FGetProgramiv GetProgramiv = nullptr;
	FGetProgramInfoLog GetProgramInfoLog = nullptr;
	FDeleteProgram DeleteProgram = nullptr;
	FUseProgram UseProgram = nullptr;
	FGetUniformLocation GetUniformLocation = nullptr;
	FUniform1f Uniform1f = nullptr;
	FUniform1iv Uniform1iv = nullptr;
	FUniform3f Uniform3f = nullptr;
	FUniform2f Uniform2f = nullptr;
	FActiveTexture ActiveTexture = nullptr;
	FGenFramebuffers GenFramebuffers = nullptr;
	FDeleteFramebuffers DeleteFramebuffers = nullptr;
	FBindFramebuffer BindFramebuffer = nullptr;
	FFramebufferTexture2D FramebufferTexture2D = nullptr;
	FCheckFramebufferStatus CheckFramebufferStatus = nullptr;
	FGenVertexArrays GenVertexArrays = nullptr;
	FDeleteVertexArrays DeleteVertexArrays = nullptr;
	FBindVertexArray BindVertexArray = nullptr;
};

template <typename T>
[[nodiscard]] T LoadOpenGlFunction(const char* const Name)
{
	// OpenGL exposes typed entry points through one generic address function, so this cast is the platform API boundary.
	return reinterpret_cast<T>(glfwGetProcAddress(Name));
}

[[nodiscard]] std::expected<FOpenGlApi, std::string> LoadOpenGlApi()
{
	FOpenGlApi Api = {
	    .CreateShader = LoadOpenGlFunction<FCreateShader>("glCreateShader"),
	    .ShaderSource = LoadOpenGlFunction<FShaderSource>("glShaderSource"),
	    .CompileShader = LoadOpenGlFunction<FCompileShader>("glCompileShader"),
	    .GetShaderiv = LoadOpenGlFunction<FGetShaderiv>("glGetShaderiv"),
	    .GetShaderInfoLog = LoadOpenGlFunction<FGetShaderInfoLog>("glGetShaderInfoLog"),
	    .DeleteShader = LoadOpenGlFunction<FDeleteShader>("glDeleteShader"),
	    .CreateProgram = LoadOpenGlFunction<FCreateProgram>("glCreateProgram"),
	    .AttachShader = LoadOpenGlFunction<FAttachShader>("glAttachShader"),
	    .LinkProgram = LoadOpenGlFunction<FLinkProgram>("glLinkProgram"),
	    .GetProgramiv = LoadOpenGlFunction<FGetProgramiv>("glGetProgramiv"),
	    .GetProgramInfoLog = LoadOpenGlFunction<FGetProgramInfoLog>("glGetProgramInfoLog"),
	    .DeleteProgram = LoadOpenGlFunction<FDeleteProgram>("glDeleteProgram"),
	    .UseProgram = LoadOpenGlFunction<FUseProgram>("glUseProgram"),
	    .GetUniformLocation = LoadOpenGlFunction<FGetUniformLocation>("glGetUniformLocation"),
	    .Uniform1f = LoadOpenGlFunction<FUniform1f>("glUniform1f"),
	    .Uniform1iv = LoadOpenGlFunction<FUniform1iv>("glUniform1iv"),
	    .Uniform3f = LoadOpenGlFunction<FUniform3f>("glUniform3f"),
	    .Uniform2f = LoadOpenGlFunction<FUniform2f>("glUniform2f"),
	    .ActiveTexture = LoadOpenGlFunction<FActiveTexture>("glActiveTexture"),
	    .GenFramebuffers = LoadOpenGlFunction<FGenFramebuffers>("glGenFramebuffers"),
	    .DeleteFramebuffers = LoadOpenGlFunction<FDeleteFramebuffers>("glDeleteFramebuffers"),
	    .BindFramebuffer = LoadOpenGlFunction<FBindFramebuffer>("glBindFramebuffer"),
	    .FramebufferTexture2D = LoadOpenGlFunction<FFramebufferTexture2D>("glFramebufferTexture2D"),
	    .CheckFramebufferStatus = LoadOpenGlFunction<FCheckFramebufferStatus>("glCheckFramebufferStatus"),
	    .GenVertexArrays = LoadOpenGlFunction<FGenVertexArrays>("glGenVertexArrays"),
	    .DeleteVertexArrays = LoadOpenGlFunction<FDeleteVertexArrays>("glDeleteVertexArrays"),
	    .BindVertexArray = LoadOpenGlFunction<FBindVertexArray>("glBindVertexArray")};

	const bool bComplete = Api.CreateShader != nullptr && Api.ShaderSource != nullptr && Api.CompileShader != nullptr &&
	                       Api.GetShaderiv != nullptr && Api.GetShaderInfoLog != nullptr && Api.DeleteShader != nullptr &&
	                       Api.CreateProgram != nullptr && Api.AttachShader != nullptr && Api.LinkProgram != nullptr &&
	                       Api.GetProgramiv != nullptr && Api.GetProgramInfoLog != nullptr && Api.DeleteProgram != nullptr &&
	                       Api.UseProgram != nullptr && Api.GetUniformLocation != nullptr && Api.Uniform1f != nullptr &&
	                       Api.Uniform1iv != nullptr && Api.Uniform3f != nullptr && Api.GenVertexArrays != nullptr &&
	                       Api.DeleteVertexArrays != nullptr && Api.BindVertexArray != nullptr && Api.Uniform2f != nullptr &&
	                       Api.ActiveTexture != nullptr && Api.GenFramebuffers != nullptr && Api.DeleteFramebuffers != nullptr &&
	                       Api.BindFramebuffer != nullptr && Api.FramebufferTexture2D != nullptr && Api.CheckFramebufferStatus != nullptr;
	if (!bComplete)
	{
		return std::unexpected("OpenGL 3.3 background renderer entry points are unavailable");
	}

	return Api;
}

[[nodiscard]] std::string ReadShaderLog(const FOpenGlApi& Api, const GLuint Shader)
{
	GLint Length = 0;
	Api.GetShaderiv(Shader, InfoLogLength, &Length);
	std::vector<char> Buffer(static_cast<std::size_t>(std::max(Length, 1)));
	Api.GetShaderInfoLog(Shader, static_cast<GLsizei>(Buffer.size()), nullptr, Buffer.data());
	return Buffer.data();
}

[[nodiscard]] std::string ReadProgramLog(const FOpenGlApi& Api, const GLuint Program)
{
	GLint Length = 0;
	Api.GetProgramiv(Program, InfoLogLength, &Length);
	std::vector<char> Buffer(static_cast<std::size_t>(std::max(Length, 1)));
	Api.GetProgramInfoLog(Program, static_cast<GLsizei>(Buffer.size()), nullptr, Buffer.data());
	return Buffer.data();
}

[[nodiscard]] std::expected<GLuint, std::string> CompileShader(const FOpenGlApi& Api, const GLenum Type, const char* const Source)
{
	const GLuint Shader = Api.CreateShader(Type);
	if (Shader == 0)
	{
		return std::unexpected("OpenGL shader allocation failed");
	}
	Api.ShaderSource(Shader, 1, &Source, nullptr);
	Api.CompileShader(Shader);
	GLint bCompiled = GL_FALSE;
	Api.GetShaderiv(Shader, CompileStatus, &bCompiled);
	if (bCompiled == GL_FALSE)
	{
		const std::string Error = ReadShaderLog(Api, Shader);
		Api.DeleteShader(Shader);
		return std::unexpected(Error);
	}

	return Shader;
}

constexpr const char* VertexShaderSource = R"glsl(#version 330 core
out vec2 ScreenUv;

void main()
{
    vec2 Position = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    ScreenUv = Position;
    gl_Position = vec4(Position * 2.0 - 1.0, 0.0, 1.0);
}
)glsl";

constexpr const char* FragmentShaderSource = R"glsl(#version 330 core
in vec2 ScreenUv;
out vec4 OutputColor;

uniform vec3 AccentColor;
uniform float GradientHeight;
uniform float Intensity;
uniform float TrailingIntensityRatio;
uniform float CheckerboardSize;
uniform int DitherPattern[64];

void main()
{
    const vec3 CanvasColor = vec3(25.0 / 255.0);
    float VerticalFade = clamp((ScreenUv.y - (1.0 - GradientHeight)) / GradientHeight, 0.0, 1.0);
    float HorizontalIntensity = mix(Intensity, Intensity * TrailingIntensityRatio, ScreenUv.x);
    vec3 BaseColor = CanvasColor;
    if (CheckerboardSize > 0.0)
    {
        ivec2 Cell = ivec2(floor(gl_FragCoord.xy / CheckerboardSize));
        BaseColor = vec3(((Cell.x + Cell.y) & 1) == 0 ? 48.0 / 255.0 : 208.0 / 255.0);
    }
    vec3 Color = mix(BaseColor, AccentColor, VerticalFade * HorizontalIntensity);

    ivec2 DitherPosition = ivec2(gl_FragCoord.xy) & 7;
    int DitherIndex = DitherPosition.y * 8 + DitherPosition.x;
    float Threshold = (float(DitherPattern[DitherIndex]) + 0.5) / 64.0;
    vec3 ScaledColor = clamp(Color, 0.0, 1.0) * 255.0;
    vec3 QuantizedColor = (floor(ScaledColor) + step(vec3(Threshold), fract(ScaledColor))) / 255.0;
    OutputColor = vec4(QuantizedColor, 1.0);
}
)glsl";

constexpr const char* BlurFragmentShaderSource = R"glsl(#version 330 core
in vec2 ScreenUv;
out vec4 OutputColor;

uniform sampler2D SourceTexture;
uniform vec2 FilterParameters;

void main()
{
    vec2 Offset = FilterParameters.x / vec2(textureSize(SourceTexture, 0));
    if (FilterParameters.y > 0.5)
    {
        vec2 HalfOffset = Offset * 0.5;
        vec4 Color = texture(SourceTexture, ScreenUv + vec2(-Offset.x, 0.0));
        Color += texture(SourceTexture, ScreenUv + vec2(Offset.x, 0.0));
        Color += texture(SourceTexture, ScreenUv + vec2(0.0, -Offset.y));
        Color += texture(SourceTexture, ScreenUv + vec2(0.0, Offset.y));
        Color += texture(SourceTexture, ScreenUv - HalfOffset) * 2.0;
        Color += texture(SourceTexture, ScreenUv + HalfOffset) * 2.0;
        Color += texture(SourceTexture, ScreenUv + vec2(HalfOffset.x, -HalfOffset.y)) * 2.0;
        Color += texture(SourceTexture, ScreenUv + vec2(-HalfOffset.x, HalfOffset.y)) * 2.0;
        OutputColor = Color / 12.0;
        return;
    }
    vec4 Color = texture(SourceTexture, ScreenUv) * 4.0;
    Color += texture(SourceTexture, ScreenUv - Offset);
    Color += texture(SourceTexture, ScreenUv + Offset);
    Color += texture(SourceTexture, ScreenUv + vec2(Offset.x, -Offset.y));
    Color += texture(SourceTexture, ScreenUv + vec2(-Offset.x, Offset.y));
    OutputColor = Color / 8.0;
}
)glsl";
}

struct FBackgroundRenderer::FImplementation
{
	FOpenGlApi Api;
	GLuint Program = 0;
	GLuint VertexArray = 0;
	GLuint BlurProgram = 0;
	std::array<GLuint, KawaseLevelCount + 1> BlurTextures = {};
	std::array<GLuint, KawaseLevelCount> BlurFramebuffers = {};
	int BlurWidth = 0;
	int BlurHeight = 0;
	GLint FilterParametersLocation = -1;
	GLint AccentColorLocation = -1;
	GLint GradientHeightLocation = -1;
	GLint IntensityLocation = -1;
	GLint TrailingIntensityRatioLocation = -1;
	GLint CheckerboardSizeLocation = -1;
};

FBackgroundRenderer::FBackgroundRenderer()
    : Implementation(std::make_unique<FImplementation>())
{
}

FBackgroundRenderer::~FBackgroundRenderer() = default;

std::expected<void, std::string> FBackgroundRenderer::Initialize()
{
	const std::expected<FOpenGlApi, std::string> Api = LoadOpenGlApi();
	if (!Api)
	{
		return std::unexpected(Api.error());
	}
	Implementation->Api = *Api;

	const std::expected<GLuint, std::string> VertexShader = CompileShader(Implementation->Api, VertexShaderType, VertexShaderSource);
	if (!VertexShader)
	{
		return std::unexpected(std::format("Background vertex shader compilation failed: {}", VertexShader.error()));
	}

	const std::expected<GLuint, std::string> FragmentShader = CompileShader(Implementation->Api, FragmentShaderType, FragmentShaderSource);
	if (!FragmentShader)
	{
		Implementation->Api.DeleteShader(*VertexShader);
		return std::unexpected(std::format("Background fragment shader compilation failed: {}", FragmentShader.error()));
	}

	Implementation->Program = Implementation->Api.CreateProgram();
	Implementation->Api.AttachShader(Implementation->Program, *VertexShader);
	Implementation->Api.AttachShader(Implementation->Program, *FragmentShader);
	Implementation->Api.LinkProgram(Implementation->Program);
	Implementation->Api.DeleteShader(*FragmentShader);

	GLint bLinked = GL_FALSE;
	Implementation->Api.GetProgramiv(Implementation->Program, LinkStatus, &bLinked);
	if (bLinked == GL_FALSE)
	{
		const std::string Error = ReadProgramLog(Implementation->Api, Implementation->Program);
		Implementation->Api.DeleteShader(*VertexShader);
		Shutdown();
		return std::unexpected(std::format("Background shader link failed: {}", Error));
	}

	const std::expected<GLuint, std::string> BlurShader = CompileShader(Implementation->Api, FragmentShaderType, BlurFragmentShaderSource);
	if (!BlurShader)
	{
		Implementation->Api.DeleteShader(*VertexShader);
		Shutdown();
		return std::unexpected(std::format("Blur fragment shader compilation failed: {}", BlurShader.error()));
	}
	Implementation->BlurProgram = Implementation->Api.CreateProgram();
	Implementation->Api.AttachShader(Implementation->BlurProgram, *VertexShader);
	Implementation->Api.AttachShader(Implementation->BlurProgram, *BlurShader);
	Implementation->Api.LinkProgram(Implementation->BlurProgram);
	Implementation->Api.DeleteShader(*VertexShader);
	Implementation->Api.DeleteShader(*BlurShader);
	Implementation->Api.GetProgramiv(Implementation->BlurProgram, LinkStatus, &bLinked);
	if (bLinked == GL_FALSE)
	{
		const std::string Error = ReadProgramLog(Implementation->Api, Implementation->BlurProgram);
		Shutdown();
		return std::unexpected(std::format("Blur shader link failed: {}", Error));
	}
	Implementation->FilterParametersLocation = Implementation->Api.GetUniformLocation(Implementation->BlurProgram, "FilterParameters");
	const GLint SourceTextureLocation = Implementation->Api.GetUniformLocation(Implementation->BlurProgram, "SourceTexture");
	if (Implementation->FilterParametersLocation < 0 || SourceTextureLocation < 0)
	{
		Shutdown();
		return std::unexpected("Blur shader uniforms are unavailable");
	}
	const GLint SourceTextureUnit = 0;
	Implementation->Api.UseProgram(Implementation->BlurProgram);
	Implementation->Api.Uniform1iv(SourceTextureLocation, 1, &SourceTextureUnit);
	Implementation->Api.UseProgram(0);

	Implementation->AccentColorLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "AccentColor");
	Implementation->GradientHeightLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "GradientHeight");
	Implementation->IntensityLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "Intensity");
	Implementation->TrailingIntensityRatioLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "TrailingIntensityRatio");
	Implementation->CheckerboardSizeLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "CheckerboardSize");
	const GLint DitherPatternLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "DitherPattern[0]");
	if (Implementation->AccentColorLocation < 0 || Implementation->GradientHeightLocation < 0 || Implementation->IntensityLocation < 0 ||
	    Implementation->TrailingIntensityRatioLocation < 0 || Implementation->CheckerboardSizeLocation < 0 || DitherPatternLocation < 0)
	{
		Shutdown();
		return std::unexpected("Background shader uniforms are unavailable");
	}

	std::array<GLint, BackgroundDitherPattern.size()> DitherPattern = {};
	for (std::size_t Index = 0; Index < DitherPattern.size(); ++Index)
	{
		DitherPattern[Index] = BackgroundDitherPattern[Index];
	}
	Implementation->Api.UseProgram(Implementation->Program);
	Implementation->Api.Uniform1iv(DitherPatternLocation, static_cast<GLsizei>(DitherPattern.size()), DitherPattern.data());
	Implementation->Api.UseProgram(0);
	Implementation->Api.GenVertexArrays(1, &Implementation->VertexArray);
	glGenTextures(static_cast<GLsizei>(Implementation->BlurTextures.size()), Implementation->BlurTextures.data());
	Implementation->Api.GenFramebuffers(static_cast<GLsizei>(Implementation->BlurFramebuffers.size()), Implementation->BlurFramebuffers.data());
	if (Implementation->VertexArray == 0 || std::ranges::find(Implementation->BlurTextures, 0) != Implementation->BlurTextures.end() ||
	    std::ranges::find(Implementation->BlurFramebuffers, 0) != Implementation->BlurFramebuffers.end())
	{
		Shutdown();
		return std::unexpected("Background renderer resource allocation failed");
	}
	return {};
}

void FBackgroundRenderer::Shutdown()
{
	if (Implementation->Api.DeleteFramebuffers != nullptr)
	{
		Implementation->Api.DeleteFramebuffers(static_cast<GLsizei>(Implementation->BlurFramebuffers.size()), Implementation->BlurFramebuffers.data());
		Implementation->BlurFramebuffers.fill(0);
	}
	glDeleteTextures(static_cast<GLsizei>(Implementation->BlurTextures.size()), Implementation->BlurTextures.data());
	Implementation->BlurTextures.fill(0);
	Implementation->BlurWidth = 0;
	Implementation->BlurHeight = 0;
	if (Implementation->BlurProgram != 0 && Implementation->Api.DeleteProgram != nullptr)
	{
		Implementation->Api.DeleteProgram(Implementation->BlurProgram);
		Implementation->BlurProgram = 0;
	}
	if (Implementation->VertexArray != 0 && Implementation->Api.DeleteVertexArrays != nullptr)
	{
		Implementation->Api.DeleteVertexArrays(1, &Implementation->VertexArray);
		Implementation->VertexArray = 0;
	}

	if (Implementation->Program != 0 && Implementation->Api.DeleteProgram != nullptr)
	{
		Implementation->Api.DeleteProgram(Implementation->Program);
		Implementation->Program = 0;
	}
}

void FBackgroundRenderer::Render(const int Width, const int Height, const FBackgroundGradient& Gradient) const
{
	if (Width <= 0 || Height <= 0 || Implementation->Program == 0)
	{
		return;
	}

	glViewport(0, 0, Width, Height);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	Implementation->Api.UseProgram(Implementation->Program);
	Implementation->Api.Uniform3f(Implementation->AccentColorLocation, Gradient.AccentRed, Gradient.AccentGreen, Gradient.AccentBlue);
	Implementation->Api.Uniform1f(Implementation->GradientHeightLocation, Gradient.HeightRatio);
	Implementation->Api.Uniform1f(Implementation->IntensityLocation, Gradient.Intensity);
	Implementation->Api.Uniform1f(Implementation->TrailingIntensityRatioLocation, Gradient.TrailingIntensityRatio);
	Implementation->Api.Uniform1f(Implementation->CheckerboardSizeLocation, Gradient.CheckerboardSize);
	Implementation->Api.BindVertexArray(Implementation->VertexArray);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	Implementation->Api.BindVertexArray(0);
	Implementation->Api.UseProgram(0);
}

std::expected<void, std::string> FBackgroundRenderer::PrepareBlur(const int Width, const int Height, const float PixelRadius)
{
	if (!std::isfinite(PixelRadius) || PixelRadius < 0.0f)
	{
		return std::unexpected("Blur radius must be finite and nonnegative");
	}
	if (PixelRadius == 0.0f || Width <= 0 || Height <= 0)
	{
		return {};
	}
	if (Implementation->BlurProgram == 0)
	{
		return std::unexpected("Background blur is not initialized");
	}

	GLint MaximumTextureSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaximumTextureSize);
	if (Width > MaximumTextureSize || Height > MaximumTextureSize)
	{
		return std::unexpected("Background blur dimensions exceed the maximum texture size");
	}
	GLint PreviousActiveTexture = 0;
	GLint PreviousTexture = 0;
	glGetIntegerv(ActiveTexture, &PreviousActiveTexture);
	Implementation->Api.ActiveTexture(Texture0);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);
	const auto RestoreState = [&]
	{
		Implementation->Api.BindFramebuffer(Framebuffer, 0);
		glViewport(0, 0, Width, Height);
		Implementation->Api.BindVertexArray(0);
		Implementation->Api.UseProgram(0);
		glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(PreviousTexture));
		Implementation->Api.ActiveTexture(static_cast<GLenum>(PreviousActiveTexture));
	};

	std::array<std::array<int, 2>, KawaseLevelCount + 1> TextureSizes = {};
	TextureSizes[0] = {Width, Height};
	for (std::size_t Level = 1; Level < TextureSizes.size(); ++Level)
	{
		TextureSizes[Level] = {(TextureSizes[Level - 1][0] + 1) / 2, (TextureSizes[Level - 1][1] + 1) / 2};
	}
	if (Implementation->BlurWidth != Width || Implementation->BlurHeight != Height)
	{
		for (std::size_t Index = 0; Index < Implementation->BlurTextures.size(); ++Index)
		{
			const int TextureWidth = TextureSizes[Index][0];
			const int TextureHeight = TextureSizes[Index][1];
			glBindTexture(GL_TEXTURE_2D, Implementation->BlurTextures[Index]);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, ClampToEdge);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, ClampToEdge);
			glTexImage2D(GL_TEXTURE_2D, 0, Rgba8, TextureWidth, TextureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
			GLint AllocatedWidth = 0;
			GLint AllocatedHeight = 0;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &AllocatedWidth);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &AllocatedHeight);
			if (AllocatedWidth != TextureWidth || AllocatedHeight != TextureHeight)
			{
				RestoreState();
				return std::unexpected("Background blur texture storage allocation failed");
			}
			if (Index > 0)
			{
				Implementation->Api.BindFramebuffer(Framebuffer, Implementation->BlurFramebuffers[Index - 1]);
				Implementation->Api.FramebufferTexture2D(Framebuffer, ColorAttachment, GL_TEXTURE_2D, Implementation->BlurTextures[Index], 0);
				const GLenum Status = Implementation->Api.CheckFramebufferStatus(Framebuffer);
				if (Status != FramebufferComplete)
				{
					RestoreState();
					return std::unexpected(std::format("Background blur framebuffer is incomplete: 0x{:X}", Status));
				}
			}
		}
		Implementation->BlurWidth = Width;
		Implementation->BlurHeight = Height;
	}

	Implementation->Api.BindFramebuffer(Framebuffer, 0);
	glBindTexture(GL_TEXTURE_2D, Implementation->BlurTextures[0]);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, Width, Height);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	Implementation->Api.UseProgram(Implementation->BlurProgram);
	Implementation->Api.BindVertexArray(Implementation->VertexArray);
	// Match Herta's radius calibration while keeping Kawase offsets within two source texels.
	const float Deviation = 0.44f * std::min(PixelRadius, 160.0f);
	std::size_t Levels = 1;
	while (Levels + 1 < KawaseLevelCount && Deviation > 3.18f * static_cast<float>(1u << Levels))
	{
		++Levels;
	}
	const float Offset = std::clamp((Deviation / static_cast<float>(1u << Levels) - 0.76f) / 1.21f, 0.0f, 2.0f);
	for (std::size_t Pass = 0; Pass <= 2 * Levels; ++Pass)
	{
		const bool bUpsample = Pass > Levels;
		const std::size_t Target = bUpsample ? 2 * Levels - Pass : Pass;
		const std::size_t Source = bUpsample ? Target + 2 : Target;
		Implementation->Api.BindFramebuffer(Framebuffer, Implementation->BlurFramebuffers[Target]);
		glViewport(0, 0, TextureSizes[Target + 1][0], TextureSizes[Target + 1][1]);
		glBindTexture(GL_TEXTURE_2D, Implementation->BlurTextures[Source]);
		Implementation->Api.Uniform2f(Implementation->FilterParametersLocation, Offset, bUpsample ? 1.0f : 0.0f);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
	RestoreState();
	return {};
}

unsigned int FBackgroundRenderer::GetBlurTexture() const
{
	return Implementation->BlurTextures[1];
}

std::expected<void, std::string> FBackgroundRenderer::ValidateBlur()
{
	if (Implementation->BlurProgram == 0)
	{
		return std::unexpected("Background blur is not initialized");
	}
	std::array<GLint, 4> PreviousViewport = {};
	std::array<GLfloat, 4> PreviousClearColor = {};
	GLint PreviousPackAlignment = 0;
	glGetIntegerv(GL_VIEWPORT, PreviousViewport.data());
	glGetFloatv(GL_COLOR_CLEAR_VALUE, PreviousClearColor.data());
	glGetIntegerv(GL_PACK_ALIGNMENT, &PreviousPackAlignment);
	const auto RestoreState = [&]
	{
		Implementation->Api.BindFramebuffer(Framebuffer, 0);
		glViewport(PreviousViewport[0], PreviousViewport[1], PreviousViewport[2], PreviousViewport[3]);
		glClearColor(PreviousClearColor[0], PreviousClearColor[1], PreviousClearColor[2], PreviousClearColor[3]);
		glPixelStorei(GL_PACK_ALIGNMENT, PreviousPackAlignment);
		glDisable(GL_SCISSOR_TEST);
	};
	const auto ReadBlur = [&](const int Width, const int Height)
	{
		std::vector<unsigned char> Pixels(static_cast<std::size_t>(Width) * Height * 4);
		Implementation->Api.BindFramebuffer(Framebuffer, Implementation->BlurFramebuffers[0]);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, Pixels.data());
		Implementation->Api.BindFramebuffer(Framebuffer, 0);
		return Pixels;
	};

	FBackgroundGradient Preview = {.HeightRatio = 1.0f, .CheckerboardSize = 32.0f};
	Render(64, 64, Preview);
	std::array<unsigned char, 4> Pixel = {};
	for (int Y = 0; Y < 2; ++Y)
	{
		for (int X = 0; X < 2; ++X)
		{
			glReadPixels(X * 32 + 16, Y * 32 + 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, Pixel.data());
			if (Pixel[0] != ((X + Y) % 2 == 0 ? 48 : 208))
			{
				RestoreState();
				return std::unexpected("Checkerboard GPU check did not alternate background colors");
			}
		}
	}
	if (const auto Prepared = PrepareBlur(64, 64, 12.0f); !Prepared)
	{
		RestoreState();
		return Prepared;
	}
	const std::vector<unsigned char> CheckerboardPixels = ReadBlur(32, 32);
	const unsigned char BlurredEdge = CheckerboardPixels[(8 * 32 + 15) * 4];
	if (BlurredEdge <= 48 || BlurredEdge >= 208)
	{
		RestoreState();
		return std::unexpected("Checkerboard GPU check did not blur a cell edge");
	}
	Preview.CheckerboardSize = 0.0f;
	Render(64, 64, Preview);
	glReadPixels(48, 16, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, Pixel.data());
	if (Pixel[0] != 25)
	{
		RestoreState();
		return std::unexpected("Checkerboard GPU check did not restore the plain background");
	}

	constexpr std::array<std::array<int, 2>, 5> Dimensions = {{{64, 64}, {65, 47}, {1, 1}, {1, 65}, {65, 1}}};
	constexpr std::array Radii = {0.25f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 160.0f};
	for (const auto& Size : Dimensions)
	{
		const int Width = Size[0];
		const int Height = Size[1];
		const int BlurWidth = Width / 2 + Width % 2;
		const int BlurHeight = Height / 2 + Height % 2;
		Implementation->Api.BindFramebuffer(Framebuffer, 0);
		glDisable(GL_SCISSOR_TEST);
		glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		for (const float Radius : Radii)
		{
			if (const auto Prepared = PrepareBlur(Width, Height, Radius); !Prepared)
			{
				RestoreState();
				return Prepared;
			}
			const std::vector<unsigned char> UniformPixels = ReadBlur(BlurWidth, BlurHeight);
			constexpr std::array<int, 4> ExpectedColor = {64, 128, 191, 255};
			for (std::size_t Index = 0; Index < UniformPixels.size(); ++Index)
			{
				if (std::abs(static_cast<int>(UniformPixels[Index]) - ExpectedColor[Index % 4]) > 1)
				{
					RestoreState();
					return std::unexpected("Kawase blur GPU check did not preserve a uniform color through the pyramid");
				}
			}
		}
		if (Width < 4 || Height < 4)
		{
			continue;
		}

		for (int Axis = 0; Axis < 2; ++Axis)
		{
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glEnable(GL_SCISSOR_TEST);
			glScissor(Axis == 0 ? Width / 2 : 0, Axis == 1 ? Height / 2 : 0, Axis == 0 ? Width - Width / 2 : Width, Axis == 1 ? Height - Height / 2 : Height);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_SCISSOR_TEST);
			if (const auto Prepared = PrepareBlur(Width, Height, 12.0f); !Prepared)
			{
				RestoreState();
				return Prepared;
			}
			const std::vector<unsigned char> EdgePixels = ReadBlur(BlurWidth, BlurHeight);
			const auto Sample = [&](const int Position)
			{
				const int X = Axis == 0 ? Position : BlurWidth / 2;
				const int Y = Axis == 1 ? Position : BlurHeight / 2;
				return EdgePixels[(static_cast<std::size_t>(Y) * BlurWidth + X) * 4];
			};
			const int Extent = Axis == 0 ? BlurWidth : BlurHeight;
			if (Sample(0) > 1 || Sample(Extent - 1) < 254 || Sample(Extent / 2 - 1) <= 1 || Sample(Extent / 2) >= 254 || Sample(Extent / 2 - 1) >= Sample(Extent / 2))
			{
				RestoreState();
				return std::unexpected("Background blur GPU check did not soften an edge while preserving distant colors");
			}
			if (const auto Prepared = PrepareBlur(Width, Height, 0.0f); !Prepared)
			{
				RestoreState();
				return Prepared;
			}
			if (ReadBlur(BlurWidth, BlurHeight) != EdgePixels)
			{
				RestoreState();
				return std::unexpected("Background blur GPU check changed the texture at zero radius");
			}
		}
	}
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);
	glScissor(64, 0, 64, 64);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_SCISSOR_TEST);
	if (const auto Prepared = PrepareBlur(128, 64, 40.0f); !Prepared)
	{
		RestoreState();
		return Prepared;
	}
	const std::vector<unsigned char> SmoothPixels = ReadBlur(64, 32);
	for (int X = 1; X < 63; ++X)
	{
		const std::size_t Index = (16 * 64 + static_cast<std::size_t>(X)) * 4;
		const int Previous = SmoothPixels[Index - 4];
		const int Current = SmoothPixels[Index];
		const int Next = SmoothPixels[Index + 4];
		if (Current < Previous || std::abs(Next - 2 * Current + Previous) > 3)
		{
			RestoreState();
			return std::unexpected(std::format("Background blur GPU check found steps at {}: {}, {}, {}", X, Previous, Current, Next));
		}
	}
	RestoreState();
	return {};
}
}
