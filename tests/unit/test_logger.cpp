#include <gtest/gtest.h>
#include "../../src/utils/logger.h"

namespace splitter::testing {

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        temp_log_file_ = "/tmp/test_splitter.log";
    }
    
    void TearDown() override {
        utils::Logger::shutdown();
        std::remove(temp_log_file_.c_str());
    }
    
    std::string temp_log_file_;
};

TEST_F(LoggerTest, InitializeLogger) {
    EXPECT_TRUE(utils::Logger::init(temp_log_file_));
    EXPECT_TRUE(utils::Logger::is_initialized());
}

TEST_F(LoggerTest, LogLevels) {
    ASSERT_TRUE(utils::Logger::init(temp_log_file_, "debug"));
    
    utils::Logger::debug("Debug message");
    utils::Logger::info("Info message");
    utils::Logger::warning("Warning message");
    utils::Logger::error("Error message");
    
    EXPECT_EQ(utils::Logger::get_level(), "debug");
}

TEST_F(LoggerTest, SetLogLevel) {
    ASSERT_TRUE(utils::Logger::init(temp_log_file_));
    
    utils::Logger::set_level("warning");
    EXPECT_EQ(utils::Logger::get_level(), "warning");
}

} // namespace splitter::testing