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
