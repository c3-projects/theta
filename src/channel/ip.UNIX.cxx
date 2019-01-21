#include "c3/theta/channel/common.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>

namespace c3::theta {
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

    bool poll() {
      ::timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 1000;

      ::fd_set fds;
      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      return ::select(fd + 1, NULL, &fds, NULL, &tv) > 0;
    }

    void write(nu::data_const_ref b) {
      do {
        size_t n_written = ::write(fd, b.data(), b.size());
        b = b.subspan(n_written);
      }
      while(b.size() > 0);
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
    (new (&impl) fd_wrapper)->write(b);
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
          else if (connect_result == EINPROGRESS)
            return std::nullopt;
          else
            throw std::runtime_error("Could not connect to remote host");
        });

        if (state == nu::cancellable_state::Empty) {
          while(!provider.is_cancelled()) {
            if (fd->poll()) {
              provider.maybe_provide([&]() { return ret; });
              return;
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
    (new (&impl) fd_wrapper)->~fd_wrapper();
  }

  template<>
  nu::cancellable<size_t> tcp_client<ipv4_ep>::receive_data(nu::data_ref) {
    throw std::logic_error("Not implemented");
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
