#include <c3/theta/channel/tcp.hpp>

namespace c3::theta {
  nu::data tcp_header::_serialise() const {
    nu::data ret = nu::serialise(base);
    auto opt_b = nu::serialise(options);
    ret.insert(opt_b.begin(), opt_b.end());
    return ret;
  }
}