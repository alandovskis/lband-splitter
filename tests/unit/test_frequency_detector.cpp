#include <gtest/gtest.h>
#include "../../src/hardware/frequency_detector.h"

namespace splitter::testing {

class FrequencyDetectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        detector_ = std::make_unique<hardware::FrequencyDetector>();
    }
    
    void TearDown() override {
        if (detector_) {
            detector_->cleanup();
        }
        detector_.reset();
    }
    
    std::unique_ptr<hardware::FrequencyDetector> detector_;
};

TEST_F(FrequencyDetectorTest, DefaultState) {
    EXPECT_EQ(detector_->get_frequency(0), 0.0);
    EXPECT_EQ(detector_->get_signal_level(0), -100.0);
    EXPECT_FALSE(detector_->is_healthy());
}

TEST_F(FrequencyDetectorTest, Constants) {
    EXPECT_GT(hardware::FrequencyDetector::MAX_FREQUENCY_MHZ, 
              hardware::FrequencyDetector::MIN_FREQUENCY_MHZ);
    EXPECT_EQ(hardware::FrequencyDetector::NUM_PORTS, 32);
}

TEST_F(FrequencyDetectorTest, GetAllReadings) {
    auto readings = detector_->get_all_readings();
    EXPECT_EQ(readings.size(), hardware::FrequencyDetector::NUM_PORTS);
}

TEST_F(FrequencyDetectorTest, InvalidPortId) {
    EXPECT_EQ(detector_->get_frequency(-1), 0.0);
    EXPECT_EQ(detector_->get_frequency(32), 0.0);
    EXPECT_EQ(detector_->measure_frequency(100), 0.0);
}

} // namespace splitter::testing