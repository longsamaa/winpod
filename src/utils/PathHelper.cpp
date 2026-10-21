#include "PathHelper.hpp"

#include <string>

#include "platform/WindowsSdk.hpp"

namespace winpod::utils {
std::filesystem::path executablePath() {
  std::wstring buffer(MAX_PATH, L'\0');
  for (;;) {
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
      return {};
    }
    // Buffer too small: the path was truncated to exactly buffer.size() characters.
    if (length < buffer.size()) {
      buffer.resize(length);
      return std::filesystem::path(buffer);
    }
    buffer.resize(buffer.size() * 2);
  }
}

std::filesystem::path executableDirectory() {
  return executablePath().parent_path();
}
} // namespace winpod::utils
