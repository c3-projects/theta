#pragma once

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrappers {
  std::unique_ptr<channel> medium2pseudochannel(std::unique_ptr<medium>&&);
  std::unique_ptr<channel> pseudochannel2channel(std::unique_ptr<medium>&&);
  std::unique_ptr<channel> medium2channel(std::unique_ptr<medium>&&);
}
