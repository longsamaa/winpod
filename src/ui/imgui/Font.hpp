#pragma once
#include "imgui.h"
#ifndef _FONT_HPP_
#define _FONT_HPP_
#include "imgui.h"
namespace winpod
{
constexpr const char* kRegularFontPath = "resources/fonts/Inter-Regular.ttf";
constexpr const char* kSemiBoldFontPath = "resources/fonts/Inter-SemiBold.ttf";
constexpr const char* kIconFontPath = "resources/fonts/fa-solid-900.otf";
class Font
{

public:
    static ImFont* LoadFont(const char* path, float size);
    static void Merge(const char* path, float size);

private:
    Font();
};
} // namespace winpod
#endif //_FONT_HPP_
