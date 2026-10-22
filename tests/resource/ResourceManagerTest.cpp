#include "bluetooth/AirpodParser.hpp"
#include "fakes/FakeRenderer.hpp"
#include "resource/ResourceManager.hpp"
#include <gtest/gtest.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using winpod::ResourceManager;
using winpod::Texture;
using winpod::test::FakeRenderer;

namespace
{
const fs::path kResources{WINPOD_RESOURCES_DIR};
const fs::path kBuds = kResources / "buds";
const fs::path kCase = kResources / "case";

constexpr int kSheetWidth = 12 * 256;
constexpr int kSheetHeight = 15 * 224;

std::vector<fs::path> FilesIn(const fs::path& folder, const std::string& extension)
{
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(folder))
    {
        if (entry.path().extension() == extension)
        {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

std::set<std::string> DeviceNamesIn(const fs::path& folder)
{
    std::set<std::string> names;
    for (const auto& json : FilesIn(folder, ".json"))
    {
        names.insert(json.stem().string());
    }
    return names;
}

class ResourceManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_TempDir = fs::temp_directory_path() / "winpod_tests" / info->name();
        fs::remove_all(m_TempDir);
        fs::create_directories(m_TempDir);

        m_Renderer = std::make_shared<FakeRenderer>();
        m_Manager = &ResourceManager::instance();
        m_Manager->Init(m_Renderer);
    }

    void TearDown() override
    {
        m_Manager->Shutdown();
        std::error_code ec;
        fs::remove_all(m_TempDir, ec);
    }

    void ExpectLoadsAll(const fs::path& folder)
    {
        const auto files = FilesIn(folder, ".png");
        ASSERT_FALSE(files.empty()) << "Không có *.png trong " << folder;

        const size_t liveBefore = m_Renderer->live.size();
        for (const auto& png : files)
        {
            SCOPED_TRACE(png.generic_string());
            const auto texture = m_Manager->LoadTexture(png);
            ASSERT_NE(texture, nullptr);
            EXPECT_TRUE(texture->IsValid());
            EXPECT_EQ(texture->width, kSheetWidth);
            EXPECT_EQ(texture->height, kSheetHeight);

            EXPECT_EQ(m_Renderer->lastWidth, static_cast<UINT>(texture->width));
            EXPECT_EQ(m_Renderer->lastHeight, static_cast<UINT>(texture->height));
            EXPECT_EQ(m_Renderer->lastUpdated, texture->id);
            EXPECT_EQ(m_Renderer->lastPitch, static_cast<UINT>(texture->width) * 4);
        }
        EXPECT_EQ(m_Renderer->live.size() - liveBefore, files.size());
    }

    fs::path m_TempDir;
    std::shared_ptr<FakeRenderer> m_Renderer;
    ResourceManager* m_Manager{nullptr};
};
} // namespace


TEST_F(ResourceManagerTest, InstanceIsAlwaysTheSameObject)
{
    EXPECT_EQ(&ResourceManager::instance(), &ResourceManager::instance());
    EXPECT_EQ(&ResourceManager::instance(), m_Manager);
}

