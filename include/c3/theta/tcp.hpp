#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/server.hpp"
#include "c3/theta/channel/ports.hpp"

#include <c3/nu/data/helpers.hpp>

namespace c3::theta {
  using tcp_port_t = uint16_t;

  template<typename BaseAddr>
  struct _tcp_ep_t { using type = ep_t<BaseAddr, tcp_port_t>; };

  template<typename BaseAddr>
  using tcp_ep_t = typename _tcp_ep_t<BaseAddr>::type;

  constexpr tcp_port_t TCP_PORT_ANY = 0;

  template<typename BaseAddr>
  class tcp_client : public stream_channel, public client<BaseAddr> {};

  template<typename BaseAddr>
  using tcp_server_t = server<tcp_client<BaseAddr>, BaseAddr>;

  template<typename BaseAddr>
  class tcp_host : public host<tcp_client<BaseAddr>, BaseAddr> {};

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
      bool sack_permitted = false;
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
