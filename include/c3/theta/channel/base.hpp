#pragma once

#include <c3/upsilon/serialisation.hpp>
#include <c3/upsilon/hash.hpp>

#include <memory>

#include <functional>

namespace c3::theta {
  // Something that can chuck bits into and gather bits from some medium
  template<uint_fast8_t DoF>
  class medium {
  public:
    virtual void transmit_points(gsl::span<upsilon::bit_datum<DoF>>) = 0;
    virtual void receive_points(gsl::span<upsilon::bit_datum<DoF>>) = 0;

  public:
    virtual ~medium() = default;
  };

  // A medium that can send and receive integrous sequences of octets
  template<size_t MaxFrameSize>
  class channel {
  public:
    virtual void transmit_frame(upsilon::data_const_ref b) = 0;
    virtual upsilon::data receive_frame() = 0;

  public:
    virtual ~channel() = default;
  };

  // A channel that can reliably send and receive arbitarily long sequences of octets (up to 2^64 - 1)
  class link {
  public:
    virtual void transmit_msg(upsilon::data_const_ref b) = 0;
    virtual upsilon::data receive_msg() = 0;

  public:
    virtual ~link() = default;
  };

  // A secure link
  class conversation {
  public:
    virtual void transmit_msg(upsilon::data_const_ref b) = 0;
    virtual upsilon::data receive_msg() = 0;

  public:
    virtual ~conversation() = default;
  };

  /*
  class medium_wrapper {
  private:
    virtual void translate_transmit_bits(medium& m, gsl::span<const bool>) = 0;
    virtual void translate_receive_bits(medium& m, gsl::span<bool>) = 0;

  public:
    template<typename T>
    inline std::unique_ptr<T> wrap_medium(std::unique_ptr<T>&& t) {
      static_assert(std::is_base_of_v<T, medium>, "medium_wrapper requires a medium-like base");
      class ret_t : public T {
      public:
        std::unique_ptr<T> base;

      public:
        void transmit_bits(gsl::span<const bool> b) override {
          translate_transmit(*base, b);
        }
        void receive_bits(gsl::span<bool>f b) override {
          translate_receive(*base, b);
        }
      };

      return ret_t{.base = std::move(t)};
    }

  public:
    virtual ~medium_wrapper() = default;
  };

  template<typename Wrappee>
  inline std::unique_ptr<Wrappee> operator& (std::unique_ptr<Wrappee>&& w,
                                             const medium_wrapper& cw) {
    return cw.wrap_medium(w);
  }

  class channel_wrapper {
  private:
    virtual void translate_transmit_msg(channel& m, upsilon::data_const_ref) = 0;
    virtual upsilon::data translate_receive_msg(channel& m) = 0;

  public:
    template<typename T>
    inline std::unique_ptr<T> wrap_channel(std::unique_ptr<T>&& t) {
      static_assert(std::is_base_of_v<T, channel>, "medium_wrapper requires a medium-like base");
      class ret_t : public T {
      public:
        std::unique_ptr<T> base;

      public:
        void transmit_msg(upsilon::data_const_ref b) override {
          translate_transmit_msg(*base, b);
        }
        upsilon::data receive_msg() override {
          return translate_receive_msg(*base);
        }
      };

      return ret_t{.base = std::move(t)};
    }

  public:
    virtual ~channel_wrapper() = default;
  };

  template<typename Wrappee>
  inline std::unique_ptr<Wrappee> operator& (std::unique_ptr<Wrappee>&& w,
                                             const channel_wrapper& cw) {
    return cw.wrap_channel(w);
  }

  class universal_wrapper : public medium_wrapper, public channel_wrapper {
  public:
    virtual ~universal_wrapper() = default;
  };

  template<typename Wrapped, typename Wrappee>
  inline Wrapped operator&(Wrappee w, Wrapped(*func)(Wrappee)) {
   return (*func)(std::forward<Wrappee>(w));
  }
  */
}
