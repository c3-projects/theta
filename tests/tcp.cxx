#include "c3/theta/channel/ip.hpp"

#include <iostream>

using namespace std::chrono_literals;

int main() {
  c3::theta::ip::ipv4_ep ep = {c3::theta::ip::IPV4_LOOPBACK, c3::theta::ip::PORT_ANY};

  auto server = c3::theta::ip::tcp_server(ep);
  ep = server.get_ep();

  auto client_c = c3::theta::ip::tcp_client<c3::theta::ip::ipv4_ep>::connect(ep);
  auto server_client_c = server.accept();

  auto client = client_c.get_or_cancel(1000s).value();
  auto server_client = server_client_c.get_or_cancel(1000s).value();

  {
    auto client_local_ep = client->get_local_ep();
    auto client_remote_ep = client->get_remote_ep();
    auto server_client_local_ep = server_client->get_local_ep();
    auto server_client_remote_ep = server_client->get_remote_ep();
    if (ep != client_remote_ep) {
      throw std::runtime_error("ep != client_remote_ep");
    }
    if (ep != server_client_local_ep) {
      throw std::runtime_error("ep != server_client_local_ep");
    }
    if (server_client_local_ep != client_remote_ep) {
      throw std::runtime_error("server_client_local_ep != client_remote_ep");
    }
    if (client_local_ep != server_client_remote_ep) {
      throw std::runtime_error("client_local_ep != server_client_remote_ep");
    }
  }

  {
    client->send_data(c3::nu::serialise<int>(69));

    c3::nu::static_buffer<int> server_recv;

    if (server_client->receive_data(server_recv).wait().value() != server_recv.size())
      throw std::runtime_error("data length corrupted on transport");

    if (c3::nu::deserialise<int>(server_recv) != 69)
      throw std::runtime_error("data corrupted on transport");
  }

  {
    server_client->send_data(c3::nu::serialise<int>(420));

    c3::nu::static_buffer<int> client_recv;

    if (client->receive_data(client_recv).wait().value() != client_recv.size())
      throw std::runtime_error("data length corrupted on transport");

    if (c3::nu::deserialise<int>(client_recv) != 420)
      throw std::runtime_error("data corrupted on transport");
  }
}
