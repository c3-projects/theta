#include "c3/theta/channel/ip.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <poll.h>

namespace c3::theta::ip {
  constexpr int socket_backlog = 32;

  inline ::sockaddr_in ep2sockaddr(const ipv4_ep& ep) {
    ::sockaddr_in ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin_family = AF_INET;
    ret.sin_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), reinterpret_cast<uint8_t*>(&ret.sin_addr));

    return ret;
  }

  inline ::sockaddr_in6 ep2sockaddr(const ipv6_ep& ep) {
    ::sockaddr_in6 ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin6_family = AF_INET6;
    ret.sin6_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), ret.sin6_addr.s6_addr);

    return ret;
  }

  inline ipv4_ep sockaddr2ep(const ::sockaddr_in& ep_struct) {
    ipv4_ep ep;
    std::copy(reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr),
              reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr) + ep.addr.size(),
              ep.addr.begin());
    ep.port = be16toh(ep_struct.sin_port);
    return ep;
  }

  inline ipv6_ep sockaddr2ep(const ::sockaddr_in6& ep_struct) {
    ipv6_ep ep;
    std::copy(reinterpret_cast<const uint8_t*>(&ep_struct.sin6_addr),
              reinterpret_cast<const uint8_t*>(&ep_struct.sin6_addr) + ep.addr.size(),
              ep.addr.begin());
    ep.port = be16toh(ep_struct.sin6_port);
    return ep;
  }

  enum class connected_state {
    Waiting,
    Accepted,
    Refused
  };

  class fd_wrapper {
  public:
    int fd;

  public:
    void set_fcntl_flag(long flag) {
      long arg = ::fcntl(fd, F_GETFL, NULL);
      if (arg < 0)
        throw std::runtime_error("Could not get socket fcntl");

      arg |= flag;

      ::fcntl(fd, F_SETFL, arg);
    }

    void unset_fcntl_flag(long flag) {
      long arg = ::fcntl(fd, F_GETFL, NULL);
      if (arg < 0)
        throw std::runtime_error("Could not get socket fcntl");

      arg &= ~flag;

      ::fcntl(fd, F_SETFL, arg);
    }

    connected_state is_connected() noexcept {
      pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLOUT;

      if (::poll(&pfd, 1, 1))
        return connected_state::Accepted;

      int result;
      socklen_t result_len = sizeof(result);

      if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &result, &result_len) == 0 && result != ECONNREFUSED)
        return connected_state::Waiting;
      else
        return connected_state::Refused;
    }

    void write(nu::data_const_ref b) {
      do {
        ssize_t n_written = ::write(fd, b.data(), static_cast<size_t>(b.size()));
        if (n_written < 0)
          throw std::runtime_error("Could not write to socket");
        b = b.subspan(n_written);
      }
      while(b.size() > 0);
    }

    size_t read(nu::data_ref b) {
      ssize_t n_read = ::read(fd, b.data(), static_cast<size_t>(b.size()));

      if (n_read >= 0)
        return static_cast<size_t>(n_read);
      if (errno == EAGAIN)
        return 0;
      else
        throw std::runtime_error("Could not write to socket");
    }

    bool poll_read() {
      ::pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLIN;

      // Poll for 1 millisecond
      return ::poll(&pfd, 1, 1) != 0;
    }

    template<typename EpType>
    EpType get_local_ep();

    template<typename EpType>
    EpType get_remote_ep();

  public:
    operator decltype(fd)() { return fd; }

  public:
    fd_wrapper() : fd{-1} {}
    fd_wrapper(int fd) : fd{fd} {
      if (fd == -1)
        throw std::runtime_error("Failed to open socket");
    }
    ~fd_wrapper() {
      // Sanity check against nullptr
      if (fd >= 2 )
        ::close(fd);
    }
  };

  static_assert(sizeof(void*) >= sizeof(fd_wrapper));

  template<>
  ipv4_ep fd_wrapper::get_local_ep() {
    ::sockaddr_in ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getsockname(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  ipv6_ep fd_wrapper::get_local_ep() {
    ::sockaddr_in6 ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getsockname(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  ipv4_ep fd_wrapper::get_remote_ep() {
    ::sockaddr_in ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getpeername(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  ipv6_ep fd_wrapper::get_remote_ep() {
    ::sockaddr_in6 ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getpeername(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

#define C3_THETA_IMPL_TCP_IP(EP) \
  template<> \
  void tcp_client<EP>::send_data(nu::data_const_ref b) { \
    reinterpret_cast<fd_wrapper*>(&impl)->write(b); \
  } \
\
  template<> \
  nu::cancellable<std::shared_ptr<tcp_client<EP>>> tcp_client<EP>::connect(EP remote) { \
    nu::cancellable_provider<std::shared_ptr<tcp_client<EP>>> provider; \
    auto ret = provider.get_cancellable(); \
\
    std::thread {[=]() mutable { \
      try { \
        auto ret = std::make_shared<tcp_client<EP>>(); \
\
        fd_wrapper* fd = (new (&ret->impl) fd_wrapper{::socket(AF_INET, SOCK_STREAM, 0)}); \
\
        fd->set_fcntl_flag(O_NONBLOCK); \
\
        auto sa = ep2sockaddr(remote); \
\
        /* Don't overcommit */ \
        if (provider.is_cancelled()) return; \
\
        auto state = provider.maybe_provide([&]() -> std::optional<std::shared_ptr<tcp_client<EP>>> { \
          auto connect_result = ::connect(fd->fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)); \
\
          if (connect_result == 0) \
            return ret; \
          else if (errno == EINPROGRESS) \
            return std::nullopt; \
          else \
            throw std::runtime_error("Could not connect to remote host"); \
        }); \
\
        if (state == nu::cancellable_state::Undecided) { \
          while(!provider.is_cancelled()) { \
            switch (fd->is_connected()) { \
              case (connected_state::Accepted): \
                provider.maybe_provide([&]() { return ret; }); \
                return; \
              case (connected_state::Refused): \
                provider.cancel(); \
                return; \
              default: \
                continue; \
            } \
          } \
        } \
      } \
      catch (...) {} \
\
      provider.cancel(); \
    }}.detach(); \
\
    return ret; \
  } \
\
  template<> \
  tcp_client<EP>::~tcp_client() { \
    reinterpret_cast<fd_wrapper*>(&impl)->~fd_wrapper(); \
  } \
\
  template<> \
  nu::cancellable<size_t> tcp_client<EP>::receive_data(nu::data_ref b) { \
    nu::cancellable_provider<size_t> provider; \
    auto ret = provider.get_cancellable(); \
\
    std::thread {[=]() mutable { \
      try { \
        auto fd = reinterpret_cast<fd_wrapper*>(&impl); \
        nu::data_ref current_pos = b; \
\
        do { \
          if (fd->poll_read()) { \
            provider.maybe_update([&] { \
              size_t n_read = fd->read(current_pos); \
              current_pos = current_pos.subspan(static_cast<ssize_t>(n_read)); \
              return b.size() - current_pos.size(); \
            }); \
          } \
        } \
        while (current_pos.size() > 0 && provider.get_state() == nu::cancellable_state::Undecided); \
        provider.maybe_provide([&] { return b.size(); }); \
      } \
      catch (...) {} \
      provider.cancel(); \
    }}.detach(); \
    return ret; \
  } \
  template<> \
  tcp_server<EP>::~tcp_server() { reinterpret_cast<fd_wrapper*>(&impl)->~fd_wrapper(); } \
  template<> \
  bool tcp_server<EP>::bind(EP ep) { \
    auto fd = reinterpret_cast<fd_wrapper*>(&impl); \
    auto sa = ep2sockaddr(ep); \
    if (::bind(fd->fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) == 0) { \
      ::listen(fd->fd, socket_backlog); \
      return true; \
    } \
    else return false; \
  } \
\
  template<> \
  tcp_server<EP>::tcp_server() : impl{} { \
    new (&impl) fd_wrapper{::socket(AF_INET, SOCK_STREAM, 0)}; \
  } \
  template<> \
  tcp_server<EP>::tcp_server(EP bind_addr) : tcp_server{} { \
    bind(bind_addr); \
  } \
  template<> \
  nu::cancellable<std::shared_ptr<theta::tcp_client<EP>>> tcp_server<EP>::accept() { \
    nu::cancellable_provider<std::shared_ptr<theta::tcp_client<EP>>> provider; \
\
    std::thread {[=]() mutable { \
      auto fd = reinterpret_cast<fd_wrapper*>(&impl); \
\
      do { \
        if (fd->poll_read()) { \
          provider.maybe_provide([&]() -> std::optional<std::shared_ptr<tcp_client<EP>>> { \
            try { \
              auto ret = std::make_shared<tcp_client<EP>>(nullptr); \
              new(&ret->impl) fd_wrapper{::accept(fd->fd, nullptr, 0)}; \
              return ret; \
            } \
            catch(...) {} \
\
            return std::nullopt; \
          }); \
        } \
      } \
      while (provider.get_state() == nu::cancellable_state::Undecided); \
    }}.detach(); \
\
    return provider.get_cancellable(); \
  } \
  template<> \
  EP tcp_server<EP>::get_ep() { \
    return reinterpret_cast<fd_wrapper*>(&impl)->get_local_ep<EP>(); \
  } \
  template<> \
  EP tcp_client<EP>::get_local_ep() { \
    return reinterpret_cast<fd_wrapper*>(&impl)->get_local_ep<EP>(); \
  } \
  template<> \
  EP tcp_client<EP>::get_remote_ep() { \
    return reinterpret_cast<fd_wrapper*>(&impl)->get_remote_ep<EP>(); \
  }

  C3_THETA_IMPL_TCP_IP(ipv4_ep);
  C3_THETA_IMPL_TCP_IP(ipv6_ep);



}
