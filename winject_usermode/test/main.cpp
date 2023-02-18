#include <gtest/gtest.h>

#include "Logger.hpp"
const char* Logger::LoggerRef = "LoggerRefXD";
std::unique_ptr<Logger> main_logger;

int main(int argc, char **argv)
{
    main_logger = std::make_unique<Logger>("test.log");
    main_logger->setLevel(Logger::TRACE);
    main_logger->logful();

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}