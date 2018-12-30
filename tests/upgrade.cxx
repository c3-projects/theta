#include "c3/theta/channel/common.hpp"
#include "c3/theta/channel/wrapper/upgrader.hpp"
#include "c3/theta/channel/wrapper/reliability.hpp"

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

using namespace c3;
using namespace c3::theta;


int main() {
  auto mp = fake_medium<1>();

  auto l0 = wrapper::link2channel(wrapper::make_reliable<3>(wrapper::medium2link(std::move(mp.first ))));
  auto l1 = wrapper::link2channel(wrapper::make_reliable<3>(wrapper::medium2link(std::move(mp.second))));

  upsilon::data tx_msg = upsilon::serialise("Hello, world!");
  l0->transmit_msg(tx_msg);

  upsilon::data rx_msg = l1->receive_msg();
  std::string rx_str = upsilon::deserialise<std::string>(rx_msg);

  if (rx_msg != tx_msg)
    throw std::runtime_error("Message corrupted");
}
