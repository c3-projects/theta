#pragma once

#include <c3/nu/concurrency/cancellable.hpp>

#include "c3/theta/channel/tcp.hpp"

namespace c3::theta::ip {
  using ipv4_address = std::array<uint8_t, 4>;
  using ipv6_address = std::array<uint8_t, 16>;

  constexpr ipv4_address IPV4_ANY = { 0, 0, 0, 0 };
  constexpr ipv6_address IPV6_ANY = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  constexpr ipv4_address IPV4_LOOPBACK = { 127, 0, 0, 1 };
  constexpr ipv6_address IPV6_LOOPBACK = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  template<typename BaseAddr>
  class tcp_host : public theta::tcp_host<BaseAddr> {
    BaseAddr local_addr;

  public:
    nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> connect(tcp_ep<BaseAddr> remote) override;
    // TODO: auto bind to local_ep to force iface
    std::unique_ptr<tcp_server<BaseAddr>> listen(tcp_port_t) override;

  public:
    inline tcp_host(BaseAddr addr) : local_addr{addr} {}
  };

  template class tcp_host<ipv4_address>;
  template class tcp_host<ipv6_address>;
}
