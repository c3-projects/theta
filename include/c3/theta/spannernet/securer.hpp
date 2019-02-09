#pragma once

#include "c3/theta/channel.hpp"

#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/identity.hpp>
#include <c3/upsilon/hash.hpp>

#include <c3/nu/concurrency/concurrent_queue.hpp>
#include <c3/nu/concurrency/postbox.hpp>

namespace c3::theta::spannernet {
  constexpr size_t id_hash_len = 16;

  class securer;

  class base_message {
  public:
    /// Acts as verification of sender
    upsilon::identity sender;
    nu::data signature;
    nu::data message;
  };
}

