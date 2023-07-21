#include <gtest/gtest.h>

#include "Logger.hpp"

template<>
const char* LoggerType::LoggerRef = "LoggerRefXD";
std::unique_ptr<LoggerType> main_logger;
std::unique_ptr<bfc::IMonitor> main_monitor;

int main(int argc, char **argv)
{
    main_logger = std::make_unique<LoggerType>("test.log");
    main_logger->logful();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}