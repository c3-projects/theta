#include "c3/theta/channel.hpp"

using namespace std::chrono_literals;

int main() {
  c3::theta::ipv4_ep ep{c3::theta::IPV4_LOOPBACK, 42069};

  auto client = c3::theta::tcp_client<c3::theta::ipv4_ep>::connect(ep).get_or_cancel(1000s).value();

  client->send_data(c3::nu::serialise("Hello!"));
}
