#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <set>

namespace c3::theta {
  template<typename Address>
  class scanner {
  public:
    virtual nu::cancellable<std::set<Address>> scan() = 0;
  };
}
