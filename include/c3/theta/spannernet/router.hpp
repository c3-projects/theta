#pragma once

#include "c3/theta/spannernet/common.hpp"

#include "c3/theta/channel/base.hpp"

namespace c3::theta::spannernet {
  template<typename Address>
  class routing_table {
  public:
    virtual std::vector<Address> get_best_route(user::ref target) = 0;
  };

  template<typename Address>
  class router {
  private:
    std::unique_ptr<serv

  public:
    virtual void forward()
  };
}
