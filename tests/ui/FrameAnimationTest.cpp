#include "fakes/FakeRenderer.hpp"
#include "resource/ResourceManager.hpp"
#include "ui/FrameAnimation.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace fs = std::filesystem;
using winpod::FrameAnimation;
using winpod::ResourceManager;
using winpod::test::FakeRenderer;

namespace
{
const fs::path kResources{WINPOD_RESOURCES_DIR};
const fs::path kBuds = kResources / "buds";
const fs::path kCase = kResources / "case";

// Load / Unload / AspectRatio không cần ImGui context (chỉ Draw cần), nên test được không cần cửa sổ.
class FrameAnimationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        m_TempDir = fs::temp_directory_path() / "winpod_tests" / (std::string("anim_") + info->name());
        fs::remove_all(m_TempDir);
        fs::create_directories(m_TempDir);

        m_Renderer = std::make_shared<FakeRenderer>();
        ResourceManager::instance().Init(m_Renderer);
    }

    void TearDown() override
    {
        ResourceManager::instance().Shutdown();
        std::error_code ec;
        fs::remove_all(m_TempDir, ec);
    }

    fs::path WriteJson(const std::string& name, const std::string& content)
    {
        const fs::path path = m_TempDir / name;
        std::ofstream(path) << content;
        return path;
    }

    // JSON hợp lệ về cú pháp, trỏ vào PNG thật (đường dẫn tuyệt đối) với bố cục tuỳ ý.
    static std::string SheetJson(const std::string& layout)
    {
        const std::string image = (kBuds / "AirPods_1.png").generic_string();
        return R"({ "image": ")" + image + R"(", )" + layout + " }";
    }

    fs::path m_TempDir;
    std::shared_ptr<FakeRenderer> m_Renderer;
};
} // namespace

// ---------------------------------------------------------------- load

TEST_F(FrameAnimationTest, LoadsEverySheet)
{
    for (const auto& folder : {kBuds, kCase})
    {
        for (const auto& entry : fs::directory_iterator(folder))
        {
            if (entry.path().extension() != ".json")
                continue;
            SCOPED_TRACE(entry.path().generic_string());
            FrameAnimation anim;
            ASSERT_TRUE(anim.Load(entry.path()));
            EXPECT_TRUE(anim.IsLoaded());
            EXPECT_GT(anim.AspectRatio(), 0.0f); // theo vùng "content" của từng sheet
        }
    }
}

TEST_F(FrameAnimationTest, AspectRatioUsesContentRect)
{
    FrameAnimation anim;
    ASSERT_TRUE(anim.Load(WriteJson(
        "content.json",
        SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 180, "fps": 30,
                     "content": { "x": 20, "y": 60, "w": 200, "h": 100 })"))));
    EXPECT_FLOAT_EQ(anim.AspectRatio(), 2.0f);
}

TEST_F(FrameAnimationTest, AspectRatioDefaultsToWholeFrame)
{
    FrameAnimation anim;
    ASSERT_TRUE(anim.Load(WriteJson(
        "no_content.json",
        SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 180, "fps": 30)"))));
    EXPECT_FLOAT_EQ(anim.AspectRatio(), 256.0f / 224.0f);
}

TEST_F(FrameAnimationTest, FailsWhenContentOutsideFrame)
{
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(WriteJson(
        "bad_content.json",
        SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 180, "fps": 30,
                     "content": { "x": 200, "y": 0, "w": 100, "h": 100 })"))));
    EXPECT_FALSE(anim.IsLoaded());
}

TEST_F(FrameAnimationTest, ImagePathIsRelativeToJson)
{
    FrameAnimation anim;
    ASSERT_TRUE(anim.Load(kCase / "AirPods_Pro_2.json"));
    EXPECT_EQ(anim.ImagePath().lexically_normal(), (kCase / "AirPods_Pro_2.png").lexically_normal());
}

TEST_F(FrameAnimationTest, TwoAnimationsShareOneTexture)
{
    FrameAnimation a, b;
    ASSERT_TRUE(a.Load(kBuds / "AirPods_1.json"));
    ASSERT_TRUE(b.Load(kBuds / "AirPods_1.json"));
    EXPECT_EQ(m_Renderer->createCount, 1); // ResourceManager cache: không tạo texture lần hai
}

TEST_F(FrameAnimationTest, UnloadKeepsTextureInResourceManager)
{
    FrameAnimation anim;
    ASSERT_TRUE(anim.Load(kBuds / "AirPods_1.json"));
    anim.Unload();

    EXPECT_FALSE(anim.IsLoaded());
    EXPECT_EQ(m_Renderer->destroyCount, 0); // chỉ bỏ tham chiếu, texture vẫn trong cache
    EXPECT_NE(ResourceManager::instance().GetTexture(kBuds / "AirPods_1.png"), nullptr);
}

TEST_F(FrameAnimationTest, NotLoadedAfterResourceManagerUnloadsTexture)
{
    FrameAnimation anim;
    ASSERT_TRUE(anim.Load(kBuds / "AirPods_1.json"));

    ResourceManager::instance().UnloadTexture(kBuds / "AirPods_1.png");

    EXPECT_FALSE(anim.IsLoaded()); // không vẽ texture đã huỷ
}

// ------------------------------------------------------------- thất bại

TEST_F(FrameAnimationTest, FailsWhenJsonMissing)
{
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(kBuds / "khong_ton_tai.json"));
    EXPECT_FALSE(anim.IsLoaded());
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(FrameAnimationTest, FailsOnMalformedJson)
{
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(WriteJson("broken.json", R"({ "image": "AirPods_1.png", )")));
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(FrameAnimationTest, FailsWhenKeyMissing)
{
    // Thiếu "fps".
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(
        WriteJson("no_fps.json", SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 180)"))));
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(FrameAnimationTest, FailsOnNonPositiveValues)
{
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(WriteJson(
        "zero_frames.json",
        SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 0, "fps": 30)"))));
    EXPECT_EQ(m_Renderer->createCount, 0);
}

TEST_F(FrameAnimationTest, FailsWhenImageMissing)
{
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(WriteJson(
        "no_image.json",
        R"({ "image": "khong_ton_tai.png", "frameWidth": 256, "frameHeight": 224, "columns": 12, "frames": 180, "fps": 30 })")));
    EXPECT_FALSE(anim.IsLoaded());
}

TEST_F(FrameAnimationTest, FailsWhenImageTooSmallForLayout)
{
    // buds/AirPods_1.png là 12x15 ô 256x224; khai 20 cột thì ảnh không đủ rộng.
    FrameAnimation anim;
    EXPECT_FALSE(anim.Load(WriteJson(
        "too_many_columns.json",
        SheetJson(R"("frameWidth": 256, "frameHeight": 224, "columns": 20, "frames": 180, "fps": 30)"))));
    EXPECT_FALSE(anim.IsLoaded());
}
