#pragma once

#include <memory>
#include <c3/nu/concurrency/cancellable.hpp>

#include "c3/theta/channel/ports.hpp"

namespace c3::theta {
  template<typename Address, typename Port = standard_port_t>
  class client {
  public:
    virtual ep_t<Address, Port> get_local_ep() = 0;
    virtual ep_t<Address, Port> get_remote_ep() = 0;

  public:
    virtual ~client() = default;
  };

  template<typename Client, typename Address, typename Port = standard_port_t>
  class server {
    static_assert(std::is_base_of_v<client<Address, Port>, Client>);

  public:
    virtual nu::cancellable<std::unique_ptr<Client>> accept() = 0;
    virtual ep_t<Address, Port> get_ep() = 0;

  public:
    virtual ~server() = default;
  };

  template<typename Client, typename Address, typename Port = standard_port_t>
  class host {
    static_assert(std::is_base_of_v<client<Address, Port>, Client>);

  public:
    virtual Address local_address() const = 0;
    virtual nu::cancellable<std::unique_ptr<Client>> connect(ep_t<Address, Port> remote) = 0;
    virtual std::unique_ptr<server<Client, Address, Port>> listen(Port) = 0;

  public:
    virtual ~host() = default;
  };
}
