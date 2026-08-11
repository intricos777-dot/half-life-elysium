#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace te {

struct ShaderPass {
  std::string name;
  uint32_t target = 0;
};

class IShaderPipeline {
 public:
  virtual ~IShaderPipeline() = default;
  virtual bool compile(const std::string& source, uint32_t target) = 0;
  virtual uint32_t create_pass(const ShaderPass& pass) = 0;
};

}  // namespace te
