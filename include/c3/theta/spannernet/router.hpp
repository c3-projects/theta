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

  template<typename Address>
  class router_channel;

  template<typename Address>
  using router_channel_table = nu::mutexed<std::map<Address, router_channel<Address>*>>;

  template<typename Address>
  class router_channel {
  public:
    using ping_counter_t = uint64_t;

  private:
    std::unique_ptr<channel> c;

    nu::timeout_t ping_timeout;
    nu::now_t last_ping_received;
    // TODO: Please init at beginning
    std::atomic<nu::timeout_t> last_ping_time;
    nu::mutexed<std::vector<Address>> to_request;
    std::thread worker;
    std::atomic<bool> keep_working = true;

    ping_counter_t ping_counter;

    std::shared_ptr<router_channel_table>

  private:
    enum class verb : uint8_t {
      Ping = 0,
      RttRequest = 1,
      RttFound = 2,
      RttNotFound = 3,
    };

  private:
    inline void init() {
      // Since we don't really care who goes first, as long as we agree,
      // a remote which forces a low or a high value doesn't really matter,
      // and as such does not need to be protected against.

      using ping_counter_t = uint64_t;

      // Send a shake
      static thread_local std::uniform_int_distribution<ping_counter_t> dist;


      ping_counter_t our_handshake;
      do our_handshake = dist(upsilon::csprng::standard);
      while (our_handshake == 0);
      c->send(nu::squash(verb::Reset, our_handshake));

      ping_counter_t their_handshake;

      // Recieve their shake
      {
        auto start = nu::now();
        auto finish = start + ping_timeout;

        while (nu::now() < finish) {
          if (auto buf = c->receive().try_take(ping_timeout)) {
            verb v;
            nu::data b;
            nu::expand(buf, v, b);

            if (v != verb::Ping)
              continue;
          }
          else throw nu::timed_out{};
        }
      }

      // We caught them in an overflow, set us to 1
      if (their_handshake == 0)
        ping_counter == 1;
      else if (their_handshake == our_handshake)
        throw std::runtime_error("Anomalous handshake");
      else
        ping_counter = std::max(our_handshake, their_handshake);
    }

    inline void _ping() {
      c->send(nu::squash(verb::Ping, ping_counter));
    }

    inline void _worker_body() {
      while (keep_working) {
        if (auto buf = c->receive().try_take(ping_timeout)) {
          verb v;
          nu::data payload;

          switch (v) {
            case(verb::Ping): {
              auto their_ping_counter = nu::deserialise<ping_counter_t>(payload);

              // If it is a newer packet, or they have negotiated a higher one, update our ping_counter
              if (their_ping_counter == 0 || their_ping_counter > ping_counter) {
                ping_counter = 0;
              }
              // If it is an older ping, or a lower negotiated one, ignore it
              else if (their_ping_counter < ping_counter)
                continue;

              // Increment the ping_counter, as we are the next one after theirs
              ping_counter++;

              // Send a ping
              _ping();
            } break;

            case (verb::OobRequest): {

            }; break;

            // Ignore bad messages
            default: continue;
          }

        }
        // They missed a ping :(
        //
        // Ping them anyway though :)
        else _ping();
      }
    }

  public:
    inline nu::timeout_t last_ping() { return last_ping_time; }
    nu::cancellable<nu::timeout_t> request_rtt(const Address& addr);
    inline void

  public:
    router_channel(decltype(c)&& base_channel,
                   nu::timeout_t ping_timeout = std::chrono::seconds(10)) :
      c{std::move(base_channel)}, ping_timeout{std::move(ping_timeout)} {
      init();
    }
  };

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
  };
}
