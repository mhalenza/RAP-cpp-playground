#include <RAP/Serdes.h>
#include <YALF/YALF.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

template <RAP::IsConfigurationType Cfg, typename CmdType>
static inline
void checkCmd(CmdType const& in)
{
    auto serdes = RAP::Serdes::Serdes<Cfg>(256);
    LOG_INFO(&in, "Checking");
    auto const buf = serdes.encodeCommand(in);
    auto const out = serdes.decodeCommand(buf);
    auto const buf2 = serdes.encodeCommand(out);
    //REQUIRE(in == out);
    REQUIRE(buf == buf2);
}

template <RAP::IsConfigurationType Cfg, typename RespType>
static inline
void checkResp(RespType const& in)
{
    auto serdes = RAP::Serdes::Serdes<Cfg>(256);
    LOG_INFO(&in, "Checking");
    auto const buf = serdes.encodeResponse(in);
    auto const out = serdes.decodeResponse(buf);
    auto const buf2 = serdes.encodeResponse(out);
    //REQUIRE(in == out);
    REQUIRE(buf == buf2);
}

#define GEN_TXN_ID     GENERATE(Catch::Generators::take(8, Catch::Generators::random<uint8_t>(0, 0xff)))
#define GEN_ADDR       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1)))
#define GEN_DATA       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1)))
#define GEN_ADDR_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1))))
#define GEN_DATA_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1))))
#define GEN_POSTED     GENERATE(false, true)
#define GEN_COUNT      GENERATE(Catch::Generators::take(1, Catch::Generators::random<uint8_t>(0, 0xff)))

struct Rap_A24D32L2C2 {
    using AddressType = uint32_t;
    static constexpr uint8_t AddressBits = 24;
    static constexpr uint8_t AddressBytes = 3;
    using DataType = uint32_t;
    static constexpr uint8_t DataBits = 32;
    static constexpr uint8_t DataBytes = 4;
    using LengthType = uint16_t;
    static constexpr uint8_t LengthBytes = 2;
    using CrcType = uint16_t;
    static constexpr uint8_t CrcBytes = 2;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = false;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = false;
    static constexpr bool FeatureReadModifyWrite = false;
};
static_assert(RAP::IsConfigurationType<Rap_A24D32L2C2>);

struct Rap_A8D8L1C1 {
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
    static constexpr bool FeatureIncrement = false;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = false;
    static constexpr bool FeatureReadModifyWrite = false;
};
static_assert(RAP::IsConfigurationType<Rap_A8D8L1C1>);

struct Rap_A48D64L2C4 {
    using AddressType = uint64_t;
    static constexpr uint8_t AddressBits = 48;
    static constexpr uint8_t AddressBytes = 6;
    using DataType = uint64_t;
    static constexpr uint8_t DataBits = 64;
    static constexpr uint8_t DataBytes = 8;
    using LengthType = uint16_t;
    static constexpr uint8_t LengthBytes = 2;
    using CrcType = uint32_t;
    static constexpr uint8_t CrcBytes = 4;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = false;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = false;
    static constexpr bool FeatureReadModifyWrite = false;
};
static_assert(RAP::IsConfigurationType<Rap_A48D64L2C4>);

#define CFG RAP::ExampleRapCfg
#define CFG_NAME "RAP::ExampleRapCfg"
#include "SerdesTestsTemplate.inc"
#undef CFG
#undef CFG_NAME
#define CFG Rap_A24D32L2C2
#define CFG_NAME "Rap_A24D32L2C2"
#include "SerdesTestsTemplate.inc"
#undef CFG
#undef CFG_NAME
#define CFG Rap_A8D8L1C1
#define CFG_NAME "Rap_A8D8L1C1"
#include "SerdesTestsTemplate.inc"
#undef CFG
#undef CFG_NAME
#define CFG Rap_A48D64L2C4
#define CFG_NAME "Rap_A48D64L2C4"
#include "SerdesTestsTemplate.inc"
#undef CFG
#undef CFG_NAME
