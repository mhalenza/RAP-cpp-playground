#include <RAP/Serdes.h>
#include <YALF/YALF.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

struct SmallCfg
{
    using AddressType = uint8_t;
    static constexpr uint8_t AddressBits = 8;
    static constexpr uint8_t AddressBytes = 1;
    using DataType = uint8_t;
    static constexpr uint8_t DataBits = 8;
    static constexpr uint8_t DataBytes = 1;
    using LengthType = uint8_t;
    static constexpr uint8_t LengthBytes = 1;
    using CrcType = uint8_t;
    static constexpr uint8_t CrcBytes = 1;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = true;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = true;
    static constexpr bool FeatureReadModifyWrite = true;
};
static_assert(RAP::IsConfigurationType<SmallCfg>);

struct LargeCfg
{
    using AddressType = uint64_t;
    static constexpr uint8_t AddressBits = 64;
    static constexpr uint8_t AddressBytes = 8;
    using DataType = uint64_t;
    static constexpr uint8_t DataBits = 64;
    static constexpr uint8_t DataBytes = 8;
    using LengthType = uint32_t; // TODO: Could lengths be 8 bytes?
    static constexpr uint8_t LengthBytes = 4;
    using CrcType = uint32_t;
    static constexpr uint8_t CrcBytes = 4;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = true;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = true;
    static constexpr bool FeatureReadModifyWrite = true;
};
static_assert(RAP::IsConfigurationType<LargeCfg>);

struct SmallABigDCfg
{
    using AddressType = uint8_t;
    static constexpr uint8_t AddressBits = 8;
    static constexpr uint8_t AddressBytes = 1;
    using DataType = uint64_t;
    static constexpr uint8_t DataBits = 64;
    static constexpr uint8_t DataBytes = 8;
    using LengthType = uint8_t;
    static constexpr uint8_t LengthBytes = 1;
    using CrcType = uint8_t;
    static constexpr uint8_t CrcBytes = 1;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = true;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = true;
    static constexpr bool FeatureReadModifyWrite = true;
};
static_assert(RAP::IsConfigurationType<SmallABigDCfg>);

struct BigASmallDCfg
{
    using AddressType = uint64_t;
    static constexpr uint8_t AddressBits = 64;
    static constexpr uint8_t AddressBytes = 8;
    using DataType = uint8_t;
    static constexpr uint8_t DataBits = 8;
    static constexpr uint8_t DataBytes = 1;
    using LengthType = uint8_t;
    static constexpr uint8_t LengthBytes = 1;
    using CrcType = uint8_t;
    static constexpr uint8_t CrcBytes = 1;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = true;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = true;
    static constexpr bool FeatureReadModifyWrite = true;
};
static_assert(RAP::IsConfigurationType<BigASmallDCfg>);

template <template<typename> typename CmdType, typename Cfg>
void measure(std::string_view type_name, std::string_view size_name)
{
    using namespace RAP::Serdes;
    auto const cmd_size = Serdes<Cfg>{4096}.encodeCommand(CmdType<Cfg>{}).size();
    using AckType = CommandResponseRelationshipTrait<CmdType<Cfg>>::AckResponseType;
    using NakType = CommandResponseRelationshipTrait<CmdType<Cfg>>::NakResponseType;
    auto const ack_size = Serdes<Cfg>{4096}.encodeResponse(AckType{}).size();
    auto const nak_size = Serdes<Cfg>{4096}.encodeResponse(NakType{}).size();
    LOG_INFO(type_name, "{}: Cmd = {}  Ack = {}  Nak = {}", size_name, cmd_size, ack_size, nak_size);
    CHECK(cmd_size <= Serdes<Cfg>::minimum_max_message_size);
    CHECK(ack_size <= Serdes<Cfg>::minimum_max_message_size);
    CHECK(nak_size <= Serdes<Cfg>::minimum_max_message_size);
}

