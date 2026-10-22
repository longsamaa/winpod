#include "Log.hpp"
#include "platform/WindowsSdk.hpp"
#include <chrono>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
namespace winpod
{
namespace
{
class SkipLevelSink final : public spdlog::sinks::sink
{
public:
    SkipLevelSink(spdlog::sink_ptr inner, spdlog::level::level_enum skip) : m_Inner(std::move(inner)), m_Skip(skip) {}

    void log(const spdlog::details::log_msg& msg) override
    {
        if (msg.level != m_Skip)
        {
            m_Inner->log(msg);
        }
    }
    void flush() override { m_Inner->flush(); }
    void set_pattern(const std::string& pattern) override { m_Inner->set_pattern(pattern); }
    void set_formatter(std::unique_ptr<spdlog::formatter> formatter) override
    {
        m_Inner->set_formatter(std::move(formatter));
    }

private:
    spdlog::sink_ptr m_Inner;
    spdlog::level::level_enum m_Skip;
};
} // namespace

Log& Log::instance()
{
    static Log s_instance;
    return s_instance;
}

void Log::init(const std::string& appName)
{
    std::lock_guard lock(m_Mutex);
    assert(!appName.empty());
    m_AppName = appName;
    if (m_Initialized)
    {
        return;
    }
    constexpr auto kLevel = spdlog::level::trace;
    spdlog::sink_ptr file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(m_Path, true);
#ifdef NDEBUG
    file_sink = std::make_shared<SkipLevelSink>(file_sink, spdlog::level::debug);
#endif
    file_sink->set_level(kLevel);
    m_Sinks = {file_sink};
    const HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdOut != nullptr && stdOut != INVALID_HANDLE_VALUE)
    {
        spdlog::sink_ptr console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#ifdef NDEBUG
        console_sink = std::make_shared<SkipLevelSink>(console_sink, spdlog::level::debug);
#endif
        console_sink->set_level(kLevel);
        m_Sinks.push_back(console_sink);
    }

    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%-5l%$] [%n] [t%t] %v");
    spdlog::set_level(kLevel);
    spdlog::flush_on(spdlog::level::warn);
    spdlog::flush_every(std::chrono::seconds(3));

    m_Initialized = true;
    spdlog::set_default_logger(create(m_AppName));
}

void Log::shutdown()
{
    std::lock_guard lock(m_Mutex);
    if (!m_Initialized)
    {
        return;
    }
    spdlog::shutdown();
    m_Sinks.clear();
    m_Initialized = false;
}

std::shared_ptr<spdlog::logger> Log::get(std::string_view name)
{
    std::lock_guard lock(m_Mutex);
    const std::string key(name);
    if (auto existing = spdlog::get(key))
    {
        return existing;
    }
    return create(key);
}

std::shared_ptr<spdlog::logger> Log::getAppLogger()
{
    return get(m_AppName);
}

std::shared_ptr<spdlog::logger> Log::create(const std::string& name)
{
    auto logger = std::make_shared<spdlog::logger>(name, m_Sinks.begin(), m_Sinks.end());
    spdlog::initialize_logger(logger);
    return logger;
}
} // namespace winpod
