#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/base.hpp"
#include "c3/theta/ports.hpp"

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::proto::udp {
  using port_t = uint16_t;

  template<typename BaseAddr>
  using ep_t = ep_t<BaseAddr, port_t>;

}
