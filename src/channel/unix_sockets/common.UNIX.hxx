#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <poll.h>

#include "c3/theta/channel/ip.hpp"

namespace c3::theta {
  constexpr int socket_backlog = 32;

  template<typename BaseEp>
  using sock_ep = ep_t<BaseEp, uint16_t>;

  inline ::sockaddr_in ep2sockaddr(const sock_ep<ip::ipv4_address>& ep) {
    ::sockaddr_in ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin_family = AF_INET;
    ret.sin_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), reinterpret_cast<uint8_t*>(&ret.sin_addr));

    return ret;
  }

  inline ::sockaddr_in6 ep2sockaddr(const sock_ep<ip::ipv6_address>& ep) {
    ::sockaddr_in6 ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin6_family = AF_INET6;
    ret.sin6_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), ret.sin6_addr.s6_addr);

    return ret;
  }

  inline sock_ep<ip::ipv4_address> sockaddr2ep(const ::sockaddr_in& ep_struct) {
    sock_ep<ip::ipv4_address> ep;
    std::copy(reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr),
              reinterpret_cast<const uint8_t*>(&ep_struct.sin_addr) + ep.addr.size(),
              ep.addr.begin());
    ep.port = be16toh(ep_struct.sin_port);
    return ep;
  }

  inline sock_ep<ip::ipv6_address> sockaddr2ep(const ::sockaddr_in6& ep_struct) {
    sock_ep<ip::ipv6_address> ep;
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

    template<typename BaseAddr>
    sock_ep<BaseAddr> get_local_ep();

    template<typename BaseAddr>
    sock_ep<BaseAddr> get_remote_ep();

    template<typename BaseAddr>
    bool bind(sock_ep<BaseAddr> ep) {
      auto sa = ep2sockaddr(ep);
      return ::bind(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) == 0;
    }

    void listen() {
      ::listen(fd, socket_backlog);
    }

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

    fd_wrapper(const fd_wrapper& other) = delete;
    fd_wrapper& operator=(const fd_wrapper& other) = delete;

    fd_wrapper(fd_wrapper&& other) : fd{other.fd} { other.fd = -1; }
    fd_wrapper& operator=(fd_wrapper&& other) {
      fd = other.fd;
      other.fd = -1;
      return *this;
    }
  };

  template<>
  sock_ep<ip::ipv4_address> fd_wrapper::get_local_ep() {
    ::sockaddr_in ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getsockname(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  sock_ep<ip::ipv6_address> fd_wrapper::get_local_ep() {
    ::sockaddr_in6 ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getsockname(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  sock_ep<ip::ipv4_address> fd_wrapper::get_remote_ep() {
    ::sockaddr_in ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getpeername(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }

  template<>
  sock_ep<ip::ipv6_address> fd_wrapper::get_remote_ep() {
    ::sockaddr_in6 ep_struct;
    ::socklen_t len = sizeof(ep_struct);

    ::getpeername(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

    return sockaddr2ep(ep_struct);
  }
}