TEST_F(ResourceManagerTest, LoadFailsBeforeInit)
{
    m_Manager->Shutdown();
    EXPECT_EQ(m_Manager->LoadTexture(kBuds / "AirPods_1.png"), nullptr);
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(ResourceManagerTest, ShutdownDestroysEveryTextureAndClearsCache)
{
    const fs::path budsPng = kBuds / "AirPods_1.png";
    const fs::path casePng = kCase / "AirPods_1.png";
    const auto buds = m_Manager->LoadTexture(budsPng);
    const auto chargingCase = m_Manager->LoadTexture(casePng);
    ASSERT_NE(buds, nullptr);
    ASSERT_NE(chargingCase, nullptr);

    m_Manager->Shutdown();

    EXPECT_TRUE(m_Renderer->live.empty());
    EXPECT_EQ(m_Renderer->destroyCount, 2);
    EXPECT_FALSE(buds->IsValid());
    EXPECT_FALSE(chargingCase->IsValid());
    EXPECT_EQ(m_Manager->GetTexture(budsPng), nullptr);
    EXPECT_EQ(m_Manager->GetTexture(casePng), nullptr);
}

TEST_F(ResourceManagerTest, ShutdownTwiceIsSafe)
{
    ASSERT_NE(m_Manager->LoadTexture(kBuds / "AirPods_1.png"), nullptr);
    m_Manager->Shutdown();
    m_Manager->Shutdown();
    EXPECT_EQ(m_Renderer->destroyCount, 1);
}

TEST_F(ResourceManagerTest, InitAgainAfterShutdownUsesNewRenderer)
{
    ASSERT_NE(m_Manager->LoadTexture(kBuds / "AirPods_1.png"), nullptr);
    m_Manager->Shutdown();

    auto other = std::make_shared<FakeRenderer>();
    m_Manager->Init(other);
    ASSERT_NE(m_Manager->LoadTexture(kBuds / "AirPods_1.png"), nullptr);
    EXPECT_EQ(other->createCount, 1);
    EXPECT_EQ(m_Renderer->createCount, 1);

    m_Manager->Shutdown();
    EXPECT_TRUE(other->live.empty());
}


TEST_F(ResourceManagerTest, LoadsEveryBudsImage)
{
    ExpectLoadsAll(kBuds);
}

TEST_F(ResourceManagerTest, LoadsEveryCaseImage)
{
    ExpectLoadsAll(kCase);
}

TEST_F(ResourceManagerTest, EveryCaseHasMatchingBuds)
{
    const auto buds = DeviceNamesIn(kBuds);
    const auto cases = DeviceNamesIn(kCase);
    for (const auto& name : cases)
    {
        EXPECT_TRUE(buds.count(name)) << "case/" << name << ".json không có buds/" << name << ".json";
    }
    EXPECT_TRUE(buds.count("AirPods_Max"));
    EXPECT_FALSE(cases.count("AirPods_Max"));
}

TEST_F(ResourceManagerTest, EveryModelHasSpriteSheet)
{
    using winpod::AirpodParser;
    using winpod::AirpodsModel;
    const AirpodsModel models[] = {
        AirpodsModel::AirPods1,    AirpodsModel::AirPods2,        AirpodsModel::AirPods3,
        AirpodsModel::AirPods4,    AirpodsModel::AirPods4Anc,     AirpodsModel::AirPodsPro,
        AirpodsModel::AirPodsPro2, AirpodsModel::AirPodsPro2UsbC, AirpodsModel::AirPodsMax,
        AirpodsModel::AirPodsMaxUsbC,
    };
    for (const auto model : models)
    {
        const std::string name{AirpodParser::GetSpriteName(model)};
        SCOPED_TRACE(name);
        ASSERT_FALSE(name.empty());
        EXPECT_TRUE(fs::exists(kBuds / (name + ".json")));
        EXPECT_TRUE(fs::exists(kBuds / (name + ".png")));
        const bool isMax = model == AirpodsModel::AirPodsMax || model == AirpodsModel::AirPodsMaxUsbC;
        EXPECT_EQ(fs::exists(kCase / (name + ".json")), !isMax);
        EXPECT_EQ(fs::exists(kCase / (name + ".png")), !isMax);
    }
    EXPECT_EQ(AirpodParser::GetSpriteName(AirpodsModel::Unknown), "");
}


TEST_F(ResourceManagerTest, SecondLoadReturnsCachedTexture)
{
    const fs::path png = kBuds / "AirPods_1.png";
    const auto first = m_Manager->LoadTexture(png);
    const auto second = m_Manager->LoadTexture(png);
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    EXPECT_EQ(m_Renderer->createCount, 1);
}

TEST_F(ResourceManagerTest, EquivalentPathsShareCacheEntry)
{
    const auto a = m_Manager->LoadTexture(kBuds / "AirPods_1.png");
    const auto b = m_Manager->LoadTexture(kBuds / "." / "AirPods_1.png");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a, b);
    EXPECT_EQ(m_Renderer->createCount, 1);
}

