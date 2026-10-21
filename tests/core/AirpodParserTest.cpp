#include "core/AirpodParser.hpp"
#include <gtest/gtest.h>
#include <array>
#include <cstdint>
#include <string_view>

using winpod::AirpodParser;
using winpod::AirpodsModel;
using winpod::AirpodsStatus;

namespace
{
using Packet = std::array<uint8_t, 27>;

struct PacketBuilder
{
    uint16_t modelId{static_cast<uint16_t>(AirpodsModel::AirPodsPro2)};
    bool primaryIsLeft{true};
    bool primaryInEar{false};
    bool otherInEar{false};
    uint8_t primaryBattery{10};
    uint8_t otherBattery{10};
    uint8_t caseBattery{10};
    bool primaryCharging{false};
    bool otherCharging{false};
    bool caseCharging{false};
    bool lidClosed{false};
    uint8_t color{0};

    Packet Build() const
    {
        Packet p{};
        p[0] = 0x07;
        p[1] = 25;
        p[2] = 0x01;
        p[3] = static_cast<uint8_t>(modelId >> 8);
        p[4] = static_cast<uint8_t>(modelId & 0xFF);
        p[5] = static_cast<uint8_t>((primaryIsLeft ? 0x20 : 0) | (primaryInEar ? 0x02 : 0) | (otherInEar ? 0x08 : 0));
        p[6] = static_cast<uint8_t>((otherBattery << 4) | (primaryBattery & 0x0F));
        const uint8_t chargeFlags =
            static_cast<uint8_t>((primaryCharging ? 0x1 : 0) | (otherCharging ? 0x2 : 0) | (caseCharging ? 0x4 : 0));
        p[7] = static_cast<uint8_t>((chargeFlags << 4) | (caseBattery & 0x0F));
        p[8] = lidClosed ? 0x08 : 0x00;
        p[9] = color;
        return p;
    }
};

std::optional<AirpodsStatus> Parse(const Packet& p)
{
    AirpodParser parser;
    return parser.Parse(p.data(), static_cast<uint32_t>(p.size()));
}
} // namespace


TEST(AirpodParserParse, RejectsNullData)
{
    AirpodParser parser;
    EXPECT_FALSE(parser.Parse(nullptr, 27).has_value());
}

TEST(AirpodParserParse, RejectsWrongLength)
{
    AirpodParser parser;
    const Packet p = PacketBuilder{}.Build();
    EXPECT_FALSE(parser.Parse(p.data(), 26).has_value());
    EXPECT_FALSE(parser.Parse(p.data(), 0).has_value());
}

TEST(AirpodParserParse, RejectsNonProximityPairingType)
{
    Packet p = PacketBuilder{}.Build();
    p[0] = 0x10;
    EXPECT_FALSE(Parse(p).has_value());
}

TEST(AirpodParserParse, RejectsWrongLengthByte)
{
    Packet p = PacketBuilder{}.Build();
    p[1] = 24;
    EXPECT_FALSE(Parse(p).has_value());
}


TEST(AirpodParserParse, ReadsKnownModels)
{
    struct Case
    {
        AirpodsModel model;
        std::string_view name;
    };
    const Case cases[] = {
        {AirpodsModel::AirPods1, "AirPods"},
        {AirpodsModel::AirPods2, "AirPods 2"},
        {AirpodsModel::AirPods3, "AirPods 3"},
        {AirpodsModel::AirPods4, "AirPods 4"},
        {AirpodsModel::AirPods4Anc, "AirPods 4 (ANC)"},
        {AirpodsModel::AirPodsPro, "AirPods Pro"},
        {AirpodsModel::AirPodsPro2, "AirPods Pro 2"},
        {AirpodsModel::AirPodsPro2UsbC, "AirPods Pro 2 (USB-C)"},
        {AirpodsModel::AirPodsMax, "AirPods Max"},
        {AirpodsModel::AirPodsMaxUsbC, "AirPods Max (USB-C)"},
    };
    for (const auto& c : cases)
    {
        SCOPED_TRACE(c.name);
        PacketBuilder b;
        b.modelId = static_cast<uint16_t>(c.model);
        const auto status = Parse(b.Build());
        ASSERT_TRUE(status.has_value());
        EXPECT_EQ(status->model, c.model);
        EXPECT_EQ(status->modelId, static_cast<uint16_t>(c.model));
        EXPECT_EQ(status->name, c.name);
    }
}

TEST(AirpodParserParse, UnknownModelKeepsRawId)
{
    PacketBuilder b;
    b.modelId = 0x9920;
    const auto status = Parse(b.Build());
    ASSERT_TRUE(status.has_value());
    EXPECT_EQ(status->model, AirpodsModel::Unknown);
    EXPECT_EQ(status->modelId, 0x9920);
    EXPECT_EQ(status->name, "Unknown");
}

