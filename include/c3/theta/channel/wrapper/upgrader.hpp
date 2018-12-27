#pragma once

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrappers {
  constexpr size_t standard_channel_size = std::numeric_limits<uint16_t>::max();

  template<uint_fast8_t DoF, size_t BeginSyncwordLenPoints = 0, size_t EndSyncwordLenPoints = 0>
  std::unique_ptr<channel<standard_channel_size>> medium2channel(
    std::unique_ptr<medium<DoF>>&& base) {
    static_assert(DoF > 0, "Must be able to send at least one bit per message");
    class tmp_channel : public channel<standard_channel_size> {
    public:
      std::unique_ptr<medium<DoF>> base;

    public:
      void transmit_frame(upsilon::data_const_ref b) {
        std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> begin_syncword;
        std::generate(begin_syncword.begin(), begin_syncword.end(), upsilon::bit_datum<DoF>::on_off);
        base->transmit_points(begin_syncword);

        std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> end_syncword;
        std::generate(end_syncword.begin(), end_syncword.end(), upsilon::bit_datum<DoF>::off_on);
        base->transmit_points(end_syncword);
      }

      upsilon::data receive_frame() {

      }
    };

    return std::make_unique<tmp_channel>(std::move(base));
  }
}
