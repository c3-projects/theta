#pragma once

#include <c3/upsilon/serialisation.hpp>

#include <memory>

#include <functional>

namespace c3::theta {
  using bits = std::vector<bool>;

  template<bool> struct Range;

  template<size_t DoF, typename = Range<true>>
  class data_point_rep;

  template<size_t DoF>
  class data_point_rep<DoF, Range<(DoF >= 1 && DoF < 8)>> {
  public:
    using type = uint8_t;
    static constexpr type Mask = std::numeric_limits<type>::max() >> DoF;
  };

  template<size_t DoF>
  class data_point_rep<DoF, Range<(DoF >= 8 && DoF < 16)>> {
  public:
    using type = uint16_t;
    static constexpr type Mask = std::numeric_limits<type>::max() >> DoF;
  };

  template<size_t DoF>
  class data_point_rep<DoF, Range<(DoF >= 16 && DoF < 32)>> {
  public:
    using type = uint32_t;
    static constexpr type Mask = std::numeric_limits<type>::max() >> DoF;
  };

  template<size_t DoF>
  class data_point_rep<DoF, Range<(DoF >= 32 && DoF < 64)>> {
  public:
    using type = uint64_t;
    static constexpr type Mask = std::numeric_limits<type>::max() >> DoF;
  };

  template<size_t DoF>
  class data_point {
  public:
    using rep_t = typename data_point_rep<DoF>::type;

  private:
    rep_t _value;

  public:
    inline data_point<DoF>& safe_set(rep_t new_val) {
      _value = new_val & data_point_rep<DoF>::Mask;
    }
    inline data_point<DoF>& unsafe_set(rep_t new_val) {
      _value = new_val;
    }
    inline data_point<DoF>& operator=(rep_t new_val) {
      safe_set(new_val);
    }

    inline rep_t get() const { return _value; }

    inline operator rep_t() const { return get(); }
    inline rep_t operator*() { return get(); }
  };

  // Something that can chuck bits into and gather bits from some medium
  template<size_t DoF>
  class medium {
  public:
    virtual void transmit_points(gsl::span<data_point<DoF>>) = 0;
    virtual void receive_points(gsl::span<const data_point<DoF>>) = 0;

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

  // A channel that can send and receive arbitarily long sequences of octets
  class unreliable_link {
  public:
    virtual void transmit_msg(upsilon::data_const_ref b) = 0;
    virtual upsilon::data receive_msg() = 0;

  public:
    virtual ~unreliable_link() = default;
  };

  // A channel that can reliably send and receive arbitarily long sequences of octets
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
