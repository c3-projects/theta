#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/base.hpp"
#include "c3/theta/server.hpp"
#include "c3/theta/ports.hpp"

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::proto::tcp {
  using port_t = uint16_t;

  template<typename BaseAddr>
  using ep_t = ep_t<BaseAddr, port_t>;

  constexpr port_t PORT_ANY = 0;

  template<typename BaseAddr>
  class client : public stream_channel, public theta::client<BaseAddr> {};

  template<typename BaseAddr>
  using server_t = theta::server<tcp::client<BaseAddr>, BaseAddr>;

  template<typename BaseAddr>
  class host : public theta::host<tcp::client<BaseAddr>, BaseAddr> {};

  class header : public nu::serialisable<tcp::header> {
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
      tcp::port_t src_port;
      tcp::port_t dst_port;
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
    C3_NU_DEFINE_DESERIALISE(tcp::header, _);
  };
}

#include <c3/nu/data/clean_helpers.hpp>
