#pragma once

#include <c3/nu/concurrency/cancellable.hpp>

#include "c3/theta/proto/tcp.hpp"

namespace c3::theta::channels::ip {
  using address_v4 = std::array<uint8_t, 4>;
  using address_v6 = std::array<uint8_t, 16>;

  constexpr address_v4 any_v4 = { 0, 0, 0, 0 };
  constexpr address_v6 any_v6 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  constexpr address_v4 loopback_v4 = { 127, 0, 0, 1 };
  constexpr address_v6 loopback_v6 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  using v4_tcp_ep_t = ep_t<address_v4>;
  using v6_tcp_ep_t = ep_t<address_v6>;

  template<typename BaseAddr>
  class tcp_host : public proto::tcp::host<BaseAddr> {
  private:
    BaseAddr local_addr;

  public:
    BaseAddr local_address() const override { return local_addr; }

    nu::cancellable<std::unique_ptr<proto::tcp::client<BaseAddr>>> connect(proto::tcp::ep_t<BaseAddr> remote) override;

    std::unique_ptr<proto::tcp::server_t<BaseAddr>> listen(proto::tcp::port_t) override;

  public:
    inline tcp_host(BaseAddr addr) : local_addr{addr} {}
  };

  template class tcp_host<address_v4>;
  template class tcp_host<address_v6>;

  template<typename BaseAddr>
  class udp_host : public proto::tcp::host<BaseAddr> {
  private:
    BaseAddr local_addr;

  public:
    BaseAddr local_address() const override { return local_addr; }

    nu::cancellable<std::unique_ptr<proto::tcp::client<BaseAddr>>> connect(proto::tcp::ep_t<BaseAddr> remote) override;

    std::unique_ptr<proto::tcp::server_t<BaseAddr>> listen(proto::tcp::port_t) override;

  public:
    inline udp_host(BaseAddr addr) : local_addr{addr} {}
  };
}
