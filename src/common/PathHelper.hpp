#pragma once
#ifndef WINPOD_UTILS_PATH_HELPER_HPP
#define WINPOD_UTILS_PATH_HELPER_HPP

#include <filesystem>

namespace winpod::utils {
std::filesystem::path executablePath();
std::filesystem::path executableDirectory();
} // namespace winpod::utils

#endif // WINPOD_UTILS_PATH_HELPER_HPP
