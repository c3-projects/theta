#pragma once

#include "c3/theta/channel/tcp.hpp"

namespace c3::theta::ip {
  using ipv4_address = std::array<uint8_t, 4>;
  using ipv6_address = std::array<uint8_t, 16>;

  struct ipv4_ep { ipv4_address addr; uint16_t port; };
  struct ipv6_ep { ipv6_address addr; uint16_t port; };

  inline bool operator==(const ipv4_ep& a, const ipv4_ep& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  inline bool operator==(const ipv6_ep& a, const ipv6_ep& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  inline bool operator!=(const ipv4_ep& a, const ipv4_ep& b) {
    return !(a == b);
  }
  inline bool operator!=(const ipv6_ep& a, const ipv6_ep& b) {
    return !(a == b);
  }

  constexpr uint16_t PORT_ANY = 0;

  constexpr ipv4_address IPV4_ANY = { 0, 0, 0, 0 };
  constexpr ipv6_address IPV6_ANY = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

  constexpr ipv4_address IPV4_LOOPBACK = { 127, 0, 0, 1 };
  constexpr ipv6_address IPV6_LOOPBACK = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

  template<typename EpType>
  class tcp_server;

  template<typename EpType>
  class tcp_client : public theta::tcp_client<EpType> {
    friend class tcp_server<EpType>;
  private:
    void* impl;

  public:
    virtual EpType get_local_ep() override ;
    virtual EpType get_remote_ep() override;

  public:
    virtual void send_data(nu::data_const_ref) override;

    virtual nu::cancellable<size_t> receive_data(nu::data_ref b) override;

  public:
    /// Unsafe construction from impl
    tcp_client(void* impl) : impl{impl} {}

  public:
    tcp_client() : impl{nullptr} {};
    virtual ~tcp_client();

    tcp_client<EpType>& operator=(const tcp_client<EpType>&) = delete;
    tcp_client(const tcp_client<EpType>&) = delete;

    inline tcp_client<EpType>& operator=(tcp_client<EpType>&& other) {
      impl = other.impl;
      other.impl = nullptr;
      return *this;
    };
    inline tcp_client(tcp_client&& other) : impl{other.impl} {
      other.impl = nullptr;
    }

  public:
    static nu::cancellable<std::shared_ptr<tcp_client<EpType>>> connect(EpType remote);
  };

  template<typename EpType>
  class tcp_server : public theta::tcp_server<EpType> {
  private:
    void* impl;

  public:
    nu::cancellable<std::shared_ptr<theta::tcp_client<EpType>>> accept() override;

    virtual EpType get_ep() override;

  public:
    virtual bool bind(EpType);

  public:
    tcp_server();
    tcp_server(EpType);

    tcp_server<EpType>& operator=(const tcp_server<EpType>&) = delete;
    tcp_server(const tcp_server<EpType>&) = delete;

    inline tcp_server<EpType>& operator=(tcp_server<EpType>&& other) {
      impl = other.impl;
      other.impl = nullptr;
      return *this;
    };
    inline tcp_server(tcp_server&& other) : impl{other.impl} {
      other.impl = nullptr;
    }

    virtual ~tcp_server();
  };

  template class tcp_client<ipv4_ep>;
  template class tcp_client<ipv6_ep>;

  template class tcp_server<ipv4_ep>;
  template class tcp_server<ipv6_ep>;
}
