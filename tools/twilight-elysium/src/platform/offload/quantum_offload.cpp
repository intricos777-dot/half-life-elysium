#include "quantum_offload.h"

namespace te {

class QuantumOffload final : public IQuantumOffload {
 public:
  explicit QuantumOffload(std::string) {}
  bool submit(const OffloadJob&) override { return true; }
};

std::unique_ptr<IQuantumOffload> CreateQuantumOffload(const std::string&) {
  return std::make_unique<QuantumOffload>("");
}

}  // namespace te
