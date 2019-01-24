#pragma once

#include <set>

#include <c3/nu/integral.hpp>
#include <c3/nu/concurrency/concurrent_queue.hpp>

#include <atomic>

namespace c3::theta::wrapper {
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
}
