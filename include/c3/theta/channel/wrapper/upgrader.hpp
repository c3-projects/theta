#pragma once

#include <c3/nu/structs.hpp>

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrapper {
<<<<<<< Updated upstream
  using standard_packet_len_t = uint16_t;
  constexpr size_t standard_packet_size = std::numeric_limits<standard_packet_len_t>::max();
=======
  constexpr c3::nu::timeout_t default_medium_timeout = std::chrono::seconds(1);
>>>>>>> Stashed changes

  using standard_message_len_t = uint64_t;

  template<nu::n_bits_rep_t DoF,
           size_t BeginSyncwordLenPoints = 1,
           size_t SyncRetries = 3 * BeginSyncwordLenPoints>
  class __medium2link : public link<standard_packet_size> {
  private:
    std::unique_ptr<medium<DoF>> _base;

  public:
    __medium2link(decltype(_base)&& wrappee) : _base{std::move(wrappee)} {}

  public:
    static constexpr std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> gen_syncword() {
      std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> ret;
      for (auto& i : ret)
        i = nu::bit_datum<DoF>::on_off();
      return ret;
    }
    static constexpr std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> begin_syncword = gen_syncword();

<<<<<<< Updated upstream
    inline void sync_rx() {
      if constexpr (BeginSyncwordLenPoints > 0) {
        // Try to find the syncword
        std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> maybe_begin_syncword;

        _base->receive_points(maybe_begin_syncword);

        if (maybe_begin_syncword != begin_syncword) {
          std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> _1;

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
=======
  public:
    void send_frame(nu::data_const_ref b) override {
      std::scoped_lock lock{send_mutex};
>>>>>>> Stashed changes

  public:
    void transmit_frame(nu::data_const_ref b) override {
      if constexpr (BeginSyncwordLenPoints > 0)
        _base->transmit_points(begin_syncword);

      nu::data bytes = nu::squash_hybrid(static_cast<standard_packet_len_t>(b.size()), b);

      std::vector<nu::bit_datum<DoF>> points(nu::bit_datum<DoF>::split_len(bytes.size()));

      nu::bit_datum<DoF>::split(bytes, points);

      _base->transmit_points(points);
    }

    nu::data receive_frame() override {
      sync_rx();

<<<<<<< Updated upstream
      constexpr size_t len_n_points = nu::bit_datum<DoF>::split_len(nu::serialised_size<standard_packet_len_t>());

      std::vector<nu::bit_datum<DoF>> points(len_n_points);
      _base->receive_points(points);
=======
      std::thread([provider, this]() mutable {
        do {
          std::scoped_lock lock{receive_mutex};

          try {
            if constexpr (BeginSyncwordLenPoints > 0) {
              // Try to find the syncword
              std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> maybe_begin_syncword;
>>>>>>> Stashed changes

      nu::static_buffer<standard_packet_len_t> len_buf;
      nu::bit_datum<DoF>::combine(points, len_buf);
      auto len = nu::deserialise<standard_packet_len_t>(len_buf);

      size_t total_bytes = nu::serialised_size<standard_packet_len_t>() + len;
      size_t total_points = nu::bit_datum<DoF>::split_len(total_bytes);
      points.resize(total_points);

      _base->receive_points({points.data() + len_n_points, static_cast<ssize_t>(points.size() - len_n_points)});

<<<<<<< Updated upstream
      auto ret = nu::bit_datum<DoF>::combine(points, nu::serialised_size<standard_packet_len_t>() * 8);
=======
                while (!provider.is_decided()) {
                  _base->receive_points({ &current->back(), 1 });
                  if (*current == begin_syncword) break;
                  std::copy(current->begin() + 1, current->end(),
                            next->begin());
                  std::swap(current, next);
                }
              }
            }
>>>>>>> Stashed changes

      ret.resize(len);

      return ret;
    }

    size_t receive_frame(nu::data_ref b) override {
      sync_rx();

      constexpr size_t len_n_points = nu::bit_datum<DoF>::split_len(nu::serialised_size<standard_packet_len_t>());

<<<<<<< Updated upstream
      std::vector<nu::bit_datum<DoF>> points(len_n_points);
      _base->receive_points(points);
=======
            gsl::span<nu::bit_datum<DoF>> recv_buf = {
              points.data() + len_n_points,
              static_cast<ssize_t>(points.size() - len_n_points)
            };

            _base->receive_points(recv_buf);
>>>>>>> Stashed changes

      nu::static_buffer<standard_packet_len_t> len_buf;
      nu::bit_datum<DoF>::combine(points, len_buf);
      auto len = nu::deserialise<standard_packet_len_t>(len_buf);

      points.resize(nu::bit_datum<DoF>::split_len(nu::serialised_size<standard_packet_len_t>() + len));

<<<<<<< Updated upstream
      _base->receive_points({points.data() + len_n_points, static_cast<ssize_t>(points.size() - len_n_points)});

      nu::bit_datum<DoF>::combine(points, b, nu::serialised_size<standard_packet_len_t>() * 8);
=======
            provider.maybe_provide([&]() -> std::optional<nu::data> { return std::move(ret); });
          }
          catch(...) {}
        }
        while (!provider.is_decided());
      }).detach();
>>>>>>> Stashed changes

      return std::min(static_cast<size_t>(b.size()), nu::bit_datum<DoF>::combine_len(points.size()));
    }
  };

<<<<<<< Updated upstream
  template<nu::n_bits_rep_t DoF, size_t BeginSyncwordLenPoints = 1>
  std::unique_ptr<link<standard_packet_size>> medium2link(
=======
  template<nu::n_bits_rep_t DoF, size_t BeginSyncwordLenPoints = 0, typename PacketLen = uint16_t>
  std::unique_ptr<link<std::numeric_limits<PacketLen>::max()>> medium2link(
>>>>>>> Stashed changes
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
      inline void tx(nu::data_const_ref b) {
        size_t last_frame_size = b.size() % PacketLen;
        size_t n_full_frames = b.size() / PacketLen;

        for (size_t i = 0; i < n_full_frames; ++i)
          _base->transmit_frame({b.data() + i * PacketLen, PacketLen});

<<<<<<< Updated upstream
        if (last_frame_size != 0)
          _base->transmit_frame({b.data() + n_full_frames * PacketLen,
                                 static_cast<decltype(b)::size_type>(last_frame_size)});
      }
=======
    inline void send_msg(nu::data_const_ref b) override {
      std::scoped_lock lock{write_mutex};
>>>>>>> Stashed changes

    public:
      void transmit_msg(nu::data_const_ref b) override {
        auto to_tx = nu::squash_hybrid(static_cast<standard_message_len_t>(b.size()), b);
        tx(to_tx);
      }

      nu::data receive_msg() override {
        nu::data in_buf;

        constexpr size_t expected_len_bytes = nu::serialised_size<standard_message_len_t>();

        in_buf.reserve(expected_len_bytes);

        do {
          auto new_frame = _base->receive_frame();

          // We must always get closer to our goal
          if (new_frame.size() == 0)
            throw nu::timed_out{};

<<<<<<< Updated upstream
          in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
        }
        while(in_buf.size() < expected_len_bytes);
=======
    inline nu::data try_get_frame() {
      auto new_frame_c = _base->receive_frame();
      std::optional<nu::data> new_frame_o = new_frame_c.get_or_cancel(PollTimeout);
      if (!new_frame_o.has_value())
        throw nu::timed_out{};
>>>>>>> Stashed changes

        auto message_len = nu::deserialise<standard_message_len_t>({ in_buf.data(), expected_len_bytes });

        in_buf.erase(in_buf.begin(), in_buf.begin() + expected_len_bytes);
        in_buf.reserve(message_len);

        while (in_buf.size() < message_len) {
          auto new_frame = _base->receive_frame();

          // We must always get closer to our goal
          if (new_frame.size() == 0)
            throw nu::timed_out{};

<<<<<<< Updated upstream
          in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
        }
=======
      // TODO: seqid
      std::thread([=]() mutable {
        do {
          try {
            while(true) {
              if (provider.is_decided()) return;
              try {
                auto frame = try_get_frame();
                if (std::equal(frame.begin(), frame.end(),
                               syncframe.begin(), syncframe.end())) break;
              }
              catch (const nu::timed_out&) {}
            }

            nu::data in_buf;

            constexpr size_t expected_len_bytes = nu::serialised_size<MessageLen>();

            in_buf.reserve(expected_len_bytes);

            do {
              if (provider.is_decided()) return;
              try {
                nu::data new_frame = try_get_frame();
                in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
              }
              catch (const nu::timed_out&) {}
            }
            while(in_buf.size() < expected_len_bytes);

            MessageLen message_len = nu::deserialise<MessageLen>({ in_buf.data(), expected_len_bytes });

            if (message_len > std::numeric_limits<size_t>::max())
              throw std::runtime_error("Cannot possibly fit struct in memory");

            in_buf.clear();
            in_buf.reserve(message_len);

            while (in_buf.size() < message_len) {
              if (provider.is_decided()) return;

              try {
                nu::data new_frame = try_get_frame();
                in_buf.insert(in_buf.end(), new_frame.begin(), new_frame.end());
              }
              catch (const nu::timed_out&) {}
            }

            provider.maybe_provide([&]() -> std::optional<nu::data> { return in_buf; });
          }
          catch(...) {}
        }
        while (!provider.is_decided());
      }).detach();
>>>>>>> Stashed changes

        return in_buf;
      }
    };

    return std::make_unique<__link2channel>(std::move(base));
  }
}
