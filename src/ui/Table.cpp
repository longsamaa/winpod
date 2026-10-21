#include "Table.hpp"
#include "log/Log.hpp"
#include <imgui.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <memory>
namespace winpod
{
Table::Table() {}
Table::~Table() {}
void Table::Update(const float& dt)
{
    for (const auto& component : m_Components)
    {
        component->Update(dt);
    }
}
float Table::GetColW(const ImVec2& area, const float& paddingX, const uint8_t& columns) const
{
    assert(columns > 0);
    return (area.x - (columns - 1) * paddingX * 2.0f) / columns;
}

std::vector<std::vector<std::shared_ptr<Component>>> Table::VisibleColumns() const
{
    std::vector<std::vector<std::shared_ptr<Component>>> columns;
    for (const auto& col : m_Columns)
    {
        std::vector<std::shared_ptr<Component>> visible;
        for (const auto& idx : col)
        {
            std::shared_ptr<Component> component = m_Components[idx];
            if (component->IsVisible())
            {
                visible.push_back(component);
            }
        }
        if (!visible.empty())
        {
            columns.push_back(std::move(visible));
        }
    }
    return columns;
}

float Table::MinWidth(float minFlexWidth)
{
    const auto columns = VisibleColumns();
    if (columns.empty())
        return 0.0f;
    float widestColumn = 0.0f;
    for (const auto& col : columns)
    {
        for (const auto& component : col)
        {
            const float fh = component->FixedHeight();
            const float need = fh > 0.0f ? std::ceil(fh * component->AspectRatio()) : minFlexWidth;
            widestColumn = std::max(widestColumn, need);
        }
    }
    const float n = static_cast<float>(columns.size());
    return n * widestColumn + (n - 1.0f) * ImGui::GetStyle().CellPadding.x * 2.0f;
}
ImVec2 Table::GetColumnSize(const int& index) const
{
    return ImVec2(0.0, 0.0);
}
void Table::Draw(const float& dt)
{
    const ImVec2 area = ImGui::GetContentRegionAvail();
    if (area.x <= 0 || area.y <= 0)
    {
        return;
    }
    const auto columns = VisibleColumns();
    if (columns.empty())
        return;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float spacingY = style.ItemSpacing.y;
    const float colW = GetColW(area, style.CellPadding.x, static_cast<uint8_t>(columns.size()));
    auto fit = [](float aspect, float maxW, float maxH) {
        float h = maxH;
        float w = h * aspect;
        if (w > maxW)
        {
            w = maxW;
            h = w / aspect;
        }
        return ImVec2{std::floor(std::max(0.0f, w)), std::floor(std::max(0.0f, h))};
    };
    std::vector<std::vector<ImVec2>> sizes(columns.size());
    size_t rows = 0;
    for (size_t c = 0; c < columns.size(); ++c)
    {
        const auto& col = columns[c];
        float fixedH = 0.0f;
        int flexCount = 0;
        for (const auto& component : col)
        {
            const float fh = component->FixedHeight();
            fixedH += fh;
            if (fh == 0.0f)
            {
                flexCount += 1;
            }
        }
        const float spacing = spacingY * static_cast<float>(col.size() - 1);
        const float flexH = flexCount > 0 ? std::max(0.0f, (area.y - fixedH - spacing) / flexCount) : 0.0f;
        for (const auto& component : col)
        {
            const float fh = component->FixedHeight();
            sizes[c].push_back(fit(component->AspectRatio(), colW, fh > 0.0f ? fh : flexH));
        }
        rows = std::max(rows, col.size());
    }

    std::vector<float> rowH(rows, 0.0f);
    for (const auto& colSizes : sizes)
    {
        for (size_t r = 0; r < colSizes.size(); ++r)
        {
            rowH[r] = std::max(rowH[r], colSizes[r].y);
        }
    }
    float blockH = spacingY * static_cast<float>(rows - 1);
    for (float h : rowH)
    {
        blockH += h;
    }
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::floor(std::max(0.0f, (area.y - blockH) * 0.5f)));

    if (!ImGui::BeginTable("##devices", static_cast<int>(columns.size()),
                           ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_NoPadOuterX))
    {
        return;
    }
    for (size_t c = 0; c < columns.size(); ++c)
    {
        ImGui::TableNextColumn();
        const float cellW = ImGui::GetContentRegionAvail().x;
        const ImVec2 cellStart = ImGui::GetCursorPos();
        float y = cellStart.y;
        for (size_t r = 0; r < columns[c].size(); ++r)
        {
            ImVec2 size = sizes[c][r];
            size.x = std::min(size.x, cellW);
            const bool fixed = columns[c][r]->FixedHeight() > 0.0f;
            const float offsetY = fixed ? 0.0f : (rowH[r] - size.y) * 0.5f;
            ImGui::SetCursorPos({std::floor(cellStart.x + (cellW - size.x) * 0.5f), std::floor(y + offsetY)});
            columns[c][r]->Draw(size);
            y += rowH[r] + spacingY;
        }
    }
    ImGui::EndTable();
}
void Table::AddComponent(const int& col, const std::shared_ptr<Component>& component)
{
    if (!component)
        return;
    const std::string name = component->GetName();
    auto it = m_Index.find(name);
    if (it != m_Index.end())
    {
        Log::instance().getAppLogger()->error("[Table] Component \"{}\" already exists", name);
        assert(false && "Table::AddComponent: component name must be unique");
        return;
    }
    if (col < 0)
    {
        Log::instance().getAppLogger()->error("[Table] Invalid column {} for component \"{}\"", col,
                                              component->GetName());
        return;
    }
    const auto column = static_cast<size_t>(col);
    if (column >= m_Columns.size())
    {
        m_Columns.resize(column + 1);
    }
    int index = m_Components.size();
    m_Components.push_back(component);
    m_Index.insert({name, index});
    m_Columns[column].push_back(index);
}
std::shared_ptr<Component> Table::GetComponent(const std::string& name)
{
    auto it = m_Index.find(name);
    if (it == m_Index.end())
        return nullptr;
    return m_Components[it->second];
}
void Table::Clear()
{
    for (const auto& component : m_Components)
    {
        component->Unload();
    }
    m_Index.clear();
    m_Components.clear();
    m_Columns.clear();
}
} // namespace winpod
