#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace ProjectTemplate::Private
{
[[nodiscard]] std::optional<std::span<const std::byte>> FindEmbeddedAsset(std::string_view VirtualPath) noexcept;
}
