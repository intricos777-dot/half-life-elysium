#include "satalight_runtime.h"
#include "runtime.h"

namespace te {

class SatalightRuntime final : public IRuntime {
 public:
  bool initialize(const RuntimeConfig& config) override;
  void tick(uint64_t delta_ms) override;
  void shutdown() override;
};

std::unique_ptr<IRuntime> CreateSatalightRuntime(const RuntimeConfig& config) {
  return std::make_unique<SatalightRuntime>();
}

bool SatalightRuntime::initialize(const RuntimeConfig& config) { return true; }
void SatalightRuntime::tick(uint64_t) {}
void SatalightRuntime::shutdown() {}

}  // namespace te
