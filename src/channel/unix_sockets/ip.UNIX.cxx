#include "c3/theta/channel/ip.hpp"

#include "common.UNIX.hxx"

namespace c3::theta {
  template<>
  struct _sockaddr_t<tcp_ep_t<ip::address_v4>> { using type = sockaddr_in; };
  template<>
  struct _baseep_t<sockaddr_in> { using type = tcp_ep_t<ip::address_v4>; };

  template<>
  struct _sockaddr_t<tcp_ep_t<ip::address_v6>> { using type = sockaddr_in6; };
  template<>
  struct _baseep_t<sockaddr_in6> { using type = tcp_ep_t<ip::address_v6>; };

  template<>
  inline ::sockaddr_in ep2sockaddr(const tcp_ep_t<ip::address_v4>& ep) {
    ::sockaddr_in ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin_family = AF_INET;
    ret.sin_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), reinterpret_cast<uint8_t*>(&ret.sin_addr));

    return ret;
  }

  template<>
  inline ::sockaddr_in6 ep2sockaddr(const tcp_ep_t<ip::address_v6>& ep) {
    ::sockaddr_in6 ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin6_family = AF_INET6;
    ret.sin6_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), ret.sin6_addr.s6_addr);

    return ret;
  }

  template<>
  inline tcp_ep_t<ip::address_v4> sockaddr2ep(const ::sockaddr_in& ep_struct) {
    tcp_ep_t<ip::address_v4> ep;
    std::copy(reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr),
              reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr) + ep.addr.size(),
              ep.addr.begin());
    ep.port = be16toh(ep_struct.sin_port);
    return ep;
  }

  template<>
  inline tcp_ep_t<ip::address_v6> sockaddr2ep(const ::sockaddr_in6& ep_struct) {
    tcp_ep_t<ip::address_v6> ep;
    std::copy(reinterpret_cast<const uint8_t*>(&ep_struct.sin6_addr),
              reinterpret_cast<const uint8_t*>(&ep_struct.sin6_addr) + ep.addr.size(),
              ep.addr.begin());
    ep.port = be16toh(ep_struct.sin6_port);
    return ep;
  }

  template<>
  constexpr int af<c3::theta::ip::address_v4>() { return AF_INET; };

  template<>
  constexpr int af<c3::theta::ip::address_v6>() { return AF_INET6; };
}

namespace c3::theta::ip {
  template<typename BaseAddr>
  class ip_tcp_server;

  template<typename BaseAddr>
  class ip_tcp_client : public theta::tcp_client<BaseAddr> {
    friend class ip_tcp_server<BaseAddr>;
  private:
    fd_wrapper fd;

  public:
    tcp_ep_t<BaseAddr> get_local_ep() override {
      return fd.get_local_ep<tcp_ep_t<BaseAddr>>();
    }
    tcp_ep_t<BaseAddr> get_remote_ep() override {
      return fd.get_remote_ep<tcp_ep_t<BaseAddr>>();
    }

  public:
    void send(nu::data_const_ref b) override {
      fd.write(b);
    }

    nu::cancellable<size_t> receive(nu::data_ref b) override {
      return fd.receive_c(b);
    }

  public:
    ip_tcp_client(fd_wrapper fd) : fd{std::move(fd)} {}

  public:
    static nu::cancellable<std::unique_ptr<tcp_client<BaseAddr>>> connect(BaseAddr remote);
  };

  template<typename BaseAddr>
  nu::cancellable<std::unique_ptr<tcp_client<BaseAddr>>> tcp_host<BaseAddr>::connect(tcp_ep_t<BaseAddr> remote) {
    nu::cancellable<std::unique_ptr<fd_wrapper>> c = fd_wrapper::connect_c<decltype(remote)>(tcp_ep_t<BaseAddr>{local_addr, 0}, remote,
                                                     af<BaseAddr>(), SOCK_STREAM, IPPROTO_TCP);
    return c.map<std::unique_ptr<tcp_client<BaseAddr>>>([](std::unique_ptr<fd_wrapper> x) {
      return std::make_unique<ip_tcp_client<BaseAddr>>(std::move(*x));
    });
  }

  template<typename BaseAddr>
  class ip_tcp_server : public tcp_server_t<BaseAddr> {
  private:
    fd_wrapper fd;

  public:
    nu::cancellable<std::unique_ptr<theta::tcp_client<BaseAddr>>> accept() override {
      auto c = fd.accept_c();
      return c.template map<std::unique_ptr<tcp_client<BaseAddr>>>([](std::unique_ptr<fd_wrapper> x) {
        return std::make_unique<ip_tcp_client<BaseAddr>>(std::move(*x));
      });
    }

    tcp_ep_t<BaseAddr> get_ep() override {
      return fd.get_local_ep<tcp_ep_t<BaseAddr>>();
    }

  public:
    template<typename... Args>
    ip_tcp_server(Args... args) : fd{std::forward<Args>(args)...} {}
  };

  template<typename BaseAddr>
  std::unique_ptr<tcp_server_t<BaseAddr>> tcp_host<BaseAddr>::listen(standard_port_t port) {
    fd_wrapper fd = ::socket(af<BaseAddr>(), SOCK_STREAM, IPPROTO_TCP);
    if (!fd.bind<tcp_ep_t<BaseAddr>>({local_addr, port}))
      throw std::runtime_error("Could not bind to requested port");
    fd.listen();
    fd.set_fcntl_flag(O_NONBLOCK);
    return std::make_unique<ip_tcp_server<BaseAddr>>(std::move(fd));
  }

  template class ip_tcp_client<address_v4>;
  template class ip_tcp_client<address_v6>;

  template class ip_tcp_server<address_v4>;
  template class ip_tcp_server<address_v6>;
}
