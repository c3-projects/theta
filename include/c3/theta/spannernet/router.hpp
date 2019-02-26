#pragma once

#include "c3/theta/spannernet/common.hpp"

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/server.hpp"

#include <c3/nu/concurrency/mutexed.hpp>

#include <c3/upsilon/csprng.hpp>

namespace c3::theta::spannernet {
  template<typename Address>
  class routing_table {
  public:
    virtual std::optional<Address> get_best_route(user::ref target) = 0;
    virtual void add_route(const Address& addr, user::ref usr) = 0;
  };

  template<typename Address>
  class simple_routing_table final : public routing_table<Address> {
  private:
    nu::worm_mutexed<std::map<user::ref, Address>> _table;

  public:
    inline Address get_best_route(user::ref target) override {
      auto handle = _table.get_ro();
      if (auto iter = handle->find(target); iter != handle->end())
        return *iter;
      else return std::nullopt;
    }

    inline void add_route(const Address& addr, user::ref usr) override {
      auto handle = _table.get_rw();
      handle->insert_or_assign(addr, usr);
    }
  };

  // Bundle for packet
  template<typename Address>
  class router_channel {
  public:
    using ping_counter_t = uint64_t;

  private:
    nu::timeout_t ping_timeout = std::chrono::seconds(10);
    nu::now_t last_ping_received;
    // TODO: Please init at beginning
    std::atomic<nu::timeout_t> last_ping_time;
    nu::mutexed<std::vector<Address>> to_request;
    std::thread worker;
    std::atomic<bool> keep_working = true;

    ping_counter_t ping_counter;

    std::unique_ptr<channel> c;

  private:
    enum class verb : uint8_t {
      Ping = 0,

      Reset = 255,
      Shake = 254,
      Init = 253,
    };

  private:
    // Who needs init when we can just extrapolate from pings?
    inline void init() {
      using handshake_buf_t = nu::static_data<32>;

      // Send a reset
      c->send(nu::squash(verb::Reset, nu::data{}));

      // Send a shake
      static thread_local std::uniform_int_distribution<uint8_t> dist;

      handshake_buf_t our_buf;
      std::generate(our_buf.begin(), our_buf.end(),
                    []() { return dist(upsilon::csprng::standard); });
      c->send(nu::squash(verb::Reset, our_buf));

      ping_counter_t their_counter;

      // Recieve their reset ack
      if (need_to_recv_reset) {
        auto start = nu::now();
        auto patience = ping_timeout;

        while (patience < 0) {
          if (auto buf = c->receive().try_take(patience)) {
            verb v;
            nu::data b;
            nu::expand(buf, v, b);

            if (v != verb::Shake)
              continue;


          }
          else throw nu::timed_out{};
        }
      }




      // Set ping counter to a random value
      ping_counter = dist(upsilon::csprng::standard);

      // Send an initialisation request
      c->send(nu::squash(ping_counter, std::vector<Address>{}));

    }

    void _worker_body() {
      while (keep_working) {
        if (auto buf = c->receive().try_take(ping_timeout)) {
          ping_counter_t their_ping_counter;
          std::vector<Address> requested_rtts;

          nu::expand(buf, their_ping_counter, requested_rtts);

          // If it is a initialisation request, init with them
          if (their_ping_counter == 0) {
            init();
          }
          // If it is an old ping, ignore it
          else if (their_ping_counter != ping_counter)
            continue;

          ping_counter++;
        }
        // They missed a ping :(
        //
        // Ping them anyway though
        else;

        std::vector<Address> requested_rtts = std::move(**to_request);

        c->send(nu::squash(ping_counter, requested_rtts));
      }
    }

  public:
    inline nu::timeout_t last_ping() { return last_ping_time; }
    nu::cancellable<nu::timeout_t> request_rtt(const Address& addr);

  public:
    router_channel() {
      begin_init();

    }
  };

  // Return a ping as soon as we recieve one
  template<typename Address>
  class dynamic_routing_table : public routing_table<Address> {
  public:
    virtual std::future<void> ping(const Address&) = 0;

  public:
    nu::worm_mutexed<std::map<user::ref, Address>> _table;

  public:
    inline Address get_best_route(user::ref target) override {
      auto handle = _table.get_ro();
      if (auto iter = handle->find(target); iter != handle->end())
        return *iter;
      else return std::nullopt;
    }

    inline void add_route(const Address& addr, user::ref usr) override {
      auto handle = _table.get_rw();
      handle->insert_or_assign(addr, usr);
    }
  }
}
