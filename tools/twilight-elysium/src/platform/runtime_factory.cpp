#include "runtime.h"
#include "local_runtime.h"
#include "satalight_runtime.h"

namespace te {

std::unique_ptr<IRuntime> CreateRuntime(const RuntimeConfig& config) {
  if (config.runtime_mode == "satalight") {
    return CreateSatalightRuntime(config);
  }
  return CreateLocalRuntime(config);
}

}  // namespace te