TEST_F(ResourceManagerTest, SameNameInBudsAndCaseAreSeparateEntries)
{
    const auto buds = m_Manager->LoadTexture(kBuds / "AirPods_1.png");
    const auto chargingCase = m_Manager->LoadTexture(kCase / "AirPods_1.png");
    ASSERT_NE(buds, nullptr);
    ASSERT_NE(chargingCase, nullptr);
    EXPECT_NE(buds, chargingCase);
    EXPECT_NE(buds->id, chargingCase->id);
    EXPECT_EQ(m_Renderer->createCount, 2);
}

TEST_F(ResourceManagerTest, GetTextureFindsLoadedTextureOnly)
{
    const fs::path png = kCase / "AirPods_Pro.png";
    EXPECT_EQ(m_Manager->GetTexture(png), nullptr);
    EXPECT_EQ(m_Renderer->createCount, 0);

    const auto texture = m_Manager->LoadTexture(png);
    EXPECT_EQ(m_Manager->GetTexture(png), texture);
}


TEST_F(ResourceManagerTest, UnloadByPathDestroysTextureAndRemovesFromCache)
{
    const fs::path png = kBuds / "AirPods_1.png";
    const auto texture = m_Manager->LoadTexture(png);
    ASSERT_NE(texture, nullptr);
    const ImTextureID id = texture->id;

    m_Manager->UnloadTexture(png);

    EXPECT_EQ(m_Renderer->live.count(id), 0u);
    EXPECT_FALSE(texture->IsValid());
    EXPECT_EQ(m_Manager->GetTexture(png), nullptr);

    const auto reloaded = m_Manager->LoadTexture(png);
    ASSERT_NE(reloaded, nullptr);
    EXPECT_NE(reloaded, texture);
    EXPECT_EQ(m_Renderer->createCount, 2);
}

TEST_F(ResourceManagerTest, UnloadByPointerDestroysTextureAndRemovesFromCache)
{
    const fs::path png = kBuds / "AirPods_1.png";
    const auto texture = m_Manager->LoadTexture(png);
    ASSERT_NE(texture, nullptr);

    m_Manager->UnloadTexture(texture);

    EXPECT_TRUE(m_Renderer->live.empty());
    EXPECT_FALSE(texture->IsValid());
    EXPECT_EQ(m_Manager->GetTexture(png), nullptr);
}

TEST_F(ResourceManagerTest, UnloadKeepsOtherTexturesLoaded)
{
    const auto buds = m_Manager->LoadTexture(kBuds / "AirPods_1.png");
    const auto chargingCase = m_Manager->LoadTexture(kCase / "AirPods_1.png");
    ASSERT_NE(buds, nullptr);
    ASSERT_NE(chargingCase, nullptr);

    m_Manager->UnloadTexture(buds);

    EXPECT_TRUE(chargingCase->IsValid());
    EXPECT_EQ(m_Renderer->live.count(chargingCase->id), 1u);
    EXPECT_EQ(m_Manager->GetTexture(kCase / "AirPods_1.png"), chargingCase);
}

TEST_F(ResourceManagerTest, UnloadUnknownOrNullIsNoop)
{
    m_Manager->UnloadTexture(std::shared_ptr<Texture>{});
    m_Manager->UnloadTexture(kBuds / "chua_load.png");
    EXPECT_EQ(m_Renderer->destroyCount, 0);
}


TEST_F(ResourceManagerTest, FailsWhenImageMissing)
{
    EXPECT_EQ(m_Manager->LoadTexture(kBuds / "khong_ton_tai.png"), nullptr);
    EXPECT_EQ(m_Manager->LoadTexture(kResources / "AirPods_1.png"), nullptr);
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(ResourceManagerTest, FailsWhenFileIsNotAnImage)
{
    const fs::path fake = m_TempDir / "not_an_image.png";
    std::ofstream(fake) << "day khong phai PNG";
    EXPECT_EQ(m_Manager->LoadTexture(fake), nullptr);
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(ResourceManagerTest, FailsWhenTextureCreationFails)
{
    m_Renderer->failCreate = true;
    const fs::path png = kBuds / "AirPods_1.png";
    EXPECT_EQ(m_Manager->LoadTexture(png), nullptr);
    EXPECT_EQ(m_Manager->GetTexture(png), nullptr);

    m_Renderer->failCreate = false;
    EXPECT_NE(m_Manager->LoadTexture(png), nullptr);
}
