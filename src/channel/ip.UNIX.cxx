#include "c3/theta/channel/common.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>
#include <poll.h>

namespace c3::theta {
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
      pollfd pfd;
      pfd.fd = fd;
      pfd.events = POLLIN;

      // Poll for 1 millisecond
      return ::poll(&pfd, 1, 1) != 0;
    }

  public:
    operator decltype(fd)() { return fd; }

  public:
    fd_wrapper() : fd{-1} {}
    fd_wrapper(int fd) : fd{fd} {
      if (fd == -1)
        throw std::runtime_error("Failed to open socket");
    }
    ~fd_wrapper() { ::close(fd); }
  };

  static_assert(sizeof(void*) >= sizeof(fd_wrapper));

  inline sockaddr_in create_sockaddr(const ipv4_ep& ep) {
    sockaddr_in ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin_family = AF_INET;
    ret.sin_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), reinterpret_cast<uint8_t*>(&ret.sin_addr));

    return ret;
  }

  inline sockaddr_in6 create_sockaddr(const ipv6_ep& ep) {
    sockaddr_in6 ret;

    memset(&ret, 0, sizeof(ret));

    ret.sin6_family = AF_INET6;
    ret.sin6_port = htobe16(ep.port);
    std::copy(ep.addr.begin(), ep.addr.end(), ret.sin6_addr.s6_addr);

    return ret;
  }

  template<>
  void tcp_client<ipv4_ep>::send_data(nu::data_const_ref b) {
    reinterpret_cast<fd_wrapper*>(&impl)->write(b);
  }

  template<>
  nu::cancellable<std::shared_ptr<tcp_client<ipv4_ep>>> tcp_client<ipv4_ep>::connect(ipv4_ep remote) {
    nu::cancellable_provider<std::shared_ptr<tcp_client<ipv4_ep>>> provider;
    auto ret = provider.get_cancellable();

    std::thread {[=]() mutable {
      try {
        auto ret = std::make_shared<tcp_client<ipv4_ep>>();

        fd_wrapper* fd = (new (&ret->impl) fd_wrapper{::socket(AF_INET, SOCK_STREAM, 0)});

        fd->set_fcntl_flag(O_NONBLOCK);

        auto sa = create_sockaddr(remote);

        /// Don't overcommit
        if (provider.is_cancelled()) return;

        auto state = provider.maybe_provide([&]() -> std::optional<std::shared_ptr<tcp_client<ipv4_ep>>> {
          auto connect_result = ::connect(fd->fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa));

          if (connect_result == 0)
            return ret;
          else if (errno == EINPROGRESS)
            return std::nullopt;
          else
            throw std::runtime_error("Could not connect to remote host");
        });

        if (state == nu::cancellable_state::Undecided) {
          while(!provider.is_cancelled()) {
            switch (fd->is_connected()) {
              case (connected_state::Accepted):
                provider.maybe_provide([&]() { return ret; });
                return;
              case (connected_state::Refused):
                provider.cancel();
                return;
              default:
                continue;
            }
          }
        }
      }
      catch (...) {}

      provider.cancel();
    }}.detach();

    return ret;
  }

  template<>
  tcp_client<ipv4_ep>::~tcp_client() {
    reinterpret_cast<fd_wrapper*>(&impl)->~fd_wrapper();
  }

  template<>
  nu::cancellable<size_t> tcp_client<ipv4_ep>::receive_data(nu::data_ref b) {
    nu::cancellable_provider<size_t> provider;

    auto ret = provider.get_cancellable();

    std::thread {[=]() mutable {
      try {
        auto fd = reinterpret_cast<fd_wrapper*>(&impl);
        nu::data_ref current_pos = b;

        do {
          if (fd->poll_read()) {
            provider.maybe_update([&] {
              size_t n_read = fd->read(current_pos);
              current_pos = current_pos.subspan(static_cast<ssize_t>(n_read));
              return b.size() - current_pos.size();
            });
          }
        }
        while (!provider.is_cancelled() && current_pos.size() > 0);
      }
      catch (...) {}
      provider.cancel();
    }}.detach();

    return ret;
  }
  /*
        fd.set_fcntl_flag(O_NONBLOCK);

        auto sa = create_sockaddr(ep);

        auto connect_result = ;

        if (connect_result != EINPROGRESS)
          throw std::runtime_error("Could not connect to remote host");

        if (!fd.wait_timeout(timeout))
          throw std::runtime_error("Connection timed out");
      }
  std::unique_ptr<stream_channel> tcp_client(ipv6_ep ep, nu::timeout_t timeout) {
    class impl : public stream_channel {
    public:
      fd_wrapper fd;

    public:
      void send_frame(nu::data_const_ref b) {
        ::write(fd, b.data(), b.size());
      }

      size_t receive_frame(nu::data_ref b) {
        return ::read(fd, b.data(), b.size());
      }

    public:
      impl(decltype(ep) ep,  nu::timeout_t timeout) : fd{} {
        fd.set_fcntl_flag(O_NONBLOCK);

        auto sa = create_sockaddr(ep);

        auto connect_result = ::connect(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa));

        if (connect_result != EINPROGRESS)
          throw std::runtime_error("Could not connect to remote host");

        if (!fd.wait_timeout(timeout))
          throw std::runtime_error("Connection timed out");
      }
    };

    return std::make_unique<impl>(ep, timeout);
  }
  */
}
