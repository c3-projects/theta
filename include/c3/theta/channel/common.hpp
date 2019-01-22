#pragma once

#include "c3/theta/channel/base.hpp"

#include <c3/nu/concurrency/concurrent_queue.hpp>

#include <cstdint>

#include <array>
#include <memory>

namespace c3::theta {
  template<nu::n_bits_rep_t DoF>
  inline std::pair<std::unique_ptr<medium<DoF>>, std::unique_ptr<medium<DoF>>> fake_medium() {
    using msg_t = nu::bit_datum<DoF>;

    auto _0 = std::make_shared<nu::concurrent_queue<msg_t>>();
    auto _1 = std::make_shared<nu::concurrent_queue<msg_t>>();

    class tmp_medium : public medium<DoF> {
    private:
      decltype(_0) inbox;
      decltype(_1) outbox;

    public:
      inline void send_points(gsl::span<const nu::bit_datum<DoF>> in) override {
        for (auto i : in)
          outbox->push(i);
      }

      inline void receive_points(gsl::span<nu::bit_datum<DoF>> out) override {
        for (auto& i : out) {
          if (auto p = inbox->try_pop(nu::timeout_t::zero()))
            i = *p;
          else
            throw nu::timed_out{};
        }
      }

    public:
      tmp_medium(decltype(_0) inbox, decltype(_0) outbox) :
        inbox(inbox), outbox(outbox) {}
    };

    return { std::make_unique<tmp_medium>(_0, _1), std::make_unique<tmp_medium>(_1, _0) };
  }

  using ipv4_address = std::array<uint8_t, 4>;
  using ipv6_address = std::array<uint8_t, 16>;

  struct ipv4_ep { ipv4_address addr; uint16_t port; };
  struct ipv6_ep { ipv6_address addr; uint16_t port; };

  inline bool operator==(const ipv4_ep& a, const ipv4_ep& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  inline bool operator==(const ipv6_ep& a, const ipv6_ep& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  inline bool operator!=(const ipv4_ep& a, const ipv4_ep& b) {
    return !(a == b);
  }
  inline bool operator!=(const ipv6_ep& a, const ipv6_ep& b) {
    return !(a == b);
  }

  constexpr uint16_t PORT_ANY = 0;

  constexpr ipv4_address IPV4_ANY = { 0, 0, 0, 0 };
  constexpr ipv6_address IPV6_ANY = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  constexpr ipv4_address IPV4_LOOPBACK = { 127, 0, 0, 1 };
  constexpr ipv6_address IPV6_LOOPBACK = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  template<typename EpType>
  class tcp_server;

  template<typename EpType>
  class tcp_client : public stream_channel {
    friend class tcp_server<EpType>;
  private:
    void* impl;

  public:
    EpType get_local_ep();
    EpType get_remote_ep();

  public:
    void send_data(nu::data_const_ref) override;

    nu::cancellable<size_t> receive_data(nu::data_ref b) override;

  public:
    /// Unsafe construction from impl
    tcp_client(void* impl) : impl{impl} {}

  public:
    tcp_client() : impl{nullptr} {};
    ~tcp_client();

    tcp_client<EpType>& operator=(const tcp_client<EpType>&) = delete;
    tcp_client(const tcp_client<EpType>&) = delete;

    inline tcp_client<EpType>& operator=(tcp_client<EpType>&& other) {
      impl = other.impl;
      other.impl = nullptr;
      return *this;
    };
    inline tcp_client(tcp_client&& other) : impl{other.impl} {
      other.impl = nullptr;
    }

  public:
    static nu::cancellable<std::shared_ptr<tcp_client<EpType>>> connect(EpType remote);
  };

  template<typename EpType>
  class tcp_server {
  private:
    void* impl;

  public:
    nu::cancellable<std::shared_ptr<tcp_client<EpType>>> accept();

    bool bind(EpType);
    EpType get_ep();

  public:
    tcp_server();
    tcp_server(EpType);

    tcp_server<EpType>& operator=(const tcp_server<EpType>&) = delete;
    tcp_server(const tcp_server<EpType>&) = delete;

    inline tcp_server<EpType>& operator=(tcp_server<EpType>&& other) {
      impl = other.impl;
      other.impl = nullptr;
      return *this;
    };
    inline tcp_server(tcp_server&& other) : impl{other.impl} {
      other.impl = nullptr;
    }

    ~tcp_server();
  };

  template<typename EpType>
  std::function<std::unique_ptr<stream_channel>(EpType)> udp(EpType local_ep);
}
