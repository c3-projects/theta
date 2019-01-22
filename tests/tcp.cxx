#include "c3/theta/channel.hpp"

#include <iostream>

using namespace std::chrono_literals;

int main() {
  c3::theta::ipv4_ep ep = {c3::theta::IPV4_LOOPBACK, 42069};

  auto client = c3::theta::tcp_client<c3::theta::ipv4_ep>::connect(ep).get_or_cancel(1000s).value();

  client->send_data(c3::nu::serialise("foo"));

  c3::nu::static_data<1<<20> message_buf;
  auto len = client->receive_data(message_buf).get_or_cancel(10s);

  c3::nu::data_ref message{message_buf.data(), static_cast<ssize_t>(len.value())};

  std::cout << c3::nu::deserialise<std::string>(message) << std::endl;
}
