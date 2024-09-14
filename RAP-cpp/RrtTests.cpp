#include <RAP/RegisterTarget.h>
#include <RAP/ServerAdapter.h>
#include <YALF/YALF.h>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

template <typename AddressType, typename DataType>
class AdvDummyRegisterTarget : public RTF::IRegisterTarget<AddressType, DataType>
{
public:
    AdvDummyRegisterTarget(std::string_view name)
        : RTF::IRegisterTarget<AddressType, DataType>(name)
    {}
    virtual std::string_view getDomain() const override { return "AdvDummyRegisterTarget"; }

    virtual void write(AddressType addr, DataType data) override
    {
        LOG_NOISE(this, "write(0x{:0{}x}, 0x{:0{}x})", addr, sizeof(AddressType) * 2, data, sizeof(DataType) * 2);
        this->regs[addr] = data;
    }
    virtual DataType read(AddressType addr) override
    {
        DataType const rv = this->regs[addr];
        LOG_NOISE(this, "read(0x{:0{}x}) -> 0x{:0{}x}", addr, sizeof(AddressType) * 2, rv, sizeof(DataType) * 2);
        return rv;
    }
    virtual void readModifyWrite(AddressType addr, DataType new_data, DataType mask) override
    {
        LOG_NOISE(this, "readModifyWrite(0x{:0{}x}, 0x{:0{}x}, 0x{:0{}x})", addr, sizeof(AddressType) * 2, new_data, sizeof(DataType) * 2, mask, sizeof(DataType) * 2);
        DataType v = this->regs[addr];
        v &= ~mask;
        v |= new_data & mask;
        this->regs[addr] = v;
    }
    virtual void seqWrite(AddressType start_addr, std::span<DataType const> data, size_t increment = sizeof(DataType)) override
    {
        LOG_NOISE(this, "seqWrite(0x{:0{}x}, {}.., {})", start_addr, sizeof(AddressType) * 2, data.size(), increment);
        for (size_t i = 0; i < data.size(); i++) {
            this->regs[start_addr + (increment * i)] = data[i];
        }
    }
    virtual void seqRead(AddressType start_addr, std::span<DataType> out_data, size_t increment = sizeof(DataType)) override
    {
        LOG_NOISE(this, "seqRead(0x{:0{}x}, {}.., {})", start_addr, sizeof(AddressType) * 2, out_data.size(), increment);
        for (size_t i = 0; i < out_data.size(); i++) {
            out_data[i] = this->regs[start_addr + (increment * i)];
        }
    }
    virtual void fifoWrite(AddressType fifo_addr, std::span<DataType const> data) override
    {
        LOG_NOISE(this, "fifoWrite(0x{:0{}x}, {}..)", fifo_addr, sizeof(AddressType) * 2, data.size());
        for (auto const d : data) {
            this->regs[fifo_addr] = d;
        }
    }
    virtual void fifoRead(AddressType fifo_addr, std::span<DataType> out_data) override
    {
        LOG_NOISE(this, "fifo_read(0x{:0{}x}, {}..)", fifo_addr, sizeof(AddressType) * 2, out_data.size());
        for (auto& d : out_data) {
            d = this->regs[fifo_addr];
        }
    }
    virtual void compWrite(std::span<std::pair<AddressType, DataType> const> addr_data) override
    {
        LOG_NOISE(this, "compWrite({}..)", addr_data.size());
        for (auto const ad : addr_data) {
            this->regs[ad.first] = ad.second;
        }
    }
    virtual void compRead(std::span<AddressType const> const addresses, std::span<DataType> out_data) override
    {
        assert(addresses.size() == out_data.size());
        LOG_NOISE(this, "compRead({}..)", addresses.size());
        for (size_t i = 0; i < addresses.size(); i++) {
            out_data[i] = this->regs[addresses[i]];
        }
    }
protected:
    std::unordered_map<AddressType, DataType> regs;
};

struct RapCfg {
    using AddressType = uint8_t;
    static constexpr uint8_t AddressBits = 8;
    static constexpr uint8_t AddressBytes = 1;
    using DataType = uint8_t;
    static constexpr uint8_t DataBits = 8;
    static constexpr uint8_t DataBytes = 1;
    using LengthType = uint8_t;
    static constexpr uint8_t LengthBytes = 1;
    using CrcType = uint16_t;
    static constexpr uint8_t CrcBytes = 2;
    static constexpr bool FeatureSequential = false;
    static constexpr bool FeatureFifo = false;
    static constexpr bool FeatureIncrement = false;
    static constexpr bool FeatureCompressed = false;
    static constexpr bool FeatureInterrupt = false;
    static constexpr bool FeatureReadModifyWrite = false;
};
static_assert(RAP::IsConfigurationType<RapCfg>);

