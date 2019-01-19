#pragma once

#include "c3/theta/channel/base.hpp"

#include <c3/nu/concurrency/concurrent_queue.hpp>

#include <cstdint>

#include <array>
#include <memory>

namespace c3::theta {
  template<nu::n_bits_rep_t DoF>
  std::pair<std::unique_ptr<medium<DoF>>, std::unique_ptr<medium<DoF>>> fake_medium() {
    using msg_t = nu::bit_datum<DoF>;

    auto _0 = std::make_shared<nu::concurrent_queue<msg_t>>();
    auto _1 = std::make_shared<nu::concurrent_queue<msg_t>>();

    class tmp_medium : public medium<DoF> {
    private:
      decltype(_0) inbox;
      decltype(_1) outbox;

    public:
      void transmit_points(gsl::span<const nu::bit_datum<DoF>> in) override {
        for (auto i : in)
          outbox->push(i);
      }

      void receive_points(gsl::span<nu::bit_datum<DoF>> out) override {
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

  // Again, either this or PImpl
  std::unique_ptr<channel> tcp_ipv4(ipv4_ep);
  std::unique_ptr<channel> tcp_ipv6(ipv6_ep);

  std::unique_ptr<link<512>> udp_ipv4(ipv4_ep);
  std::unique_ptr<link<512>> udp_ipv6(ipv6_ep);
}
