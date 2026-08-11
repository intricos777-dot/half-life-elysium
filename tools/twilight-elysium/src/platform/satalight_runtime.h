#pragma once
#include "runtime.h"

namespace te {

std::unique_ptr<IRuntime> CreateSatalightRuntime(const RuntimeConfig& config);

}  // namespace te
