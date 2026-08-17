#include "Rendering/BackgroundRenderer.h"

#include "Rendering/BackgroundDither.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
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
	    .GenVertexArrays = LoadOpenGlFunction<FGenVertexArrays>("glGenVertexArrays"),
	    .DeleteVertexArrays = LoadOpenGlFunction<FDeleteVertexArrays>("glDeleteVertexArrays"),
	    .BindVertexArray = LoadOpenGlFunction<FBindVertexArray>("glBindVertexArray")};

	const bool bComplete = Api.CreateShader != nullptr && Api.ShaderSource != nullptr && Api.CompileShader != nullptr &&
	                       Api.GetShaderiv != nullptr && Api.GetShaderInfoLog != nullptr && Api.DeleteShader != nullptr &&
	                       Api.CreateProgram != nullptr && Api.AttachShader != nullptr && Api.LinkProgram != nullptr &&
	                       Api.GetProgramiv != nullptr && Api.GetProgramInfoLog != nullptr && Api.DeleteProgram != nullptr &&
	                       Api.UseProgram != nullptr && Api.GetUniformLocation != nullptr && Api.Uniform1f != nullptr &&
	                       Api.Uniform1iv != nullptr && Api.Uniform3f != nullptr && Api.GenVertexArrays != nullptr &&
	                       Api.DeleteVertexArrays != nullptr && Api.BindVertexArray != nullptr;
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
uniform int DitherPattern[64];

void main()
{
    const vec3 CanvasColor = vec3(18.0 / 255.0, 18.0 / 255.0, 19.0 / 255.0);
    float VerticalFade = clamp((ScreenUv.y - (1.0 - GradientHeight)) / GradientHeight, 0.0, 1.0);
    float HorizontalIntensity = mix(Intensity, Intensity * TrailingIntensityRatio, ScreenUv.x);
    vec3 Color = mix(CanvasColor, AccentColor, VerticalFade * HorizontalIntensity);

    ivec2 DitherPosition = ivec2(gl_FragCoord.xy) & 7;
    int DitherIndex = DitherPosition.y * 8 + DitherPosition.x;
    float Threshold = (float(DitherPattern[DitherIndex]) + 0.5) / 64.0;
    vec3 ScaledColor = clamp(Color, 0.0, 1.0) * 255.0;
    vec3 QuantizedColor = (floor(ScaledColor) + step(vec3(Threshold), fract(ScaledColor))) / 255.0;
    OutputColor = vec4(QuantizedColor, 1.0);
}
)glsl";
}

struct FBackgroundRenderer::FImplementation
{
	FOpenGlApi Api;
	GLuint Program = 0;
	GLuint VertexArray = 0;
	GLint AccentColorLocation = -1;
	GLint GradientHeightLocation = -1;
	GLint IntensityLocation = -1;
	GLint TrailingIntensityRatioLocation = -1;
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
	Implementation->Api.DeleteShader(*VertexShader);
	Implementation->Api.DeleteShader(*FragmentShader);

	GLint bLinked = GL_FALSE;
	Implementation->Api.GetProgramiv(Implementation->Program, LinkStatus, &bLinked);
	if (bLinked == GL_FALSE)
	{
		const std::string Error = ReadProgramLog(Implementation->Api, Implementation->Program);
		Shutdown();
		return std::unexpected(std::format("Background shader link failed: {}", Error));
	}

	Implementation->AccentColorLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "AccentColor");
	Implementation->GradientHeightLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "GradientHeight");
	Implementation->IntensityLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "Intensity");
	Implementation->TrailingIntensityRatioLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "TrailingIntensityRatio");
	const GLint DitherPatternLocation = Implementation->Api.GetUniformLocation(Implementation->Program, "DitherPattern[0]");
	if (Implementation->AccentColorLocation < 0 || Implementation->GradientHeightLocation < 0 || Implementation->IntensityLocation < 0 ||
	    Implementation->TrailingIntensityRatioLocation < 0 || DitherPatternLocation < 0)
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
	return {};
}

void FBackgroundRenderer::Shutdown()
{
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
	Implementation->Api.BindVertexArray(Implementation->VertexArray);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	Implementation->Api.BindVertexArray(0);
	Implementation->Api.UseProgram(0);
}
}
