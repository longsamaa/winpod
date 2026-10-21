#pragma once
#ifndef _WINPOD_UI_COLORS_HPP_
#define _WINPOD_UI_COLORS_HPP_
#include <imgui.h>
namespace winpod::colors
{
inline constexpr ImU32 kCardBackground = IM_COL32(32, 32, 32, 255);

inline constexpr ImU32 kCloseButton = IM_COL32(70, 70, 70, 255);
inline constexpr ImU32 kCloseButtonHovered = IM_COL32(196, 43, 28, 255);
inline constexpr ImU32 kCloseButtonActive = IM_COL32(232, 17, 35, 255);
inline constexpr ImU32 kCloseIcon = IM_COL32(255, 255, 255, 200);
inline constexpr ImU32 kCloseIconHovered = IM_COL32(255, 255, 255, 255);
inline constexpr ImU32 kBatteryTrack = IM_COL32(70, 70, 70, 255);
inline constexpr ImU32 kBatteryLow = IM_COL32(255, 69, 58, 255);
inline constexpr ImU32 kBatteryNormal = IM_COL32(48, 230, 100, 255);
inline constexpr ImU32 kBatteryText = IM_COL32(255, 255, 255, 255);
inline constexpr ImU32 kBatteryLabel = IM_COL32(170, 170, 170, 255);

inline constexpr ImU32 kComboBg = IM_COL32(12, 12, 12, 255);
inline constexpr ImU32 kComboBgHovered = IM_COL32(40, 40, 40, 255);
inline constexpr ImU32 kComboPopupBg = IM_COL32(12, 12, 12, 255);
inline constexpr ImU32 kComboItemSelected = IM_COL32(50, 50, 50, 255);
inline constexpr ImU32 kComboItemHovered = IM_COL32(60, 60, 60, 255);
inline constexpr ImU32 kComboBorder = IM_COL32(70, 70, 70, 255);
} // namespace winpod::colors
#endif // _WINPOD_UI_COLORS_HPP_
