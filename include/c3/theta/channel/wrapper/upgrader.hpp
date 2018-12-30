#pragma once

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrapper {
//  /// So that one can apply ecc to a link
//  template<uint_fast8_t MaxFrameSize, size_t BeginSyncwordLenPoints = 0,
//           typename = std::enable_if<MaxFrameSize * 8 <= upsilon::MAX_BIT_DATUM_SIZE>>
//  std::unique_ptr<medium<MaxFrameSize * 8>> link2medium (

  using standard_packet_len_t = uint16_t;
  constexpr size_t standard_packet_size = std::numeric_limits<standard_packet_len_t>::max();

  using standard_message_len_t = uint64_t;

  template<upsilon::n_bits_rep_t DoF,
           size_t BeginSyncwordLenPoints = 1,
           size_t SyncRetries = 3 * BeginSyncwordLenPoints>
  class __medium2link : public link<standard_packet_size> {
  private:
    std::unique_ptr<medium<DoF>> _base;

  public:
    __medium2link(decltype(_base)&& wrappee) : _base{std::move(wrappee)} {}

  public:
    static constexpr std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> gen_syncword() {
      std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> ret;
      for (auto& i : ret)
        i = upsilon::bit_datum<DoF>::on_off();
      return ret;
    }
    static constexpr std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> begin_syncword = gen_syncword();

    inline void sync_rx() {
      if constexpr (BeginSyncwordLenPoints > 0) {
        // Try to find the syncword
        std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> maybe_begin_syncword;

        _base->receive_points(maybe_begin_syncword);

        if (maybe_begin_syncword != begin_syncword) {
          std::array<upsilon::bit_datum<DoF>, BeginSyncwordLenPoints> _1;

          auto* current = &maybe_begin_syncword;
          auto* next = &_1;

          for (size_t i = 0; i < SyncRetries; ++i) {
            _base->receive_points({ &current->back(), 1 });
            if (*current == begin_syncword) break;
            std::copy(current->begin() + 1, current->end(),
                      next->begin());
            std::swap(current, next);
          }
        }
      }
    }

  public:
    void transmit_frame(upsilon::data_const_ref b) override {
      if constexpr (BeginSyncwordLenPoints > 0)
        _base->transmit_points(begin_syncword);

      upsilon::data bytes = upsilon::squash_hybrid(static_cast<standard_packet_len_t>(b.size()), b);

      std::vector<upsilon::bit_datum<DoF>> points(upsilon::bit_datum<DoF>::split_len(bytes.size()));

      upsilon::bit_datum<DoF>::split(bytes, points);

      _base->transmit_points(points);
    }

    upsilon::data receive_frame() override {
      sync_rx();

      constexpr size_t len_n_points = upsilon::bit_datum<DoF>::split_len(upsilon::serialised_size<standard_packet_len_t>());

      std::vector<upsilon::bit_datum<DoF>> points(len_n_points);
      _base->receive_points(points);

      upsilon::static_buffer<standard_packet_len_t> len_buf;
      upsilon::bit_datum<DoF>::combine(points, len_buf);
      auto len = upsilon::deserialise<standard_packet_len_t>(len_buf);

      size_t total_bytes = upsilon::serialised_size<standard_packet_len_t>() + len;
      size_t total_points = upsilon::bit_datum<DoF>::split_len(total_bytes);
      points.resize(total_points);

      _base->receive_points({points.data() + len_n_points, static_cast<ssize_t>(points.size() - len_n_points)});

      auto ret = upsilon::bit_datum<DoF>::combine(points, upsilon::serialised_size<standard_packet_len_t>() * 8);

      ret.resize(len);

      return ret;
    }

    size_t receive_frame(upsilon::data_ref b) override {
      sync_rx();

      constexpr size_t len_n_points = upsilon::bit_datum<DoF>::split_len(upsilon::serialised_size<standard_packet_len_t>());

      std::vector<upsilon::bit_datum<DoF>> points(len_n_points);
      _base->receive_points(points);

      upsilon::static_buffer<standard_packet_len_t> len_buf;
      upsilon::bit_datum<DoF>::combine(points, len_buf);
      auto len = upsilon::deserialise<standard_packet_len_t>(len_buf);

      points.resize(upsilon::bit_datum<DoF>::split_len(upsilon::serialised_size<standard_packet_len_t>() + len));

      _base->receive_points({points.data() + len_n_points, static_cast<ssize_t>(points.size() - len_n_points)});

      upsilon::bit_datum<DoF>::combine(points, b, upsilon::serialised_size<standard_packet_len_t>() * 8);

      return std::min(static_cast<size_t>(b.size()), upsilon::bit_datum<DoF>::combine_len(points.size()));
    }
  };

  template<upsilon::n_bits_rep_t DoF, size_t BeginSyncwordLenPoints = 1>
  std::unique_ptr<link<standard_packet_size>> medium2link(
    std::unique_ptr<medium<DoF>>&& base) {
    static_assert(DoF > 0, "Must be able to send at least one bit per point");

    return std::make_unique<__medium2link<DoF, BeginSyncwordLenPoints>>(std::move(base));
  }

  template<size_t PacketLen>
  std::unique_ptr<channel> link2channel(std::unique_ptr<link<PacketLen>>&& base) {
    static_assert(PacketLen > 0, "Must be able to send at least one octet per packet");

    class __link2channel : public channel {
    private:
      std::unique_ptr<link<PacketLen>> _base;

    public:
      inline __link2channel(decltype(base)&& base) : _base{std::move(base)} {}

    public:
      inline void tx(upsilon::data_const_ref b) {
        size_t last_frame_size = b.size() % PacketLen;
        size_t n_full_frames = b.size() / PacketLen;

        for (size_t i = 0; i < n_full_frames; ++i)
          _base->transmit_frame({b.data() + i * PacketLen, PacketLen});

        if (last_frame_size != 0)
          _base->transmit_frame({b.data() + n_full_frames * PacketLen,
                                 static_cast<decltype(b)::size_type>(last_frame_size)});
      }

    public:
      void transmit_msg(upsilon::data_const_ref b) override {
        auto to_tx = upsilon::squash_hybrid(static_cast<standard_message_len_t>(b.size()), b);
        tx(to_tx);
      }

      upsilon::data receive_msg() override {
        upsilon::data in_buf;

        constexpr size_t expected_len_bytes = upsilon::serialised_size<standard_message_len_t>();

        in_buf.reserve(expected_len_bytes);

        do {
          auto new_frame = _base->receive_frame();

          // We must always get closer to our goal
          if (new_frame.size() == 0)
            throw upsilon::timed_out{};

          in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
        }
        while(in_buf.size() < expected_len_bytes);

        auto message_len = upsilon::deserialise<standard_message_len_t>({ in_buf.data(), expected_len_bytes });

        in_buf.erase(in_buf.begin(), in_buf.begin() + expected_len_bytes);
        in_buf.reserve(message_len);

        while (in_buf.size() < message_len) {
          auto new_frame = _base->receive_frame();

          // We must always get closer to our goal
          if (new_frame.size() == 0)
            throw upsilon::timed_out{};

          in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
        }

        return in_buf;
      }
    };

    return std::make_unique<__link2channel>(std::move(base));
  }

  // todo: reliability link wrapper
}
