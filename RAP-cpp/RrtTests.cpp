#include <RAP/RegisterTarget.h>
#include <RAP/ServerAdapter.h>
#include <RTF/RTF_SimpleDummyTarget.h>
#include <YALF/YALF.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

struct RapCfg {
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
static_assert(RAP::IsConfigurationType<RapCfg>);

#define GEN_INCR       GENERATE(Catch::Generators::take(1, Catch::Generators::random<uint8_t>(0, 0xFF)))
#define GEN_ADDR       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1)))
#define GEN_DATA       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1)))
#define GEN_ADDR_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1))))
#define GEN_DATA_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1))))

TEST_CASE("Explore RapRegisterTarget", "[Explore][RRT]")
{

    using CFG = RapCfg;
    auto [client_xport, server_xport] = RAP::Transport::makeSyncPairedIpcTransport(512);
    client_xport->setTimeout(std::chrono::seconds(1));
    auto rap_target = RAP::RTF::RapRegisterTarget<CFG>("Rap Target", std::move(client_xport));

    auto simple_target = std::make_shared<RTF::SimpleDummyRegisterTarget<CFG::AddressType, CFG::DataType>>("Simple Dummy");
    auto rap_server_adapter = RAP::RTF::RapServerAdapter<CFG>(std::move(server_xport), simple_target);

    auto fluent_target = RTF::FluentRegisterTarget{ rap_target };

    SECTION("Write & Read")
    {
        auto a = GEN_ADDR;
        auto d = GEN_DATA;
        CHECK_NOTHROW(fluent_target.write(a, d));
        CHECK(fluent_target.read(a) == d);
    }
    SECTION("ReadModifyWrite")
    {
        auto a = GEN_ADDR;
        auto d = 0xAA;// GEN_DATA;
        auto m = 0x0F; // GEN_DATA;
        CHECK_NOTHROW(fluent_target.write(a, ~0));
        CHECK_NOTHROW(fluent_target.readModifyWrite(a, d & m, m));
        CHECK(fluent_target.read(a) == CFG::DataType(((~0) & ~m) | (d & m)));
    }
    SECTION("Seq Write & Read")
    {
        auto a = GEN_ADDR;
        auto i = GEN_INCR;
        auto d = GEN_DATA_RNG;
        CHECK_NOTHROW(fluent_target.seqWrite(a, d, i));
        std::vector<CFG::DataType> od(d.size());
        CHECK_NOTHROW(fluent_target.seqRead(a, od, i));
        CHECK(d == od);
    }
    SECTION("FIFO Write & Read")
    {
        auto a = GEN_ADDR;
        auto d = GEN_DATA_RNG;
        CHECK_NOTHROW(fluent_target.fifoWrite(a, d));
        CHECK(fluent_target.read(a) == d[d.size() - 1]);
    }
}
