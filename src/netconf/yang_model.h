#pragma once

#include <memory>
#include <string>
#include <vector>

struct ly_ctx;
struct lys_module;

namespace splitter::netconf {

class YangModel {
public:
  YangModel();
  ~YangModel();

  bool load_default_models();
  bool load_models(const std::vector<std::string> &model_paths);
  bool load_model_from_string(const std::string &yang_content);

  bool validate_config(const std::string &config_xml);
  bool validate_data(const std::string &data_xml);

  std::string get_last_error() const { return last_error_; }
  bool is_loaded() const { return context_ != nullptr; }

private:
  bool create_context();
  void cleanup_context();
  bool load_ietf_models();
  bool load_splitter_model();

  std::string get_default_splitter_yang();
  std::vector<std::string> get_yang_search_paths();

  struct ly_ctx *context_{nullptr};
  const struct lys_module *splitter_module_{nullptr};

  mutable std::string last_error_;
};

} // namespace splitter::netconf