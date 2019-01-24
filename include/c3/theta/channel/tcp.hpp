#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/utils/ports.hpp"

namespace c3::theta {
  using tcp_port_t = uint16_t;
  constexpr tcp_port_t TCP_PORT_ANY = 0;

  template<typename BaseAddr>
  using tcp_ep = ep_t<BaseAddr, tcp_port_t>;

  template<typename BaseEp>
  inline bool operator==(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  template<typename BaseEp>
  inline bool operator!=(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    return !(a == b);
  }
  template<typename BaseEp>
  inline bool operator<(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    if (a.port != b.port)
      return a.port < b.port;
    else
      return a.addr < b.addr;
  }
  template<typename BaseEp>
  inline bool operator>(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    if (a.port != b.port)
      return a.port > b.port;
    else
      return a.addr > b.addr;
  }
  template<typename BaseEp>
  inline bool operator<=(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    return !(a > b);
  }
  template<typename BaseEp>
  inline bool operator>=(const tcp_ep<BaseEp>& a, const tcp_ep<BaseEp>& b) {
    return !(a < b);
  }

  template<typename BaseEp>
  class tcp_server;

  template<typename BaseAddr>
  class tcp_client : public stream_channel {
    friend class tcp_server<BaseAddr>;
  public:
    virtual tcp_ep<BaseAddr> get_local_ep() = 0;
    virtual tcp_ep<BaseAddr> get_remote_ep() = 0;

  public:
    virtual void send(nu::data_const_ref) override = 0;

    virtual nu::cancellable<size_t> receive(nu::data_ref b) override = 0;

  public:
    virtual ~tcp_client() = default;
  };

  template<typename BaseAddr>
  class tcp_server {
  public:
    virtual nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> accept() = 0;

    virtual tcp_ep<BaseAddr> get_ep() = 0;

  public:
    virtual ~tcp_server() = default;
  };

  template<typename BaseAddr>
  class tcp_host {
  public:
    virtual nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> connect(tcp_ep<BaseAddr> remote) = 0;
    virtual std::unique_ptr<tcp_server<BaseAddr>> listen(tcp_port_t local) = 0;

  public:
    virtual ~tcp_host() = default;
  };
}
