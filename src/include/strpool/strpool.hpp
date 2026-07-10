#ifndef STRPOOL_HPP_
#define STRPOOL_HPP_
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace Anvil {

class StrPool {
public:
  enum class Index : uint32_t {};
  using OptString = std::optional<Index>;
  StrPool() = default;

  Index InternOrGet(std::string_view str);
  std::optional<Index> Get(std::string_view str);

private:
  std::unordered_map<std::string_view, Index> pooled_; // Extern. Owned Data
  uint32_t tracked_ = 0;
};

} // namespace Anvil

#endif
