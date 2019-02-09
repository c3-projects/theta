#pragma once

#include "c3/theta/channel.hpp"

#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/identity.hpp>
#include <c3/upsilon/hash.hpp>

#include <c3/nu/concurrency/concurrent_queue.hpp>
#include <c3/nu/concurrency/postbox.hpp>

namespace c3::theta::spannernet {
  class node {
  public:
    class message {
    public:
      /// Should be ephemeral
      upsilon::remote_agreer src_agreer;
      /// Should be published, and acts as verification of receiver
      upsilon::remote_agreer dst_agreer;
      upsilon::symmetric_algorithm _enc_alg;
      nu::data base_message_enc;
    };

  private:
    std::unique_ptr<floating_channel> _base;
    nu::concurrent_queue<message> _messages;

    nu::gateway_bool _stop_pump;
    std::thread _pump_thread;

  private:
    void _pump_body();

  public:
    inline nu::cancellable<message> receive() { return _messages.pop(); }
    inline size_t n_received() const { return _messages.size(); }

  public:
    inline router(std::unique_ptr<channel>&& base) : _base{std::move(base)} {
      _pump_thread = std::thread(&router::_pump_body, this);
    }

    inline ~router() {
      _stop_pump.open();
      _pump_thread.join();
    }
  };
}
