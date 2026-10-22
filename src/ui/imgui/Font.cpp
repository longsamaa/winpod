#include "Font.hpp"
#include <filesystem>
namespace winpod
{
ImFont* Font::LoadFont(const char* path, float size)
{
    if (!std::filesystem::exists(path))
        return nullptr;
    ImFontConfig cfg;
    cfg.PixelSnapH = true;
    cfg.RasterizerMultiply = 1.2f;
    return ImGui::GetIO().Fonts->AddFontFromFileTTF(path, size, &cfg);
}
void Font::Merge(const char* path, float size)
{
    if (!std::filesystem::exists(path))
        return;
    ImFontConfig cfg;
    cfg.MergeMode = true;
    cfg.PixelSnapH = true;
    cfg.GlyphMinAdvanceX = size;
    ImGui::GetIO().Fonts->AddFontFromFileTTF(kIconFontPath, size, &cfg);
}

} // namespace winpod
