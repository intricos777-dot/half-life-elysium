#pragma once
#include <cstdint>
#include <string>
#include <memory>

namespace te {

struct RuntimeConfig {
  std::string runtime_mode;
  std::string satalight_endpoint;
  std::string local_render_backend;
};

class IRuntime {
 public:
  virtual ~IRuntime() = default;
  virtual bool initialize(const RuntimeConfig& config) = 0;
  virtual void tick(uint64_t delta_ms) = 0;
  virtual void shutdown() = 0;
};

std::unique_ptr<IRuntime> CreateRuntime(const RuntimeConfig& config);

}  // namespace te
