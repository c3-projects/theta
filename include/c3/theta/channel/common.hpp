#pragma once

#include "c3/theta/channel/base.hpp"

#include <cstdint>

#include <array>
#include <memory>

namespace c3::theta {
   std::pair<std::unique_ptr<link>, std::unique_ptr<link>> fake_link();

  using ipv4_address = std::array<uint8_t, 4>;
  using ipv6_address = std::array<uint8_t, 16>;

  struct ipv4_ep { ipv4_address addr; uint16_t port; };
  struct ipv6_ep { ipv6_address addr; uint16_t port; };

  // Again, either this or PImpl
  std::unique_ptr<link> tcp_ipv4(ipv4_ep);
  std::unique_ptr<link> tcp_ipv6(ipv6_ep);

  std::unique_ptr<channel<512>> udp_ipv4(ipv4_ep);
  std::unique_ptr<channel<512>> udp_ipv6(ipv6_ep);
}
