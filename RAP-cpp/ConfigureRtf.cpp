#include <RTF/RTF.h>
#include <YALF/YALF.h>
#include <ACFP/ACFP.h>

using namespace std::literals::string_view_literals;

class LogConsoleInterposer : public RTF::IFluentRegisterTargetInterposer
{
public:
    explicit LogConsoleInterposer(std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next = nullptr)
        : RTF::IFluentRegisterTargetInterposer()
        , next(std::move(next))
    {}

    virtual void seq(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        LOG_INFO_I("FluentRegisterTarget", target_instance, "\033[36mSeq: {}\033[0m", msg);
        if (this->next)
            this->next->seq(target_domain, target_instance, msg);
    }
    virtual void step(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        LOG_INFO_I("FluentRegisterTarget", target_instance, "\033[96m  Step: {}\033[0m", msg);
        if (this->next)
            this->next->step(target_domain, target_instance, msg);
    }
    virtual void opStart(std::string_view target_domain, std::string_view target_instance, std::string_view op_msg) override
    {
        LOG_DEBUG_I("FluentRegisterTarget", target_instance, "\033[32m    Op: {}\033[0m", op_msg);
        if (this->next)
            this->next->opStart(target_domain, target_instance, op_msg);
    }
    virtual void opExtra(std::string_view target_domain, std::string_view target_instance, std::string_view values) override
    {
        LOG_DEBUG_I("FluentRegisterTarget", target_instance, "\033[33m      {}\033[0m", values);
        if (this->next)
            this->next->opExtra(target_domain, target_instance, values);
    }
    virtual void opEnd(std::string_view target_domain, std::string_view target_instance) override
    {
        LOG_DEBUG_I("FluentRegisterTarget", target_instance, "\033[92m      <\033[0m");
        if (this->next)
            this->next->opEnd(target_domain, target_instance);
    }
    virtual void opError(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        LOG_ERROR_I("FluentRegisterTarget", target_instance, "\033[31m      Error: {}\033[0m", msg);
        if (this->next)
            this->next->opError(target_domain, target_instance, msg);
    }

private:
    std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next;
};

static inline
std::unique_ptr<RTF::IFluentRegisterTargetInterposer> makeLogConsoleInterposer(std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next = nullptr)
{
    return std::make_unique<LogConsoleInterposer>(std::move(next));
}

class LogFileInterposer : public RTF::IFluentRegisterTargetInterposer
{
public:
    explicit LogFileInterposer(std::filesystem::path filename, std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next)
        : RTF::IFluentRegisterTargetInterposer()
        , next(std::move(next))
        , os(filename, std::ios::app | std::ios::binary)
        , osi(this->os)
    {}

    virtual void seq(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        std::format_to(this->osi, "{}[{}]  Seq: {}\n", target_domain, target_instance, msg);
        if (this->next)
            this->next->seq(target_domain, target_instance, msg);
    }
    virtual void step(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        std::format_to(this->osi, "{}[{}]      Step: {}\n", target_domain, target_instance, msg);
        if (this->next)
            this->next->step(target_domain, target_instance, msg);
    }
    virtual void opStart(std::string_view target_domain, std::string_view target_instance, std::string_view op_msg) override
    {
        std::format_to(this->osi, "{}[{}]          Op: {}\n", target_domain, target_instance, op_msg);
        if (this->next)
            this->next->opStart(target_domain, target_instance, op_msg);
    }
    virtual void opExtra(std::string_view target_domain, std::string_view target_instance, std::string_view values) override
    {
        std::format_to(this->osi, "{}[{}]            {}\n", target_domain, target_instance, values);
        if (this->next)
            this->next->opExtra(target_domain, target_instance, values);
    }
    virtual void opEnd(std::string_view target_domain, std::string_view target_instance) override
    {
        if (this->next)
            this->next->opEnd(target_domain, target_instance);
    }
    virtual void opError(std::string_view target_domain, std::string_view target_instance, std::string_view msg) override
    {
        std::format_to(this->osi, "{}[{}]            Error: {}\n", target_domain, target_instance, msg);
        if (this->next)
            this->next->opError(target_domain, target_instance, msg);
    }

private:
    std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next;
    std::ofstream os;
    std::ostream_iterator<char> osi;
};

static inline
std::unique_ptr<RTF::IFluentRegisterTargetInterposer> makeLogFileInterposer(std::filesystem::path filename, std::unique_ptr<RTF::IFluentRegisterTargetInterposer> next = nullptr)
{
    return std::make_unique<LogFileInterposer>(filename, std::move(next));
}

void configureRtf(ACFP::Section const& config)
{
    LOG_INFO("Main", "Configuring FluentRegisterTarget global interposer");

    std::unique_ptr<RTF::IFluentRegisterTargetInterposer> interposer = makeLogConsoleInterposer();

    if (config["Enabled"].value_or("false") == "true"sv) {
        auto const regoplog_filename_template = config["FilenameTemplate"].value_or("Logs/DfeOperations_{0:%Y.%m.%d_%H.%M.%S}.txt");
        auto const now = std::chrono::system_clock::now();
        auto const filename = std::vformat(regoplog_filename_template, std::make_format_args(now));
        interposer = makeLogFileInterposer(filename, std::move(interposer));
    }

    RTF::IFluentRegisterTargetInterposer::setDefault(std::move(interposer));
}