TEST(AirpodParserParse, ReadsColor)
{
    PacketBuilder b;
    b.color = 0x05;
    EXPECT_EQ(Parse(b.Build())->color, 0x05);
}


TEST(AirpodParserParse, PrimaryLeftMapsFieldsToLeft)
{
    PacketBuilder b;
    b.primaryIsLeft = true;
    b.primaryBattery = 10;
    b.otherBattery = 8;
    b.primaryCharging = true;
    b.primaryInEar = true;
    const auto s = Parse(b.Build());
    ASSERT_TRUE(s.has_value());

    EXPECT_TRUE(s->left.isLeft);
    EXPECT_FALSE(s->right.isLeft);

    EXPECT_TRUE(s->left.isPrimary);
    EXPECT_FALSE(s->right.isPrimary);
    EXPECT_EQ(s->left.battery, 100);
    EXPECT_EQ(s->right.battery, 80);
    EXPECT_TRUE(s->left.isCharging);
    EXPECT_FALSE(s->right.isCharging);
    EXPECT_TRUE(s->left.isInEar);
    EXPECT_FALSE(s->right.isInEar);
}

TEST(AirpodParserParse, PrimaryRightMapsFieldsToRight)
{
    PacketBuilder b;
    b.primaryIsLeft = false;
    b.primaryBattery = 10;
    b.otherBattery = 8;
    b.primaryCharging = true;
    b.primaryInEar = true;
    const auto s = Parse(b.Build());
    ASSERT_TRUE(s.has_value());

    EXPECT_TRUE(s->right.isPrimary);
    EXPECT_FALSE(s->left.isPrimary);
    EXPECT_EQ(s->right.battery, 100);
    EXPECT_EQ(s->left.battery, 80);
    EXPECT_TRUE(s->right.isCharging);
    EXPECT_FALSE(s->left.isCharging);
    EXPECT_TRUE(s->right.isInEar);
    EXPECT_FALSE(s->left.isInEar);
}

TEST(AirpodParserParse, OtherBudFlagsGoToOppositeSide)
{
    for (const bool primaryIsLeft : {true, false})
    {
        SCOPED_TRACE(primaryIsLeft ? "primary left" : "primary right");
        PacketBuilder b;
        b.primaryIsLeft = primaryIsLeft;
        b.otherCharging = true;
        b.otherInEar = true;
        const auto s = Parse(b.Build());
        ASSERT_TRUE(s.has_value());

        const auto& other = primaryIsLeft ? s->right : s->left;
        const auto& primary = primaryIsLeft ? s->left : s->right;
        EXPECT_TRUE(other.isCharging);
        EXPECT_TRUE(other.isInEar);
        EXPECT_FALSE(other.isPrimary);
        EXPECT_FALSE(primary.isCharging);
        EXPECT_FALSE(primary.isInEar);
    }
}

TEST(AirpodParserParse, BothInEar)
{
    PacketBuilder b;
    b.primaryInEar = true;
    b.otherInEar = true;
    const auto s = Parse(b.Build());
    ASSERT_TRUE(s.has_value());
    EXPECT_TRUE(s->left.isInEar);
    EXPECT_TRUE(s->right.isInEar);
}


TEST(AirpodParserParse, BatteryNibbleToPercent)
{
    for (uint8_t nibble = 0; nibble <= 10; ++nibble)
    {
        SCOPED_TRACE(static_cast<int>(nibble));
        PacketBuilder b;
        b.primaryBattery = nibble;
        b.otherBattery = nibble;
        b.caseBattery = nibble;
        const auto s = Parse(b.Build());
        ASSERT_TRUE(s.has_value());
        EXPECT_EQ(s->left.battery, nibble * 10);
        EXPECT_EQ(s->right.battery, nibble * 10);
        EXPECT_EQ(s->chargingCase.battery, nibble * 10);
    }
}

TEST(AirpodParserParse, BatteryUnavailableIsNullopt)
{
    for (const uint8_t nibble : {uint8_t{11}, uint8_t{14}, uint8_t{15}})
    {
        SCOPED_TRACE(static_cast<int>(nibble));
        PacketBuilder b;
        b.otherBattery = nibble;
        b.caseBattery = nibble;
        const auto s = Parse(b.Build());
        ASSERT_TRUE(s.has_value());
        EXPECT_FALSE(s->right.battery.has_value());
        EXPECT_FALSE(s->chargingCase.battery.has_value());
        EXPECT_EQ(s->left.battery, 100);
    }
}


TEST(AirpodParserParse, CaseCharging)
{
    PacketBuilder b;
    b.caseCharging = true;
    b.caseBattery = 4;
    const auto s = Parse(b.Build());
    ASSERT_TRUE(s.has_value());
    EXPECT_TRUE(s->chargingCase.isCharging);
    EXPECT_EQ(s->chargingCase.battery, 40);
    EXPECT_FALSE(s->left.isCharging);
    EXPECT_FALSE(s->right.isCharging);
}

