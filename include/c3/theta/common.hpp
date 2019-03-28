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
      void send(gsl::span<const nu::bit_datum<DoF>> in) override {
        for (auto i : in)
          outbox->push(i);
      }

      void receive(gsl::span<nu::bit_datum<DoF>> out) override {
        for (auto& i : out) {
          if (auto p = inbox->pop().get_or_cancel(poll_timeout));
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
}
