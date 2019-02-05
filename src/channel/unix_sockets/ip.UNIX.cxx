#include "c3/theta/channel/ip.hpp"

#include "common.UNIX.hxx"

namespace c3::theta::ip {
  template<typename BaseAddr>
  class ip_tcp_server;

  template<typename BaseAddr>
  class ip_tcp_client : public theta::tcp_client<BaseAddr> {
    friend class tcp_server<BaseAddr>;
  private:
    fd_wrapper fd;

  public:
    tcp_ep<BaseAddr> get_local_ep() override {
      return fd.get_local_ep<BaseAddr>();
    }
    tcp_ep<BaseAddr> get_remote_ep() override {
      return fd.get_remote_ep<BaseAddr>();
    }

  public:
    void send(nu::data_const_ref b) override {
      fd.write(b);
    }

    nu::cancellable<size_t> receive(nu::data_ref b) override {
      nu::cancellable_provider<size_t> provider;
      auto ret = provider.get_cancellable();

      std::thread {[=]() mutable {
        try {
          nu::data_ref current_pos = b;

          do {
            if (fd.poll_read()) {
              provider.maybe_update([&] {
                size_t n_read = fd.read(current_pos);
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

  public:
    template<typename... Args>
    ip_tcp_client(Args... args) : fd{std::forward<Args>(args)...} {}

  public:
    static nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> connect(BaseAddr remote);
  };

  template<typename BaseAddr>
  nu::cancellable<std::shared_ptr<tcp_client<BaseAddr>>> tcp_host<BaseAddr>::connect(tcp_ep<BaseAddr> remote) {
    nu::cancellable_provider<std::shared_ptr<tcp_client<BaseAddr>>> provider;
    auto ret = provider.get_cancellable();
    auto sa = ep2sockaddr(remote);

    std::thread {[=]() mutable {
      try {
        fd_wrapper fd{::socket(AF_INET, SOCK_STREAM, 0)};

        fd.bind<BaseAddr>({local_addr, 0});

        /* Don't overcommit */
        if (provider.is_cancelled()) return;

        provider.maybe_provide([&]() -> std::optional<std::shared_ptr<tcp_client<BaseAddr>>> {
          auto connect_result = ::connect(fd, reinterpret_cast<::sockaddr*>(&sa), sizeof(sa));

          if (connect_result == 0)
            return std::make_shared<ip_tcp_client<BaseAddr>>(std::move(fd));
          else if (errno == EINPROGRESS)
            return std::nullopt;
          else
            throw std::runtime_error("Could not connect to remote host");
        });

        while(!provider.is_decided()) {
          switch (fd.is_connected()) {
            case (connected_state::Accepted):
              provider.maybe_provide([&]() {
                return std::make_shared<ip_tcp_client<BaseAddr>>(std::move(fd));
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

  template<typename BaseAddr>
  class ip_tcp_server : public tcp_server<BaseAddr> {
  private:
    fd_wrapper fd;

  public:
    nu::cancellable<std::shared_ptr<theta::tcp_client<BaseAddr>>> accept() override {
      nu::cancellable_provider<std::shared_ptr<theta::tcp_client<BaseAddr>>> provider;

      std::thread {[=]() mutable {
        do {
          if (fd.poll_read()) {
            provider.maybe_provide([&]() -> std::optional<std::shared_ptr<tcp_client<BaseAddr>>> {
              try {
                return std::make_shared<ip_tcp_client<BaseAddr>>(::accept(fd, nullptr, 0));
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

    tcp_ep<BaseAddr> get_ep() override {
      return fd.get_local_ep<BaseAddr>();
    }

  public:
    template<typename... Args>
    ip_tcp_server(Args... args) : fd{std::forward<Args>(args)...} {}
  };

  template<typename BaseAddr>
  std::unique_ptr<tcp_server<BaseAddr>> tcp_host<BaseAddr>::listen(tcp_port_t port) {
    fd_wrapper fd = ::socket(AF_INET, SOCK_STREAM, 0);
    fd.bind<BaseAddr>({local_addr, port});
    fd.listen();
    fd.set_fcntl_flag(O_NONBLOCK);
    return std::make_unique<ip_tcp_server<BaseAddr>>(std::move(fd));
  }

  template class ip_tcp_client<ipv4_address>;
  template class ip_tcp_client<ipv6_address>;

  template class ip_tcp_server<ipv4_address>;
  template class ip_tcp_server<ipv6_address>;
}
