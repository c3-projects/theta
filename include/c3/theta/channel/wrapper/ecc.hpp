#pragma once

#include <type_traits>

#include "c3/theta/channel/base.hpp"

//! Adding an ecc to something with an integrity guarantee is pointless,
//! as a corrupted message will cause it to be dropped regardless

namespace c3::theta::wrapper {
  class __reed_sol_van_impl;


  template<nu::n_bits_rep_t DoF, nu::n_bits_rep_t ErrorBits>
  class reed_sol_van {
    static_assert (DoF > ErrorBits, "Need at least 1 free bit for message!");
    constexpr static nu::n_bits_rep_t MessageBits = DoF - ErrorBits;

  public:
    static void encode(gsl::span<const nu::bit_datum<nu::dynamic_size>> in,
                       gsl::span<nu::bit_datum<nu::dynamic_size>> out);
    static void decode(gsl::span<const nu::bit_datum<nu::dynamic_size>> in,
                       gsl::span<nu::bit_datum<nu::dynamic_size>> out);

  public:
     //, typename = std::enable_if<(DoF < ErrorBits)>>
    std::unique_ptr<medium<MessageBits>> wrap(std::unique_ptr<medium<DoF>>&& base) {
      class tmp_medium : public medium<MessageBits> {
      public:
        std::unique_ptr<medium<DoF>> base;

      public:
        void send_points(gsl::span<nu::bit_datum<MessageBits>> in) override {
          std::vector<nu::bit_datum<nu::dynamic_size>> in_dyn(in.begin(), in.end());
          std::vector<nu::bit_datum<nu::dynamic_size>> out_dyn(in.size());
          encode(in_dyn, out_dyn);
          std::vector<nu::bit_datum<DoF>> out(out_dyn.begin(), out_dyn.end());
          base->send_points(out);
        }

        void receive_points(gsl::span<nu::bit_datum<MessageBits>> out) override {
          std::vector<nu::bit_datum<DoF>> in(out.size());
          base->receive_points(in);
          std::vector<nu::bit_datum<nu::dynamic_size>> in_dyn(in.size());
          std::vector<nu::bit_datum<nu::dynamic_size>> out_dyn(in.begin(), in.end());
          decode(in_dyn, out_dyn);
          std::copy(out_dyn.begin(), out_dyn.end(), out.begin());
        }

      public:
        constexpr tmp_medium(std::unique_ptr<medium<DoF>>&& base) : base{std::move(base)} {}
      };

      return std::make_unique<tmp_medium>(std::move(base));
    }
  };
}
