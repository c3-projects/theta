#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/utils/ports.hpp"

#include <c3/nu/data/helpers.hpp>

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

  class tcp_header : public nu::serialisable<tcp_header> {
  public:
    class flags_t {
    public:
      // TODO: implement ECN
      bool ns = false;
      bool cwr = false;
      bool ece = false;

      bool urg = false;
      bool ack = false;
      bool psh = false;
      bool rst = false;
      bool syn = false;
      bool fin = false;
    };

    class base_t : public nu::static_serialisable<base_t> {
    public:
      tcp_port_t src_port;
      tcp_port_t dst_port;
      uint32_t seq_id;
      uint32_t ack_id = 0;
      nu::bit_datum<4> data_offset = 5;
      nu::bit_datum<3> reserved = 0;
      flags_t flags;
      uint16_t window_size = 0;
      uint16_t checksum = 0;
      uint16_t urgent_ptr = 0;

    public:
      void _serialise_static(nu::data_ref) const override;
      C3_NU_DEFINE_STATIC_DESERIALISE(base_t, 20, _);
    };

    class options_t : public nu::serialisable<options_t> {
    public:
      std::optional<uint16_t> max_seg_size = std::nullopt;
      std::optional<int8_t> window_scale = std::nullopt;
      std::optional<void> sack_permitted = std::nullopt;
      std::optional<std::vector<std::pair<uint32_t, uint32_t>>> sack = std::nullopt;
      std::optional<std::pair<uint32_t, uint32_t>> timestamp = std::nullopt;

    public:
      nu::data _serialise() const override;
      C3_NU_DEFINE_DESERIALISE(base_t, _);
    };

  public:
    base_t base;
    options_t options;

  public:
    nu::data _serialise() const override;
    C3_NU_DEFINE_DESERIALISE(tcp_header, _);
  };
}

#include <c3/nu/data/clean_helpers.hpp>