TEST_CASE("Collect messages sizes", "[sizing]")
{
    // Verify that any serialized message (with any vectors set to zero size) are smaller than the minimum required by Serdes
    // This verifies that the Serdes-imposed minimum is actually sufficient
    measure<RAP::Serdes::ReadSingleCommand, SmallCfg>("ReadSingleCommand", "Small");
    measure<RAP::Serdes::WriteSingleCommand, SmallCfg>("WriteSingleCommand", "Small");
    measure<RAP::Serdes::ReadSeqCommand, SmallCfg>("ReadSeqCommand", "Small");
    measure<RAP::Serdes::WriteSeqCommand, SmallCfg>("WriteSeqCommand", "Small");
    measure<RAP::Serdes::ReadCompCommand, SmallCfg>("ReadCompCommand", "Small");
    measure<RAP::Serdes::WriteCompCommand, SmallCfg>("WriteCompCommand", "Small");
    measure<RAP::Serdes::ReadModifyWriteCommand, SmallCfg>("ReadModifyWriteCommand", "Small");

    measure<RAP::Serdes::ReadSingleCommand, LargeCfg>("ReadSingleCommand", "Large");
    measure<RAP::Serdes::WriteSingleCommand, LargeCfg>("WriteSingleCommand", "Large");
    measure<RAP::Serdes::ReadSeqCommand, LargeCfg>("ReadSeqCommand", "Large");
    measure<RAP::Serdes::WriteSeqCommand, LargeCfg>("WriteSeqCommand", "Large");
    measure<RAP::Serdes::ReadCompCommand, LargeCfg>("ReadCompCommand", "Large");
    measure<RAP::Serdes::WriteCompCommand, LargeCfg>("WriteCompCommand", "Large");
    measure<RAP::Serdes::ReadModifyWriteCommand, LargeCfg>("ReadModifyWriteCommand", "Large");

    #if 0
    measure<RAP::Serdes::ReadSingleCommand, SmallABigDCfg>("ReadSingleCommand", "addrDATA");
    measure<RAP::Serdes::WriteSingleCommand, SmallABigDCfg>("WriteSingleCommand", "addrDATA");
    measure<RAP::Serdes::ReadSeqCommand, SmallABigDCfg>("ReadSeqCommand", "addrDATA");
    measure<RAP::Serdes::WriteSeqCommand, SmallABigDCfg>("WriteSeqCommand", "addrDATA");
    measure<RAP::Serdes::ReadCompCommand, SmallABigDCfg>("ReadCompCommand", "addrDATA");
    measure<RAP::Serdes::WriteCompCommand, SmallABigDCfg>("WriteCompCommand", "addrDATA");
    measure<RAP::Serdes::ReadModifyWriteCommand, SmallABigDCfg>("ReadModifyWriteCommand", "addrDATA");

    measure<RAP::Serdes::ReadSingleCommand, BigASmallDCfg>("ReadSingleCommand", "ADDRdata");
    measure<RAP::Serdes::WriteSingleCommand, BigASmallDCfg>("WriteSingleCommand", "ADDRdata");
    measure<RAP::Serdes::ReadSeqCommand, BigASmallDCfg>("ReadSeqCommand", "ADDRdata");
    measure<RAP::Serdes::WriteSeqCommand, BigASmallDCfg>("WriteSeqCommand", "ADDRdata");
    measure<RAP::Serdes::ReadCompCommand, BigASmallDCfg>("ReadCompCommand", "ADDRdata");
    measure<RAP::Serdes::WriteCompCommand, BigASmallDCfg>("WriteCompCommand", "ADDRdata");
    measure<RAP::Serdes::ReadModifyWriteCommand, BigASmallDCfg>("ReadModifyWriteCommand", "ADDRdata");
    #endif
}

