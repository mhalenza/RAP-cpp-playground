TEST_CASE("RAP Serdes round trip " CFG_NAME, "[serdes]")
{
    SECTION("ReadSingleCommand")
    {
        checkCmd<CFG>(RAP::Serdes::ReadSingleCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .addr = GEN_ADDR,
        });
    }
    SECTION("WriteSingleCommand")
    {
        checkCmd<CFG>(RAP::Serdes::WriteSingleCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .posted = GEN_POSTED,
            .addr = GEN_ADDR,
            .data = GEN_DATA,
        });
    }
    SECTION("ReadSeqCommand")
    {
        checkCmd<CFG>(RAP::Serdes::ReadSeqCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .start_addr = GEN_ADDR,
            .count = GEN_COUNT,
        });
    }
    // DESIGN FLAW: WriteSeqCommand::data is a span, but in order for decoding to work, it must own the data otherwise it's a use-after-free
    SECTION("WriteSeqCommand")
    {
        auto data = GEN_DATA_RNG;
        checkCmd<CFG>(RAP::Serdes::WriteSeqCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .posted = GEN_POSTED,
            .start_addr = GEN_ADDR,
            .data = GEN_DATA_RNG,
        });
    }
    SECTION("ReadCompCommand")
    {
        auto addrs = GEN_ADDR_RNG;
        checkCmd<CFG>(RAP::Serdes::ReadCompCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .addresses = GEN_ADDR_RNG,
        });
    }
    SECTION("WriteCompCommand")
    {
        auto addrs = GEN_ADDR_RNG;
        auto datas = GEN_DATA_RNG;
        REQUIRE(addrs.size() == datas.size());
        std::vector<std::pair<CFG::AddressType, CFG::DataType>> addr_data;
        addr_data.reserve(addrs.size());
        for (size_t i = 0; i < addrs.size(); i++) {
            addr_data.push_back(std::make_pair(addrs[i], datas[i]));
        }
        checkCmd<CFG>(RAP::Serdes::WriteCompCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .posted = GEN_POSTED,
            .addr_data = addr_data,
        });
    }
    SECTION("ReadModifyWriteCommand")
    {
        checkCmd<CFG>(RAP::Serdes::ReadModifyWriteCommand<CFG>{
            .transaction_id = GEN_TXN_ID,
            .posted = GEN_POSTED,
            .addr = GEN_ADDR,
            .data = GEN_DATA,
            .mask = GEN_DATA,
        });
    }

    SECTION("ReadSingleAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadSingleAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .data = GEN_DATA,
        });
    }
    SECTION("WriteSingleAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteSingleAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
        });
    }
    SECTION("ReadSeqAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadSeqAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .data = GEN_DATA_RNG,
        });
    }
    SECTION("WriteSeqAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteSeqAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
        });
    }
    SECTION("ReadCompAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadCompAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .data = GEN_DATA_RNG,
        });
    }
    SECTION("WriteCompAckResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteCompAckResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
        });
    }
    SECTION("ReadSingleNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadSingleNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("WriteSingleNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteSingleNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("ReadSeqNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadSeqNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("WriteSeqNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteSeqNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("ReadCompNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::ReadCompNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("WriteCompNakResponse")
    {
        checkResp<CFG>(RAP::Serdes::WriteCompNakResponse<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
    SECTION("Interrupt")
    {
        checkResp<CFG>(RAP::Serdes::Interrupt<CFG>{
            .transaction_id = GEN_TXN_ID,
            .status = GEN_DATA,
        });
    }
}
