#include "c3/theta/channel/wrapper/tcp.hpp"

#include <random>

/*
class flags_t {
public:
  // TODO: implement ECN
  bool ns = false;
  bool cwr = false;
  bool ece = false;

  bool urg = false;
  bool ack = false;
  bool psh = false;
  bool syn = false;
  bool fin = false;
};

tcp_port_t src_port;
tcp_port_t dst_port;
uint32_t seq_id;
uint32_t ack_id;
nu::bit_datum<4> data_offset;
nu::bit_datum<3> reserved = 0;
flags_t flags;
uint16_t window_size;
uint16_t checksum;
uint16_t urgent_ptr;
*/

namespace c3::theta::wrapper {
  constexpr nu::timeout_t connect_timeout = std::chrono::seconds(10);
  constexpr int retries = 5;
  constexpr float rtt_ema_weight = 0.2;

  template<typename BaseAddr>
  class tcp_common {
  public:
    std::shared_ptr<nu::concurrent_queue<nu::data>> bin;
    tcp_port_t local_port;
    tcp_header base_header;
    tcp_ep<BaseAddr> remote;
    std::atomic<decltype(tcp_header::base_t::seq_id)> current_seq_id;
    // Exponential moving average of round trip time
    nu::timeout_t rtt_ema = connect_timeout;

  public:
    tcp_common(decltype(bin) bin, decltype(local_port) local_port, decltype(remote) remote) :
      bin{bin}, local_port{local_port}, remote{remote} {
      thread_local std::default_random_engine rng_engine{std::random_device()()};
      thread_local std::uniform_int_distribution<decltype(tcp_header::base_t::seq_id)> rng_dist;

      current_seq_id = rng_dist(rng_engine);

      base_header.src_port = local_port;
      base_header.dst_port = remote.port;
    }

  public:
    std::pair<tcp_header, nu::data> get_packet() {
      for (int i = 0; i < retries; ++i) {
        if (auto buf = bin->pop().try_take(rtt_ema).value()) {
          if (buf.size() < nu::serialised_size<tcp_header::base_t>())
            continue;
          nu::data_const_ref header_buf = { buf.data(), nu::serialised_size<tcp_header::base_t>() };
          tcp_header_t header = nu::deserialise<tcp_header_t>(header_buf);
          header
        }
      }
    }
  };

  // TODO: remove
  constexpr size_t MaxFrameSize = 512;
  using BaseAddr = int;
  //template<size_t MaxFrameSize, typename BaseAddr>
  template<>
  class tcp_host<MaxFrameSize, BaseAddr>::client :
      public tcp_client<BaseAddr>,
      public tcp_common<BaseAddr> {
  public:
    tcp_host* parent;

  public:
    tcp_ep<BaseAddr> get_local_ep() override {
      return { parent->_base->get_ep(), local_port };
    }
    tcp_ep<BaseAddr> get_remote_ep() override {
      return remote;
    }

  public:
    void send(nu::data_const_ref b) override;

    nu::cancellable<size_t> receive(nu::data_ref b) override = 0;

  public:
    client(tcp_host* _parent, tcp_port_t local_port, tcp_ep<BaseAddr> remote) :
        tcp_common{(*_parent->_received.get_rw())[local_port], local_port, remote}, parent{_parent} {
      // Reserve bin
      parent->_received.get_rw()->emplace(local_port);

      // Send syn
      {
        tcp_header_t header = base_header;
        header.flags.syn = true;
        header.seq_id = current_seq_id++;

        parent->_base->send(remote.addr, nu::serialise(header));
      }

      // Get syn-ack
      {
        tcp_header_t header = nu::deserialise<tcp_header_t>(bin->pop().try_get_final(connect_timeout));
        if (!header.)
      }
    }
  };
}
