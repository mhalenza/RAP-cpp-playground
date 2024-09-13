#define YALF_IMPLEMENTATION
#define RTF_IMPLEMENTATION
#include "YALF/YALF.h"
#include "ACFP/ACFP.h"
#include "RTF/RTF.h"
#include <catch2/catch_session.hpp>

using namespace std::literals::string_view_literals;

void configureLogger(ACFP::SectionGroup const& config_group, ACFP::SectionGroup const& dll_config_group);
void configureRtf(ACFP::Section const& config);

int main(int argc, char** argv)
{
    try {
        auto const config = ACFP::parseConfigFile("Config.txt");
        configureLogger(config["Logger"], config["DomainLogLevels"]);
        configureRtf(config["RegisterOperationLogging"][""]);
        return Catch::Session().run(argc, argv);
    }
    catch (std::exception const& ex) {
        if (YALF::hasGlobalLogger()) {
            LOG_CRIT("main", "Uncaught exception bubbled up to main: {}", ex.what());
            return 1;
        }
        else {
            auto cout_itr = std::ostream_iterator<char>(std::cerr);
            std::format_to(cout_itr, "Uncaught exception bubbled up to main (before logging set up): {}", ex.what());
            return 1;
        }
    }
    catch (...) {
        if (YALF::hasGlobalLogger()) {
            LOG_CRIT("main", "Uncaught exception bubbled up to main, but it is not a std::exception, so 'something bad happened'.");
            return 1;
        }
        else {
            auto cout_itr = std::ostream_iterator<char>(std::cerr);
            std::format_to(cout_itr, "Uncaught exception bubbled up to main, but it is not a std::exception, so 'something bad happened'.");
            return 1;
        }
    }
}