TEST(AirpodParserParse, LidOpenAndClosed)
{
    PacketBuilder b;
    b.lidClosed = false;
    EXPECT_TRUE(Parse(b.Build())->chargingCase.isLidOpen);

    b.lidClosed = true;
    EXPECT_FALSE(Parse(b.Build())->chargingCase.isLidOpen);
}


TEST(AirpodParserParse, FullPacket)
{
    PacketBuilder b;
    b.modelId = static_cast<uint16_t>(AirpodsModel::AirPodsPro2UsbC);
    b.primaryIsLeft = false;
    b.primaryBattery = 9;
    b.otherBattery = 7;
    b.caseBattery = 5;
    b.primaryInEar = true;
    b.otherCharging = true;
    b.lidClosed = false;
    const auto s = Parse(b.Build());
    ASSERT_TRUE(s.has_value());

    EXPECT_EQ(s->model, AirpodsModel::AirPodsPro2UsbC);
    EXPECT_EQ(s->name, "AirPods Pro 2 (USB-C)");

    EXPECT_EQ(s->left.battery, 70);
    EXPECT_TRUE(s->left.isCharging);
    EXPECT_FALSE(s->left.isInEar);
    EXPECT_FALSE(s->left.isPrimary);

    EXPECT_EQ(s->right.battery, 90);
    EXPECT_FALSE(s->right.isCharging);
    EXPECT_TRUE(s->right.isInEar);
    EXPECT_TRUE(s->right.isPrimary);

    EXPECT_EQ(s->chargingCase.battery, 50);
    EXPECT_FALSE(s->chargingCase.isCharging);
    EXPECT_TRUE(s->chargingCase.isLidOpen);
}


TEST(AirpodParserHardwareId, ParsesBluetoothHardwareId)
{
    const auto id =
        AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b-0000-1000-8000-00805f9b34fb}_VID&0001004c_PID&2014");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->vendorIdSource, 0x0001);
    EXPECT_EQ(id->vendorId, 0x004C);
    EXPECT_EQ(id->productId, 0x2014);
}

TEST(AirpodParserHardwareId, IsCaseInsensitive)
{
    const auto lower = AirpodParser::ParseHardwareId(L"bthenum\\{0000110b}_vid&0001004c_pid&200e");
    const auto upper = AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110B}_VID&0001004C_PID&200E");
    ASSERT_TRUE(lower.has_value());
    ASSERT_TRUE(upper.has_value());
    EXPECT_EQ(lower->vendorId, upper->vendorId);
    EXPECT_EQ(lower->productId, 0x200E);
    EXPECT_EQ(upper->productId, 0x200E);
}

TEST(AirpodParserHardwareId, AcceptsIdWithoutServiceGuid)
{
    const auto id = AirpodParser::ParseHardwareId(L"BTHENUM\\VID&0001004C_PID&2024&REV&0100");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(id->productId, 0x2024);
}

TEST(AirpodParserHardwareId, RejectsMalformedIds)
{
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b}_LOCALMFG&0002").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b}_VID&0001004c").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b}_VID&004c_PID&2014").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b}_VID&0001004c_PID&20").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"BTHENUM\\{0000110b}_VID&0001004c_PID&20XZ").has_value());
    EXPECT_FALSE(AirpodParser::ParseHardwareId(L"USB\\VID_05AC&PID_8290").has_value());
}

TEST(AirpodParserHardwareId, ProductIdMapsToBleModel)
{
    struct Case
    {
        uint16_t productId;
        AirpodsModel model;
    };
    const Case cases[] = {
        {0x2002, AirpodsModel::AirPods1},     {0x200F, AirpodsModel::AirPods2},
        {0x2013, AirpodsModel::AirPods3},     {0x2019, AirpodsModel::AirPods4},
        {0x201B, AirpodsModel::AirPods4Anc},  {0x200E, AirpodsModel::AirPodsPro},
        {0x2014, AirpodsModel::AirPodsPro2},  {0x2024, AirpodsModel::AirPodsPro2UsbC},
        {0x200A, AirpodsModel::AirPodsMax},   {0x201F, AirpodsModel::AirPodsMaxUsbC},
    };
    AirpodParser parser;
    for (const auto& c : cases)
    {
        SCOPED_TRACE(::testing::Message() << "PID 0x" << std::hex << c.productId);
        EXPECT_EQ(parser.ToModel(AirpodParser::ModelIdFromProductId(c.productId)), c.model);
    }
}

TEST(AirpodParserHardwareId, NonAirPodsProductIsUnknown)
{
    AirpodParser parser;
    EXPECT_EQ(parser.ToModel(AirpodParser::ModelIdFromProductId(0x200B)), AirpodsModel::Unknown);
    EXPECT_EQ(parser.ToModel(AirpodParser::ModelIdFromProductId(0x0000)), AirpodsModel::Unknown);
}
