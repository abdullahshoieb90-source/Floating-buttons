#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
namespace pl::memory {
std::unordered_map<std::string, std::uintptr_t> resolveSignatures(std::span<const std::string>, std::string_view);
}
