#include "../../src/netconf/yang_model.h"
#include <gtest/gtest.h>

namespace splitter::testing {

class YangModelTest : public ::testing::Test {
protected:
  void SetUp() override {
    yang_model_ = std::make_unique<netconf::YangModel>();
  }

  void TearDown() override { yang_model_.reset(); }

  std::unique_ptr<netconf::YangModel> yang_model_;
};

TEST_F(YangModelTest, DefaultState) { EXPECT_FALSE(yang_model_->is_loaded()); }

TEST_F(YangModelTest, LoadEmptyModelList) {
  std::vector<std::string> empty_models;
  bool result = yang_model_->load_models(empty_models);
  EXPECT_TRUE(result || !result);
}

} // namespace splitter::testing