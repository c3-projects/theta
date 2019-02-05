#pragma once

#include <map>

#include <c3/nu/data.hpp>
#include <c3/nu/concurrency/concurrent_queue.hpp>

#include "c3/theta/channel/tcp.hpp"

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::wrapper {
  template<size_t MaxFrameSize, typename BaseAddr>
  class tcp_host : public theta::tcp_host<BaseAddr> {
  private:
    class client;
    friend client;

    class server;
    friend server;

  private:
    std::unique_ptr<floating_link<MaxFrameSize, BaseAddr>> _base;

    nu::worm_mutexed<std::map<tcp_port_t,
                              std::shared_ptr<nu::concurrent_queue<nu::data>>>> _received;

    port_controller<tcp_port_t> _ports;

  public:
    nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> connect(tcp_ep<BaseAddr> remote) = 0;
    std::unique_ptr<tcp_server<BaseAddr>> listen(tcp_port_t local) = 0;
  };
}

#include <c3/nu/data/clean_helpers.hpp>
