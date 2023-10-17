#include "ShitUtility.hpp"
namespace Shit {
std::vector<std::string_view> split(std::string_view str,
                                    std::string_view delim) {
  std::vector<std::string_view> ret;
  size_t i = 0, p = 0, s = delim.size();
  while ((p = str.find(delim, i)) != std::string_view::npos) {
    ret.emplace_back(str.begin() + i, str.begin() + p);
    i = p + s;
  }
  if (i < str.size()) ret.emplace_back(str.begin() + i, str.end());
  return ret;
}
}  // namespace Shit
