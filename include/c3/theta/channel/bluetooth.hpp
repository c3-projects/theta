#pragma once

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/ports.hpp"
#include "c3/theta/channel/server.hpp"
#include "c3/theta/channel/scanner.hpp"

#include <ios>
#include <iomanip>

namespace c3::theta::bluetooth {
  using address = nu::static_data<6>;

  constexpr address any   {0, 0, 0, 0, 0, 0};
  constexpr address all   {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  constexpr address local {0, 0, 0, 0xff, 0xff, 0xff};

  using rfcomm_port_t = uint8_t;

  using rfcomm_ep_t = ep_t<bluetooth::address, rfcomm_port_t>;

  class rfcomm_client : public client<bluetooth::address, rfcomm_port_t>, public stream_channel {};

  using rfcomm_server_t = server<rfcomm_client, bluetooth::address, rfcomm_port_t>;

  class rfcomm_host : public host<rfcomm_client, bluetooth::address, rfcomm_port_t> {
  private:
    bluetooth::address local_addr;

  public:
    bluetooth::address local_address() const override { return local_addr; }

    nu::cancellable<std::unique_ptr<rfcomm_client>> connect(rfcomm_ep_t remote) override;
    std::unique_ptr<rfcomm_server_t> listen(rfcomm_port_t) override;

  public:
    rfcomm_host();
    rfcomm_host(address);
  };

  class l2cap_port_t {
  public:
    uint16_t psm;
    uint16_t cid = 2;

  public:
    constexpr l2cap_port_t(uint16_t psm = 0, uint16_t cid = 2) : psm{psm}, cid{cid} {}
  };

  constexpr bool operator==(const l2cap_port_t& a, const l2cap_port_t& b) {
    return a.psm == b.psm && a.cid == b.cid;
  }
  constexpr bool operator!=(const l2cap_port_t& a, const l2cap_port_t& b) {
    return !(a == b);
  }
  constexpr bool operator<(const l2cap_port_t& a, const l2cap_port_t& b) {
    if (a.psm != b.psm)
      return a.psm < b.psm;
    else
      return a.cid < b.cid;
  }
  constexpr bool operator>(const l2cap_port_t& a, const l2cap_port_t& b) {
    if (a.psm != b.psm)
      return a.psm > b.psm;
    else
      return a.cid > b.cid;
  }
  constexpr bool operator<=(const l2cap_port_t& a, const l2cap_port_t& b) {
    return !(a > b);
  }
  constexpr bool operator>=(const l2cap_port_t& a, const l2cap_port_t& b) {
    return !(a < b);
  }

  using l2cap_ep_t = ep_t<bluetooth::address, l2cap_port_t>;

  class l2cap_client : public client<bluetooth::address, l2cap_port_t>, public stream_channel {};

  using l2cap_server_t = server<l2cap_client, bluetooth::address, l2cap_port_t>;

  class l2cap_host : public host<l2cap_client, bluetooth::address, l2cap_port_t> {
  private:
    bluetooth::address local_addr;

  public:
    bluetooth::address local_address() const override { return local_addr; }

    nu::cancellable<std::unique_ptr<l2cap_client>> connect(l2cap_ep_t remote) override;
    std::unique_ptr<l2cap_server_t> listen(l2cap_port_t) override;

  public:
    l2cap_host();
    l2cap_host(address);
  };

  std::unique_ptr<scanner<bluetooth::address>> get_scanner();

  // TODO: address stream write
}
