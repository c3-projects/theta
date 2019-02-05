#pragma once

#include <c3/nu/data.hpp>
#include <c3/nu/bits.hpp>
#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/concurrency/timeout.hpp>

#include <memory>

#include <functional>
#include <future>
#include <chrono>

//! All of these should throw a timed_out exception when a reasonable amount of time has passed
//! If the timeout is non-positive, an exception should be immediately thrown

namespace c3::theta {
  // Something that can chuck bits into and gather bits from some medium
  //
  // Buffering is recommended but not mandatory
  // Must timeout if a reasonable time has passed
  template<nu::n_bits_rep_t DoF>
  class medium {
  public:
    virtual void send(gsl::span<const nu::bit_datum<DoF>>) = 0;
    virtual void receive(gsl::span<nu::bit_datum<DoF>>) = 0;

  public:
    virtual ~medium() = default;
  };

  // A medium that can send and receive sequences of octets
  template<size_t MaxFrameSize>
  class link {
  public:
    virtual void send(nu::data_const_ref b) = 0;

    virtual nu::cancellable<nu::data> receive() = 0;

  public:
    virtual ~link() = default;
  };

  // A medium that can send and receive sequences of octets to arbitary hosts
  template<size_t MaxFrameSize, typename EpType>
  class floating_link {
  public:
    virtual EpType get_ep() = 0;

    virtual void send(EpType dest, nu::data_const_ref b) = 0;

    virtual nu::cancellable<std::pair<EpType, nu::data>> receive() = 0;
  public:
    virtual ~floating_link() = default;
  };

  // A medium that can send and receive arbitarily long sequences of octets (up to 2^64 - 1)
  // but without message delimiters
  class stream_channel {
  public:
    virtual void send(nu::data_const_ref b) = 0;
    virtual nu::cancellable<size_t> receive(nu::data_ref) = 0;

  public:
    virtual ~stream_channel() = default;
  };

  // A link that can send and receive arbitarily long sequences of octets (up to 2^64 - 1)
  class channel {
  public:
    virtual void send(nu::data_const_ref) = 0;

    virtual nu::data receive() = 0;

  public:
    virtual ~channel() = default;
  };
}
