#ifndef _WINPOD_TABLE_HPP_
#define _WINPOD_TABLE_HPP_
#include "imgui.h"
#include "core/AirpodParser.hpp"
#include "ui/component/Component.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
namespace winpod
{
class Table
{
public:
    Table();
    ~Table();

public:
    void Draw(const float& dt);
    void Update(const float& dt);
    void AddComponent(const int& col, const std::shared_ptr<Component>& component);
    std::shared_ptr<Component> GetComponent(const std::string& name);
    void Clear();
    float MinWidth(float minFlexWidth);

private:
    float GetColW(const ImVec2& area, const float& paddingX, const uint8_t& columns) const;
    std::vector<std::vector<std::shared_ptr<Component>>> VisibleColumns() const;
    ImVec2 GetColumnSize(const int& index) const;

private:
    std::unordered_map<std::string, int> m_Index;
    std::vector<std::shared_ptr<Component>> m_Components;
    std::vector<std::vector<int>> m_Columns{};
};
} // namespace winpod
#endif //_WINPOD_TABLE_HPP_
