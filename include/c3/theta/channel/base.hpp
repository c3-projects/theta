#pragma once

#include <c3/nu/data.hpp>
#include <c3/nu/bits.hpp>
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
    virtual void transmit_points(gsl::span<const nu::bit_datum<DoF>>) = 0;
    virtual void receive_points(gsl::span<nu::bit_datum<DoF>>) = 0;

  public:
    virtual ~medium() = default;
  };

  // A medium that can send and receive sequences of octets
  template<size_t MaxFrameSize>
  class link {
  public:
    virtual void transmit_frame(nu::data_const_ref b) = 0;

    virtual nu::data receive_frame() = 0;
    /// Discards remainder of the frame
    virtual size_t receive_frame(nu::data_ref b) {
      auto f = receive_frame();

<<<<<<< Updated upstream
      ssize_t n_to_copy = std::min(b.size(), static_cast<ssize_t>(f.size()));
=======
  // A medium that can send and receive sequences of octets to arbitary hosts
  template<size_t MaxFrameSize, typename EpType>
  class floating_link {
  public:
    virtual EpType get_ep();

    virtual bool bind(const EpType&);

    virtual void send_frame(nu::data_const_ref b, EpType dest) = 0;

    virtual nu::cancellable<std::pair<EpType, nu::data>> receive_frame() = 0;
  public:
    virtual ~floating_link() = default;
  };

  // A medium that can send and receive arbitarily long sequences of octets (up to 2^64 - 1)
  // but without message delimiters
  class stream_channel {
  public:
    virtual void send_data(nu::data_const_ref b) = 0;
>>>>>>> Stashed changes

      std::copy(f.begin(), f.begin() + n_to_copy, b.begin());

      return static_cast<size_t>(n_to_copy);
    }

  public:
    virtual ~link() = default;
  };

  // A link that can send and receive arbitarily long sequences of octets (up to 2^64 - 1)
  class channel {
  public:
    virtual void transmit_msg(nu::data_const_ref) = 0;

    virtual nu::data receive_msg() = 0;
    /// Discards remainder of the frame
    virtual size_t receive_msg(nu::data_ref b) {
      auto f = receive_msg();

      ssize_t n_to_copy = std::min(b.size(), static_cast<ssize_t>(f.size()));

      std::copy(f.begin(), f.begin() + n_to_copy, b.begin());

      return static_cast<size_t>(n_to_copy);
    }

  public:
    virtual ~channel() = default;
  };

  // A secure channel
  class conversation {
  public:
    virtual void transmit_msg(nu::data_const_ref b) = 0;

    virtual nu::data receive_msg() = 0;

  public:
    virtual ~conversation() = default;
  };
}