template <typename Cfg>
static inline
void checkSeqRead(RAP::Serdes::Serdes<Cfg> serdes, char const* sect)
{
    SECTION(sect)
    {
        using namespace RAP::Serdes;
        auto const max_count = serdes.getMaxSeqReadCount();
        auto const max_count_szt = static_cast<size_t>(max_count);
        CHECK_NOTHROW(serdes.encodeCommand(ReadSeqCommand<Cfg>{.count = max_count}));
        CHECK_NOTHROW(serdes.encodeResponse(ReadSeqAckResponse<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_szt)}));
        if (max_count != std::numeric_limits<typename Cfg::LengthType>::max()) {
            auto const max_count_p1 = static_cast<Cfg::LengthType>(max_count + 1);
            auto const max_count_p1_szt = static_cast<size_t>(max_count_p1);
            CHECK_THROWS(serdes.encodeCommand(ReadSeqCommand<Cfg>{.count = max_count_p1}));
            CHECK_THROWS(serdes.encodeResponse(ReadSeqAckResponse<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_p1_szt)}));
        }
    }
}
template <typename Cfg>
static inline
void checkSeqWrite(RAP::Serdes::Serdes<Cfg> serdes, char const* sect)
{
    SECTION(sect)
    {
        using namespace RAP::Serdes;
        auto const max_count = serdes.getMaxSeqWriteCount();
        auto const max_count_szt = static_cast<size_t>(max_count);
        CHECK_NOTHROW(serdes.encodeCommand(WriteSeqCommand<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_szt)}));
        if (max_count != std::numeric_limits<typename Cfg::LengthType>::max()) {
            auto const max_count_p1 = static_cast<Cfg::LengthType>(max_count + 1);
            auto const max_count_p1_szt = static_cast<size_t>(max_count_p1);
            CHECK_THROWS(serdes.encodeCommand(WriteSeqCommand<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_p1_szt)}));
        }
    }
}
template <typename Cfg>
static inline
void checkCompRead(RAP::Serdes::Serdes<Cfg> serdes, char const* sect)
{
    SECTION(sect)
    {
        using namespace RAP::Serdes;
        auto const max_count = serdes.getMaxCompReadCount();
        auto const max_count_szt = static_cast<size_t>(max_count);
        CHECK_NOTHROW(serdes.encodeCommand(ReadCompCommand<Cfg>{.addresses = std::vector<typename Cfg::AddressType>(max_count_szt)}));
        CHECK_NOTHROW(serdes.encodeResponse(ReadCompAckResponse<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_szt)}));
        if (max_count != std::numeric_limits<typename Cfg::LengthType>::max()) {
            auto const max_count_p1 = static_cast<Cfg::LengthType>(max_count + 1);
            auto const max_count_p1_szt = static_cast<size_t>(max_count_p1);
            CHECK_THROWS(serdes.encodeCommand(ReadCompCommand<Cfg>{.addresses = std::vector<typename Cfg::AddressType>(max_count_p1_szt)}));
            CHECK_THROWS(serdes.encodeResponse(ReadCompAckResponse<Cfg>{.data = std::vector<typename Cfg::DataType>(max_count_p1_szt)}));
        }
    }
}
template <typename Cfg>
static inline
void checkCompWrite(RAP::Serdes::Serdes<Cfg> serdes, char const* sect)
{
    SECTION(sect)
    {
        using namespace RAP::Serdes;
        auto const max_count = serdes.getMaxCompWriteCount();
        auto const max_count_szt = static_cast<size_t>(max_count);
        CHECK_NOTHROW(serdes.encodeCommand(WriteCompCommand<Cfg>{.addr_data = std::vector<std::pair<typename Cfg::AddressType, typename Cfg::DataType>>(max_count_szt)}));
        if (max_count != std::numeric_limits<typename Cfg::LengthType>::max()) {
            auto const max_count_p1 = static_cast<Cfg::LengthType>(max_count + 1);
            auto const max_count_p1_szt = static_cast<size_t>(max_count_p1);
            CHECK_THROWS(serdes.encodeCommand(WriteCompCommand<Cfg>{.addr_data = std::vector<std::pair<typename Cfg::AddressType, typename Cfg::DataType>>(max_count_p1_szt)}));
        }
    }
}

TEST_CASE("Check maximum message size boundariees", "[sizing]")
{
    using namespace RAP::Serdes;
    //auto const max_message_size = GENERATE(32, 64, 128, 256, 512, 1024, 2048, 4096);
    auto const max_message_size = GENERATE(
        32, // Serdes<X>::minimum_max_message_size
        Catch::Generators::range(33, 32+16),
        Catch::Generators::range(64-16, 64+16),
        Catch::Generators::range(128-16, 128+16),
        Catch::Generators::range(256-16, 256+16),
        Catch::Generators::range(512-16, 512+16),
        Catch::Generators::range(1024-16, 1024+16),
        Catch::Generators::range(2048-16, 2048+16),
        Catch::Generators::range(4096-16, 4096+16)
    );
    //LOG_NOTICE("Test", "Max Message Size = {}", max_message_size);

    SECTION("SmallCfg")
    {
        auto serdes = Serdes<SmallCfg>(max_message_size);
        checkSeqRead(serdes, "Seq Read");
        checkSeqWrite(serdes, "Seq Write");
        checkCompRead(serdes, "Comp Read");
        checkCompWrite(serdes, "Comp Write");
    }

    SECTION("LargeCfg")
    {
        auto serdes = Serdes<LargeCfg>(max_message_size);
        checkSeqRead(serdes, "Seq Read");
        checkSeqWrite(serdes, "Seq Write");
        checkCompRead(serdes, "Comp Read");
        checkCompWrite(serdes, "Comp Write");
    }

    SECTION("SmallABigDCfg")
    {
        auto serdes = Serdes<SmallABigDCfg>(max_message_size);
        checkSeqRead(serdes, "Seq Read");
        checkSeqWrite(serdes, "Seq Write");
        checkCompRead(serdes, "Comp Read");
        checkCompWrite(serdes, "Comp Write");
    }

    SECTION("BigASmallDCfg")
    {
        auto serdes = Serdes<BigASmallDCfg>(max_message_size);
        checkSeqRead(serdes, "Seq Read");
        checkSeqWrite(serdes, "Seq Write");
        checkCompRead(serdes, "Comp Read");
        checkCompWrite(serdes, "Comp Write");
    }
}
