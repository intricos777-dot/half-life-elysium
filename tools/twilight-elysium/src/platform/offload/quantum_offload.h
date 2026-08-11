#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace te {

struct OffloadJob {
  std::string job_id;
  std::string node_id;
  uint32_t priority = 0;
};

class IQuantumOffload {
 public:
  virtual ~IQuantumOffload() = default;
  virtual bool submit(const OffloadJob& job) = 0;
};

std::unique_ptr<IQuantumOffload> CreateQuantumOffload(const std::string& edge_api);

}  // namespace te