struct RapCfg_Seq  : RapCfg { static constexpr bool FeatureSequential = true; };
struct RapCfg_Fifo : RapCfg { static constexpr bool FeatureFifo = true; };
struct RapCfg_Incr : RapCfg { static constexpr bool FeatureIncrement = true; };
struct RapCfg_Comp : RapCfg { static constexpr bool FeatureCompressed = true; };
struct RapCfg_Intr : RapCfg { static constexpr bool FeatureInterrupt = true; };
struct RapCfg_RMW  : RapCfg { static constexpr bool FeatureReadModifyWrite = true; };

#define GEN_INCR       GENERATE(Catch::Generators::take(1, Catch::Generators::random<uint8_t>(0, 0xFF)))
#define GEN_ADDR       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1)))
#define GEN_DATA       GENERATE(Catch::Generators::take(1, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1)))
#define GEN_ADDR_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::AddressType>(0, (1ULL << CFG::AddressBits) - 1))))
#define GEN_DATA_RNG   GENERATE(Catch::Generators::take(1, Catch::Generators::chunk(8, Catch::Generators::random<CFG::DataType>(0, (1ULL << CFG::DataBits) - 1))))

TEST_CASE("Explore RapRegisterTarget", "[Explore][RRT]")
{
    using CFG = RapCfg_Comp;
    auto [client_xport, server_xport] = RAP::Transport::makeSyncPairedIpcTransport(512);
    client_xport->setTimeout(std::chrono::seconds(1));
    auto rap_target = RAP::RTF::RapRegisterTarget<CFG>("Rap Target", std::move(client_xport));

    auto simple_target = std::make_shared<AdvDummyRegisterTarget<CFG::AddressType, CFG::DataType>>("Adv Dummy");
    auto rap_server_adapter = RAP::RTF::RapServerAdapter<CFG>(std::move(server_xport), simple_target);

    auto fluent_target = RTF::FluentRegisterTarget{ rap_target };

    typename CFG::DataType out_data{};
    std::vector<typename CFG::DataType> data_vec(size_t(2));
    std::vector<std::pair<CFG::AddressType, CFG::DataType>> addr_data_vec(size_t(2));
    fluent_target
        #if 1
        .write(0x1, 0x2)
        .write(0x1, 0x2)
        .read(0x1, out_data)
        #endif
        #if 1
        .seqWrite(0x10, data_vec, 0, "inc=0  F")
        .seqWrite(0x11, data_vec, sizeof(CFG::DataType), "inc=DS  S")
        .seqWrite(0x12, data_vec, 13, "inc=13  I")
        #endif
        #if 1
        .fifoWrite(0x20, data_vec, "Fifo")
        #endif
        #if 1
        .compWrite(addr_data_vec, "Comp")
        #endif
        .readModifyWrite(0x1, 0x2, 0x3, "RMW")
    ;

    #if 0
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
    #endif
}

struct CFG
{
    using AddressType = uint32_t;
    static constexpr uint8_t AddressBits = 8;
    static constexpr uint8_t AddressBytes = 4;
    using DataType = uint16_t;
    static constexpr uint8_t DataBits = 8;
    static constexpr uint8_t DataBytes = 2;
    using LengthType = uint8_t;
    static constexpr uint8_t LengthBytes = 1;
    using CrcType = uint16_t;
    static constexpr uint8_t CrcBytes = 2;
    static constexpr bool FeatureSequential = true;
    static constexpr bool FeatureFifo = true;
    static constexpr bool FeatureIncrement = true;
    static constexpr bool FeatureCompressed = true;
    static constexpr bool FeatureInterrupt = true;
    static constexpr bool FeatureReadModifyWrite = true;
};
static_assert(RAP::IsConfigurationType<CFG>);

