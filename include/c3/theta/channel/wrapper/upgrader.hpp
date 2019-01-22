#pragma once

#include <c3/nu/structs.hpp>
#include <c3/nu/int_maths.hpp>

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrapper {
  constexpr c3::nu::timeout_t default_medium_timeout = std::chrono::milliseconds(1);

  template<nu::n_bits_rep_t DoF, size_t BeginSyncwordLenPoints = 0, typename PacketLen = uint16_t>
  class _medium2link : public link<std::numeric_limits<PacketLen>::max()> {
  private:
    std::unique_ptr<medium<DoF>> _base;

    std::mutex send_mutex;
    std::mutex receive_mutex;

  public:
    _medium2link(decltype(_base)&& wrappee) : _base{std::move(wrappee)} {}

  public:
    static constexpr std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> gen_syncword() {
      std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> ret;
      for (auto& i : ret)
        i = nu::bit_datum<DoF>::on_off();
      return ret;
    }
    static constexpr std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> begin_syncword = gen_syncword();

  public:
    void send_frame(nu::data_const_ref b) override {
      std::lock_guard lock{send_mutex};

      if constexpr (BeginSyncwordLenPoints > 0)
        _base->send_points(begin_syncword);

      nu::data bytes = nu::squash_hybrid(static_cast<PacketLen>(b.size()), b);

      std::vector<nu::bit_datum<DoF>> points(nu::bit_datum<DoF>::split_len(bytes.size()));

      nu::bit_datum<DoF>::split(bytes, points);

      _base->send_points(points);
    }

    nu::cancellable<nu::data> receive_frame() override {
      nu::cancellable_provider<nu::data> provider;

      std::thread([=]() mutable {
        do {
          try {
            std::lock_guard lock{receive_mutex};

            if constexpr (BeginSyncwordLenPoints > 0) {
              // Try to find the syncword
              std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> maybe_begin_syncword;

              _base->receive_points(maybe_begin_syncword);

              if (maybe_begin_syncword != begin_syncword) {
                std::array<nu::bit_datum<DoF>, BeginSyncwordLenPoints> _1;

                auto* current = &maybe_begin_syncword;
                auto* next = &_1;

                while (!provider.is_cancelled()) {
                  _base->receive_points({ &current->back(), 1 });
                  if (*current == begin_syncword) break;
                  std::copy(current->begin() + 1, current->end(),
                            next->begin());
                  std::swap(current, next);
                }
              }
            }

            constexpr size_t len_n_points = nu::bit_datum<DoF>::split_len(nu::serialised_size<PacketLen>());

            std::vector<nu::bit_datum<DoF>> points(len_n_points);
            _base->receive_points(points);

            nu::static_buffer<PacketLen> len_buf;
            nu::bit_datum<DoF>::combine(points, len_buf);
            auto len = nu::deserialise<PacketLen>(len_buf);

            size_t total_bytes = nu::serialised_size<PacketLen>() + len;
            size_t total_points = nu::bit_datum<DoF>::split_len(total_bytes);
            points.resize(total_points);

            _base->receive_points({points.data() + len_n_points,
                                   static_cast<ssize_t>(points.size() - len_n_points)});

            auto ret = nu::bit_datum<DoF>::combine(points, nu::serialised_size<PacketLen>() * 8);

            ret.resize(len);

            provider.maybe_provide([&]() -> std::optional<nu::data> { return ret; });
          }
          catch(...) {}
        }
        while (!provider.is_cancelled());
      }).detach();

      return provider.get_cancellable();
    }
  };

  template<nu::n_bits_rep_t DoF, size_t BeginSyncwordLenPoints = 1, typename PacketLen = uint16_t>
  std::unique_ptr<link<std::numeric_limits<PacketLen>::max()>> medium2link(
    std::unique_ptr<medium<DoF>>&& base) {
    static_assert(DoF > 0, "Must be able to send at least one bit per point");

    return std::make_unique<_medium2link<DoF, BeginSyncwordLenPoints, PacketLen>>(std::move(base));
  }

  template<size_t PacketLen,
           typename MessageLen = uint64_t,
           nu::timeout_t::rep PollTimeoutVal = default_medium_timeout.count()>
  class _link2channel : public channel {
  public:
    static constexpr nu::timeout_t PollTimeout{PollTimeoutVal};
  private:
    std::unique_ptr<link<PacketLen>> _base;

    std::mutex write_mutex;
    std::mutex read_mutex;

  public:
    inline _link2channel(decltype(_base)&& base) : _base{std::move(base)} {}

  public:
    static inline nu::static_buffer<MessageLen> gen_syncframe() {
      nu::static_buffer<MessageLen> ret;
      nu::serialise_static<MessageLen>(0, ret);
      return ret;
    }

    nu::static_buffer<MessageLen> syncframe = gen_syncframe();

    inline void send_msg(nu::data_const_ref b) override {
      std::lock_guard lock{write_mutex};

      _base->send_frame(syncframe);

      auto to_tx = nu::squash_hybrid(static_cast<MessageLen>(b.size()), b);

      size_t last_frame_size = b.size() % PacketLen;
      size_t n_full_frames = b.size() / PacketLen;

      for (size_t i = 0; i < n_full_frames; ++i)
        _base->send_frame({b.data() + i * PacketLen, PacketLen});

      if (last_frame_size != 0)
        _base->send_frame({b.data() + n_full_frames * PacketLen,
                           static_cast<decltype(b)::size_type>(last_frame_size)});
    }

    inline nu::data try_get_frame() {
      auto new_frame_c = _base->receive_frame();
      auto new_frame_o = new_frame_c.get_or_cancel(PollTimeout);
      if (!new_frame_o)
        throw nu::timed_out{};

      nu::data new_frame = *new_frame_o;

      // We must always get closer to our goal
      if (new_frame.size() == 0)
        throw nu::timed_out{};

      return new_frame;
    }

    inline nu::cancellable<nu::data> receive_msg() override {
      nu::cancellable_provider<nu::data> provider;

      // TODO: seqid
      std::thread([=]() mutable {
        do {
          try {
            while(true) {
              if (provider.is_cancelled()) return;
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
              if (provider.is_cancelled()) return;
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
              if (provider.is_cancelled()) return;

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
        while (!provider.is_cancelled());
      }).detach();

      return provider.get_cancellable();
    }
  };

  template<size_t PacketLen,
           typename MessageLen = uint64_t,
           nu::timeout_t::rep PollTimeoutVal = default_medium_timeout.count()>
  std::unique_ptr<channel> link2channel(std::unique_ptr<link<PacketLen>>&& base) {
    static_assert(PacketLen > 0, "Must be able to send at least one octet per packet");

    return std::make_unique<_link2channel<PacketLen, MessageLen, PollTimeoutVal>>(std::move(base));
  }
}
