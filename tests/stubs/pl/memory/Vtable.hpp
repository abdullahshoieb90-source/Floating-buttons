#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace pl::memory {
std::uintptr_t resolveVtableFunction(std::string_view, std::size_t, std::string_view);
}
