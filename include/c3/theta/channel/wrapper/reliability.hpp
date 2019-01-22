#pragma once

#include "c3/theta/channel/base.hpp"

#include <set>
#include <random>
#include <thread>

#include <c3/nu/data.hpp>
#include <c3/nu/structs.hpp>
#include <c3/nu/concurrency/mutexed.hpp>
#include <c3/nu/concurrency/concurrent_queue.hpp>

#include <queue>
#include <map>

namespace c3::theta::wrapper {
  using reliable_id_t = uint64_t;

  reliable_id_t generate_reliable_id();

  template<size_t PacketSize>
  constexpr size_t reliable_packet_size = PacketSize - nu::serialised_size<reliable_id_t>();

  template<size_t PacketSize>
  std::unique_ptr<link<reliable_packet_size<PacketSize>>> make_reliable(
      std::unique_ptr<link<PacketSize>>&& base,
      nu::timeout_t timeout = std::chrono::seconds(1)) {
    constexpr auto new_packet_size = reliable_packet_size<PacketSize>;

    class _make_reliable : public link<new_packet_size> {
    public:
      enum class frame_type : uint8_t {
        Msg = 0,
        Ack = 1,
      };

      class to_tx_t {
      public:
        reliable_id_t id;
        nu::data payload;
        std::promise<void> acked;

      public:
        inline to_tx_t(decltype(id) id, decltype(payload) payload, decltype(acked) acked) :
          id{std::move(id)}, payload{std::move(payload)}, acked{std::move(acked)} {}
      };

    private:
      std::unique_ptr<link<PacketSize>> _base;
      decltype(timeout) _timeout;

    private:
      nu::concurrent_queue<to_tx_t> to_tx;
      nu::concurrent_queue<reliable_id_t> to_retx;

      nu::concurrent_queue<nu::data> recv_frames;

      std::atomic_bool keep_reading = true;
      std::thread reader;

    public:
      _make_reliable(decltype(base) base, decltype(timeout) timeout) :
        _base{std::move(base)}, _timeout{timeout} {
        reader = std::thread{&_make_reliable::reader_body, this};
      }
      ~_make_reliable() {
        keep_reading = false;
        reader.join();
      }

    private:
      void reader_body() {
        std::map<reliable_id_t, std::pair<nu::data, std::promise<void>>> sent_ids;

        while (keep_reading) {
          try {
            while (to_tx.size() > 0) {
              auto i = to_tx.pop().get_or_cancel(_timeout).value();

              auto& val = sent_ids.emplace(i.id, std::pair{
                nu::squash_hybrid(frame_type::Msg, i.id, i.payload),
                std::move(i.acked)
              }).first->second;

              _base->send_frame(val.first);
            }

            while (to_retx.size() > 0) {
              auto i = to_retx.pop().get_or_cancel(_timeout).value();

              auto iter = sent_ids.find(i);

              if (iter != sent_ids.end())
                _base->send_frame(iter->second.first);
            }

            auto frame = _base->receive_frame().get_or_cancel(_timeout).value();

            frame_type type;
            reliable_id_t id;
            nu::data payload;

            nu::expand_hybrid(frame, type, id, payload);

            switch (type) {
              case (frame_type::Msg): {
                recv_frames.push(std::move(payload));
                _base->send_frame(nu::squash_hybrid(frame_type::Ack, id));
              } break;
              case (frame_type::Ack): {
                auto iter = sent_ids.find(id);

                if (iter != sent_ids.end()) {
                  iter->second.second.set_value();
                  sent_ids.erase(iter);
                }
              } break;
              default: break;
            }
          }
          catch(...) {}

          std::this_thread::yield();
        }
      }

    public:
      void send_frame(nu::data_const_ref b) override {
        std::promise<void> promise;
        std::future<void> future = promise.get_future();

        reliable_id_t id = generate_reliable_id();

        to_tx.push(to_tx_t{id, nu::data(b.begin(), b.end()), std::move(promise)});

        // Friendly worker thread that keeps pushing the data until it is received
        std::thread([=, future{std::move(future)}]{
          while (future.wait_for(_timeout) != std::future_status::ready)
            to_retx.push(id);
        }).detach();
      }

      nu::cancellable<nu::data> receive_frame() override {
        return recv_frames.pop();
      }
    };

    return std::make_unique<_make_reliable>(std::move(base), timeout);
  }
}
