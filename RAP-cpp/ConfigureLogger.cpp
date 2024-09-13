#include <ACFP/ACFP.h>
#include <YALF/YALF.h>
#include <YALF/YALF_DeferredSink.h>

void configureLogger(ACFP::SectionGroup const& config_group, ACFP::SectionGroup const& dll_config_group)
{
    auto const getConfigValueMaker = [](ACFP::SectionGroup const& group, std::string_view subkey) -> auto {
        auto const& section = group[subkey];
        auto const& global_section = group[""];
        return [=](std::string_view key) -> std::optional<std::string_view> {
            if (auto v = global_section.getField(key); v.has_value()) return v.value();
            if (auto v = section.getField(key); v.has_value()) return v.value();
            return std::nullopt;
        };
    };

    auto const configureSharedStuff = [&](auto& getConfigValue, YALF::FormattedStringSink& sink) {
        if (auto default_log_level = getConfigValue("LogLevel"))
            if (auto default_log_level_enum_maybe = YALF::parseLogLevelString(default_log_level.value()))
                sink.setDefaultLogLevel(default_log_level_enum_maybe.value());

        if (auto default_format_maybe = getConfigValue("Format"))
            sink.setFormat(default_format_maybe.value());

        for (auto const level : YALF::getLogLevelList())
            if (auto const v = getConfigValue(std::string{ getLogLevelString(level) } + "Format"))
                sink.setFormat(level, v.value());
    };

    auto const configureDomainLogLevels = [&](ACFP::Section const& dll_section, YALF::Sink& sink) {
        dll_section.iterate([&](std::string_view domain, std::string_view level_str) {
            if (auto const level_maybe = YALF::parseLogLevelString(level_str))
                sink.setDomainLogLevel(domain, level_maybe.value());
        });
    };

    auto logger = std::make_unique<YALF::Logger>();
    auto addSink = [&](std::string_view name, std::unique_ptr<YALF::Sink> sink, bool defer) {
        if (defer) {
            auto deferred_sink = std::make_unique<YALF::DeferredSink>(std::move(sink));
            logger->addSink(std::string{ name }, std::move(deferred_sink));
        }
        else {
            logger->addSink(std::string{ name }, std::move(sink));
        }
    };

    { // Configure Console sink
        auto getConfigValue = getConfigValueMaker(config_group, "ConsoleSink");
        auto const enabled = ACFP::parse<bool>(getConfigValue("Enabled")).value_or(true);
        if (enabled) {
            auto console_sink = YALF::makeConsoleSink();

            configureSharedStuff(getConfigValue, *console_sink);
            configureDomainLogLevels(dll_config_group["ConsoleSink"], *console_sink);

            addSink("ConsoleSink", std::move(console_sink), ACFP::parse<bool>(getConfigValue("Deferred")).value_or(false));
        }
    }
    { // Configure File sink
        auto getConfigValue = getConfigValueMaker(config_group, "FileSink");
        auto const enabled = ACFP::parse<bool>(getConfigValue("Enabled")).value_or(false);
        if (enabled) {
            auto const log_filename_template = getConfigValue("FilenameTemplate").value_or("Logs/TSW_{0:%Y.%m.%d_%H.%M.%S}.txt");
            auto const now = std::chrono::system_clock::now();
            auto const log_filename = std::vformat(log_filename_template, std::make_format_args(now));
            auto file_sink = YALF::makeFileSink(log_filename);

            configureSharedStuff(getConfigValue, *file_sink);
            configureDomainLogLevels(dll_config_group["FileSink"], *file_sink);

            addSink("FileSink", std::move(file_sink), ACFP::parse<bool>(getConfigValue("Deferred")).value_or(true));
        }
    }
    #if 0
    { // Configure PB File sink
        auto getConfigValue = getConfigValueMaker(config_group, "PbFileSink");
        auto const enabled = ACFP::parse<bool>(getConfigValue("Enabled")).value_or(false);
        if (enabled) {
            auto const log_filename_template = getConfigValue("FilenameTemplate").value_or("Logs/TSW_{0:%Y.%m.%d_%H.%M.%S}.pbin");
            auto const now = std::chrono::system_clock::now();
            auto const log_filename = std::vformat(log_filename_template, std::make_format_args(now));
            auto pbfile_sink = YALF::makePbFileSink(log_filename);

            if (auto default_log_level = getConfigValue("LogLevel"))
                if (auto default_log_level_enum = YALF::parseLogLevelString(default_log_level.value()))
                    pbfile_sink->setDefaultLogLevel(default_log_level_enum.value());

            configureDomainLogLevels(dll_config_group["PbFileSink"], *pbfile_sink);

            addSink("PbFileSink", std::move(pbfile_sink), ACFP::parse<bool>(getConfigValue("Deferred")).value_or(true));
        }
    }
    #endif

    YALF::setGlobalLogger(std::move(logger));
}
