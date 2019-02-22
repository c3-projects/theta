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

  inline ::sockaddr_in ep2sockaddr(const ep_t<ip::address_v4>& ep) {
    ::sockaddr_in ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin_family = AF_INET;
    ret.sin_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), reinterpret_cast<uint8_t*>(&ret.sin_addr));

    return ret;
  }

  template<typename BaseEp>
  struct _sockaddr_t;

  template<typename BaseEp>
  using sockaddr_t = typename _sockaddr_t<BaseEp>::type;

  template<typename SockAddr>
  struct _baseep_t;

  template<typename SockAddr>
  using baseep_t = typename _baseep_t<SockAddr>::type;

  template<typename BaseEp>
  inline sockaddr_t<BaseEp> ep2sockaddr(const BaseEp&);

  template<typename SockAddr>
  inline baseep_t<SockAddr> sockaddr2ep(const SockAddr&);

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
      ::pollfd pfd;
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

      // Poll for 100 milliseconds
      return ::poll(&pfd, 1, 100) != 0;
    }

    template<typename BaseEp>
    BaseEp get_local_ep() {
      sockaddr_t<BaseEp> ep_struct;
      ::socklen_t len = sizeof(ep_struct);

      ::getsockname(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

      return sockaddr2ep(ep_struct);
    }

    template<typename BaseEp>
    BaseEp get_remote_ep() {
      sockaddr_t<BaseEp> ep_struct;
      ::socklen_t len = sizeof(ep_struct);

      ::getpeername(fd, reinterpret_cast<sockaddr*>(&ep_struct), &len);

      return sockaddr2ep(ep_struct);
    }

    template<typename BaseEp>
    bool bind(BaseEp ep) {
      auto sa = ep2sockaddr(ep);
      return ::bind(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) == 0;
    }

    template<typename BaseEp>
    bool connect(BaseEp ep) {
      auto sa = ep2sockaddr(ep);
      return ::connect(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa)) == 0;
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

  public:
    nu::cancellable<size_t> receive_c(nu::data_ref b) {
      nu::cancellable_provider<size_t> provider;
      auto ret = provider.get_cancellable();

      std::thread {[=]() mutable {
        try {
          nu::data_ref current_pos = b;

          do {
            if (poll_read()) {
              provider.maybe_update([&] {
                size_t n_read = read(current_pos);
                current_pos = current_pos.subspan(static_cast<ssize_t>(n_read));
                return b.size() - current_pos.size();
              });
            }
          }
          while (current_pos.size() > 0 && provider.get_state() == nu::cancellable_state::Undecided);
          provider.maybe_provide([&] { return b.size(); });
        }
        catch (...) {}
        provider.cancel();
      }}.detach();
      return ret;
    }

    template<typename Ep>
    static nu::cancellable<std::unique_ptr<fd_wrapper>> connect_c(Ep bind_ep, Ep remote_ep,
                                                                  int domain, int type, int protocol) {
      nu::cancellable_provider<std::unique_ptr<fd_wrapper>> provider;
      auto ret = provider.get_cancellable();

      std::thread {[=, bind_ep{std::move(bind_ep)}, remote_ep{std::move(remote_ep)}]() mutable {
        try {
          fd_wrapper fd{::socket(domain, type, protocol)};

          fd.bind(bind_ep);

          provider.maybe_provide([&]() -> std::optional<std::unique_ptr<fd_wrapper>> {
            if (fd.connect(remote_ep))
              return std::make_unique<fd_wrapper>(std::move(fd));
            else if (errno == EINPROGRESS)
              return std::nullopt;
            else
              throw std::runtime_error("Could not connect to remote host");
          });

          while(!provider.is_decided()) {
            switch (fd.is_connected()) {
              case (connected_state::Accepted):
                provider.maybe_provide([&]() {
                  return std::make_unique<fd_wrapper>(std::move(fd));
                });
                return;
              case (connected_state::Refused):
                provider.cancel();
                return;
              default:
                continue;
            }
          }
        }
        catch (...) {}

        provider.cancel();
      }}.detach();

      return ret;
    }

    nu::cancellable<std::unique_ptr<fd_wrapper>> accept_c() {
      nu::cancellable_provider<std::unique_ptr<fd_wrapper>> provider;

      std::thread {[=]() mutable {
        do {
          if (poll_read()) {
            provider.maybe_provide([&]() -> std::optional<std::unique_ptr<fd_wrapper>> {
              // A single dodgy client shouldn't cancel this
              try {
                return std::make_unique<fd_wrapper>(::accept(fd, nullptr, 0));
              }
              catch(...) {}

              return std::nullopt;
            });
          }
        }
        while (provider.get_state() == nu::cancellable_state::Undecided);
      }}.detach();

      return provider.get_cancellable();
    }
  };

  template<typename BaseAddr>
  constexpr int af();
}
