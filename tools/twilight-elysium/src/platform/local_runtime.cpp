#include "local_runtime.h"
#include "runtime.h"

namespace te {

class LocalRuntime final : public IRuntime {
 public:
  bool initialize(const RuntimeConfig& config) override;
  void tick(uint64_t delta_ms) override;
  void shutdown() override;
};

std::unique_ptr<IRuntime> CreateLocalRuntime(const RuntimeConfig& config) {
  return std::make_unique<LocalRuntime>();
}

bool LocalRuntime::initialize(const RuntimeConfig&) { return true; }
void LocalRuntime::tick(uint64_t) {}
void LocalRuntime::shutdown() {}

}  // namespace te
