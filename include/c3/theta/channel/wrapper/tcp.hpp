#pragma once

#include <map>

#include "c3/theta/channel/base.hpp"
#include "c3/theta/channel/tcp.hpp"
#include "c3/theta/channel/utils/ports.hpp"
#include "c3/nu/concurrency/concurrent_queue.hpp"
#include "c3/nu/integral.hpp"

#include "c3/nu/data/helpers.hpp"

namespace c3::theta::wrapper {
  class tcp_header_t : public nu::static_serialisable<tcp_header_t> {
  public:
    class flags_t {
    public:
      // TODO: implement ECN
      bool ns = false;
      bool cwr = false;
      bool ece = false;

      bool urg = false;
      bool ack = false;
      bool psh = false;
      bool syn = false;
      bool fin = false;
    };

  public:
    tcp_port_t src_port;
    tcp_port_t dst_port;
    uint32_t seq_id;
    uint32_t ack_id;
    nu::bit_datum<4> data_offset;
    nu::bit_datum<3> reserved = 0;
    flags_t flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;

  private:
    void _serialise_static(nu::data_ref) const override;
    C3_NU_DEFINE_STATIC_DESERIALISE(tcp_header_t, 20, b);
  };

  template<size_t MaxFrameSize, typename EpType>
  class tcp_host {
  private:
    std::unique_ptr<floating_link<MaxFrameSize, EpType>> _base;

    nu::worm_mutexed<std::map<tcp_port_t,
                              std::shared_ptr<nu::concurrent_queue<nu::data>>>> _pumps;

    port_controller<tcp_port_t> _ports;

  private:
    class client;
    friend client;

    class server;
    friend server;
  };

  class tcp_host::client : public theta::tcp_client<tcp_ep<EpType>> {
  public:
    std::shared_ptr<nu::concurrent_queue<nu::data>> pump;
    tcp_port_t local_port;

    EpType remote_ep;
    tcp_port_t remote_port;
  };

  template<size_t MaxFrameSize, typename EpType>
  class tcp_server;

  template<size_t MaxFrameSize, typename EpType>
  class tcp_client : public theta::tcp_client<tcp_ep<EpType>> {
  public:
    friend class tcp_server<MaxFrameSize, EpType>;

  private:
    struct request {
      nu::cancellable_provider<nu::data> provider;
      size_t requested_bytes;
    };

  private:
    std::shared_ptr<floating_link<MaxFrameSize, EpType>> _base;
    tcp_port_t _local_port;

    EpType _remote;
    tcp_port_t _remote_port;

    std::thread _pumper_thread;
    std::thread _responder_thread;
    std::atomic<bool> _keep_working = true;

    std::shared_ptr<nu::concurrent_queue<nu::data>> pump;
    std::shared_ptr<nu::concurrent_queue<request>> requests;

  private:
    void _pumper_thread_body() {
      static constexpr nu::timeout_t poll_timout = std::chrono::milliseconds(10);

      while (_keep_working) {
        try {
          auto frame_c = _base->receive_frame();
          auto frame_o = frame_c.get_or_cancel(poll_timout);
          nu::data frame = frame_o.value();

          if (frame.size() < nu::serialised_size<tcp_header_t>())
            throw std::runtime_error("Received packet too small");

          auto header = nu::deserialise<tcp_header_t>({frame.data(), nu::serialised_size<tcp_header_t>()});


        }
        catch (...) {}
      }
    }

  public:
    virtual tcp_ep<EpType> get_local_ep() override { return { _base->get_ep(), _local_port }; };
    virtual tcp_ep<EpType> get_remote_ep() override { return { _remote, _remote_port }; }

  public:
    virtual void send_data(nu::data_const_ref b) override {
      tcp_header_t header;
      header.src_port = _local_port;
      header.dst_port = _remote_port;
      // TODO: rest of header fill in

      nu::data to_send = nu::serialise(header);
      to_send.insert(to_send.end(), b.begin(), b.end());

      // TODO: make sure we get an ack (worker?)

      _base->send_data(to_send, _remote);
    }

    virtual nu::cancellable<size_t> receive_data(nu::data_ref b) override {
      // Pull from data pool
    }

  public:
    virtual ~tcp_client() override {
      _keep_working = false;

      if (_pumper_thread.joinable())
        _pumper_thread.join();

      if (_responder_thread.joinable())
        _responder_thread.join();
    }
  };

  template<size_t MaxFrameSize, typename EpType>
  class tcp_server : public theta::tcp_server<tcp_ep<EpType>> {
  private:
    std::shared_ptr<floating_link<MaxFrameSize, EpType>> _base;
    tcp_port_t port;

    nu::worm_mutexed<std::map<tcp_ep<EpType>,
                              std::shared_ptr<nu::concurrent_queue<nu::data>>>> _pumps;

  private:
    class pseudoclient : public theta::tcp_client<tcp_ep<EpType>> {
    private:
      std::thread

    public:

    };

  public:
    virtual nu::cancellable<std::shared_ptr<theta::tcp_client<EpType>>> accept() override {

    }

    virtual tcp_ep<EpType> get_ep() override { return { _base->get_ep(), port }; };

  public:
    virtual ~tcp_server() override;
  };
}

#include "c3/nu/data/clean_helpers.hpp"
