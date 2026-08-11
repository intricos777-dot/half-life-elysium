#include "distributor.h"

namespace te {

class SatalightAssetDistributor final : public IAssetDistributor {
 public:
  explicit SatalightAssetDistributor(std::string) {}
  bool distribute(const std::vector<AssetChunk>&) override { return true; }
};

std::unique_ptr<IAssetDistributor> CreateSatalightAssetDistributor(const std::string&) {
  return std::make_unique<SatalightAssetDistributor>("");
}

}  // namespace te
