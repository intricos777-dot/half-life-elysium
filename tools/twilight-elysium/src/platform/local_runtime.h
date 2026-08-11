#pragma once
#include "runtime.h"

namespace te {

std::unique_ptr<IRuntime> CreateLocalRuntime(const RuntimeConfig& config);

}  // namespace te
