#pragma once

#include <c3/nu/concurrency/cancellable.hpp>
#include <c3/nu/data.hpp>

#include "c3/theta/channel/base.hpp"

namespace c3::theta {
  template<typename EpType>
  class tcp_server;

  template<typename EpType>
  class tcp_client : public stream_channel {
    friend class tcp_server<EpType>;
  public:
    virtual EpType get_local_ep() = 0;
    virtual EpType get_remote_ep() = 0;

  public:
    virtual void send_data(nu::data_const_ref) override = 0;

    virtual nu::cancellable<size_t> receive_data(nu::data_ref b) override = 0;

  public:
    virtual ~tcp_client() = default;
  };

  template<typename EpType>
  class tcp_server {
  public:
    virtual nu::cancellable<std::shared_ptr<tcp_client<EpType>>> accept() = 0;

    virtual EpType get_ep() = 0;

  public:
    ~tcp_server() = default;
  };
}