TEST_CASE("Generate UDP packets of every type", "[Serdes][UDP]")
{
    auto serdes = RAP::Serdes::Serdes<CFG>(4096);
    auto xport = RAP::Transport::makeSyncUdpTransport("localhost", 1234, "localhost", 4321, true);
    auto server_xport = RAP::Transport::makeSyncUdpTransport("localhost", 4321, "localhost", 1234, false);
    uint8_t txn_id = 0;
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadSingleCommand<CFG>{
        .transaction_id = txn_id++,
        .addr = 0x123456,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSingleCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .addr = 0x7891011,
        .data = 0xABCD,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSingleCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .addr = 0x45684,
        .data = 0xCDEF,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .start_addr = 0x646518,
        .increment = 0, // FIFO
        .count = 3,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .start_addr = 0x646518,
        .increment = 4, // SEQ
        .count = 3,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .start_addr = 0x646518,
        .increment = 13, // INCR
        .count = 3,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .start_addr = 0x1685168,
        .increment = 0, // FIFO
        .data = std::vector<CFG::DataType>{0xAA,0xBB,0xCC},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .start_addr = 0x96874,
        .increment = 0, // FIFO
        .data = std::vector<CFG::DataType>{0xDD,0xEE,0xFF},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .start_addr = 0x1685168,
        .increment = 4, // SEQ
        .data = std::vector<CFG::DataType>{0xAA,0xBB,0xCC},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .start_addr = 0x96874,
        .increment = 4, // SEQ
        .data = std::vector<CFG::DataType>{0xDD,0xEE,0xFF},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .start_addr = 0x1685168,
        .increment = 13, // INCR
        .data = std::vector<CFG::DataType>{0xAA,0xBB,0xCC},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteSeqCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .start_addr = 0x96874,
        .increment = 13, // INCR
        .data = std::vector<CFG::DataType>{0xDD,0xEE,0xFF},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadCompCommand<CFG>{
        .transaction_id = txn_id++,
        .addresses = std::vector<CFG::AddressType>{0x1122,0x3344, 0x5566},
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteCompCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .addr_data = std::vector<std::pair<CFG::AddressType, CFG::DataType>>{
            std::pair<CFG::AddressType, CFG::DataType>{0x11,0xAA},
            std::pair<CFG::AddressType, CFG::DataType>{0x22,0xBB},
            std::pair<CFG::AddressType, CFG::DataType>{0x33,0xCC},
        },
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::WriteCompCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .addr_data = std::vector<std::pair<CFG::AddressType, CFG::DataType>>{
            std::pair<CFG::AddressType, CFG::DataType>{0x11,0xAA},
            std::pair<CFG::AddressType, CFG::DataType>{0x22,0xBB},
            std::pair<CFG::AddressType, CFG::DataType>{0x33,0xCC},
        },
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadModifyWriteCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = false,
        .addr = 0x53587,
        .data = 0xCDEF,
        .mask = 0xFEDC,
    }));
    xport->send(serdes.encodeCommand(RAP::Serdes::ReadModifyWriteCommand<CFG>{
        .transaction_id = txn_id++,
        .posted = true,
        .addr = 0x68453854,
        .data = 0xFABC,
        .mask = 0xCBAF,
    }));

    xport->send(serdes.encodeResponse(RAP::Serdes::ReadSingleAckResponse<CFG>{
        .transaction_id = txn_id++,
        .data = 0xCAFE,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteSingleAckResponse<CFG>{
        .transaction_id = txn_id++,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadSeqAckResponse<CFG>{
        .transaction_id = txn_id++,
        .data = std::vector<CFG::DataType>{0xABCD, 0xFEDC},
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteSeqAckResponse<CFG>{
        .transaction_id = txn_id++,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadCompAckResponse<CFG>{
        .transaction_id = txn_id++,
        .data = std::vector<CFG::DataType>{0x1234, 0x5678},
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteCompAckResponse<CFG>{
        .transaction_id = txn_id++,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadSingleNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x1111,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteSingleNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x2222,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadSeqNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x3333,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteSeqNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x4444,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadCompNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x5555,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::WriteCompNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x6666,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadmodifywriteSingleAckResponse<CFG>{
        .transaction_id = txn_id++,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::ReadmodifywriteSingleNakResponse<CFG>{
        .transaction_id = txn_id++,
        .status = 0x7777,
    }));
    xport->send(serdes.encodeResponse(RAP::Serdes::Interrupt<CFG>{
        .transaction_id = txn_id++,
        .status = 0x8888,
    }));
}
