#pragma once

#include <set>

#include <c3/nu/integer.hpp>
#include <c3/nu/concurrency/concurrent_queue.hpp>

#include <atomic>

namespace c3::theta {
  using standard_port_t = uint16_t;

  template<typename BaseEp, typename PortType = standard_port_t>
  struct ep_t {
    using address_t = BaseEp;
    using port_t = PortType;

  public:
    BaseEp addr;
    PortType port;
  };

  template<typename BaseEp, typename PortType>
  inline bool operator==(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b) {
    return a.port == b.port && a.addr == b.addr;
  }
  template<typename BaseEp, typename PortType>
  inline bool operator!=(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b) {
    return !(a == b);
  }
  template<typename BaseEp, typename PortType>
  inline bool operator<(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b) {
    if (a.port != b.port)
      return a.port < b.port;
    else
      return a.addr < b.addr;
  }
  template<typename BaseEp, typename PortType>
  inline bool operator>(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b) {
    if (a.port != b.port)
      return a.port > b.port;
    else
      return a.addr > b.addr;
  }
  template<typename BaseEp, typename PortType>
  inline bool operator<=(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b){
    return !(a > b);
  }
  template<typename BaseEp, typename PortType>
  inline bool operator>=(const ep_t<BaseEp, PortType>& a, const ep_t<BaseEp, PortType>& b) {
    return !(a < b);
  }

  template<typename Port>
  class port_controller {
  private:
    nu::mutexed<std::set<Port>> in_use_ports;
    // A queue won't work as someone may reserve a dynamic port
    Port dyn_min;
    Port dyn_max;

  public:
    bool reserve(Port port) {
      return (*in_use_ports)->insert(port).second;
    }

    Port request_dynamic_port() {
      // We lose 1 port this way
      // _oh no_
      for (Port port = dyn_min; port < dyn_max; ++port) {
        // Try to insert, and check if we succeded
        if (reserve(port))
          return port;
      }

      throw std::runtime_error("Dynamic port space exhausted");
    }

    void unreserve(Port port) {
      (*in_use_ports)->erase(port);
    }
  };

  class port_error : public std::exception {
  public:
    std::string msg;

  public:
    inline const char* what() const noexcept override { return msg.data(); }

  public:
    inline port_error(decltype(msg) msg) : msg{std::move(msg)} {}
  };
}
