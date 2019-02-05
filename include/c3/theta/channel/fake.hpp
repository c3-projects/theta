#pragma once

#include <map>
#include <memory>

#include "c3/theta/channel/base.hpp"

#include "c3/nu/concurrency/mutexed.hpp"
#include "c3/nu/concurrency/concurrent_queue.hpp"

namespace c3::theta {
  template<size_t MaxFrameSize = 512, typename Ep = uint16_t>
  class fake_floating_link_controller {
  private:
    using dgram_t = std::pair<Ep, nu::data>;

    class client : public floating_link<MaxFrameSize, Ep> {
    public:
      fake_floating_link_controller* parent;
      Ep local_ep;

    public:
      inline nu::cancellable<std::pair<Ep, nu::data>> receive() override {
        auto handle = parent->boxes.get_ro();

        auto iter = handle->find(local_ep);

        [[unlikely]]
        if (iter == handle->end())
          throw std::runtime_error("Box disappeared");

        return iter->second->pop();
      }

      inline void send(Ep remote, nu::data_const_ref b) override {
        auto handle = parent->boxes.get_ro();

        auto iter = handle->find(local_ep);

        [[unlikely]]
        if (iter == handle->end())
          throw std::runtime_error("Box disappeared");

        iter->second->push(dgram_t{remote, nu::data{b.begin(), b.end()}});
      }

      inline Ep get_ep() override { return local_ep; }

    public:
      inline client(decltype(parent) parent, decltype(local_ep) local_ep) :
        parent{parent}, local_ep{local_ep} {}
    };

  private:
    nu::worm_mutexed<std::map<Ep, std::unique_ptr<nu::concurrent_queue<dgram_t>>>> boxes;

  public:
    inline std::unique_ptr<floating_link<MaxFrameSize, Ep>> listen(Ep ep) {
      auto res = boxes.get_rw()->emplace(ep, std::make_unique<nu::concurrent_queue<dgram_t>>());

      if (!res.second)
        throw std::runtime_error("endpoint in use");

      return std::make_unique<client>(this, ep);
    }
  };
}
