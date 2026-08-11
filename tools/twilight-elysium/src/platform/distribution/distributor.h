#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace te {

struct AssetChunk {
  std::string path;
  std::string stage_id;
  uint64_t offset = 0;
  uint32_t size = 0;
  uint32_t node_hint = 0;
};

class IAssetDistributor {
 public:
  virtual ~IAssetDistributor() = default;
  virtual bool distribute(const std::vector<AssetChunk>& chunks) = 0;
};

std::unique_ptr<IAssetDistributor> CreateSatalightAssetDistributor(const std::string& edge_api);

}  // namespace te
