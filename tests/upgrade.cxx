#include "c3/theta/channel/common.hpp"
#include "c3/theta/channel/wrapper/upgrader.hpp"
#include "c3/theta/channel/wrapper/reliability.hpp"

#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

using namespace c3;
using namespace c3::theta;


int main() {
//  auto l0 = wrapper::link2channel(wrapper::make_reliable(wrapper::medium2link(std::move(mp.first ))));
//  auto l1 = wrapper::link2channel(wrapper::make_reliable(wrapper::medium2link(std::move(mp.second))));
//  auto l0 = wrapper::link2channel(wrapper::medium2link(std::move(mp.first )));
//  auto l1 = wrapper::link2channel(wrapper::medium2link(std::move(mp.second)));

<<<<<<< Updated upstream
  auto l0 = wrapper::link2channel(wrapper::make_reliable<3>(wrapper::medium2link(std::move(mp.first ))));
  auto l1 = wrapper::link2channel(wrapper::make_reliable<3>(wrapper::medium2link(std::move(mp.second))));

  nu::data tx_msg = nu::serialise("Hello, world!");
  l0->transmit_msg(tx_msg);

  nu::data rx_msg = l1->receive_msg();
  std::string rx_str = nu::deserialise<std::string>(rx_msg);
=======
//  {
//    auto mp = fake_medium<1>();

//    auto l0 = wrapper::medium2link(std::move(mp.first ));
//    auto l1 = wrapper::medium2link(std::move(mp.second));

//    nu::data tx_msg = nu::serialise("Hello, world!");
//    l0->send_frame(tx_msg);
>>>>>>> Stashed changes

//    nu::data rx_msg = l1->receive_frame().get_or_cancel(1s).value();
//    std::string rx_str = nu::deserialise<std::string>(rx_msg);

//    if (rx_msg != tx_msg)
//      throw std::runtime_error("medium2link message corrupted");
//  }

//  {
//    auto mp = fake_medium<1>();

//    auto l0 = wrapper::link2channel(wrapper::medium2link(std::move(mp.first )));
//    auto l1 = wrapper::link2channel(wrapper::medium2link(std::move(mp.second)));

//    nu::data tx_msg = nu::serialise("Hello, world!");
//    l0->send_msg(tx_msg);

//    nu::data rx_msg = l1->receive_msg().get_or_cancel(1000s).value();
//    std::string rx_str = nu::deserialise<std::string>(rx_msg);

//    if (rx_msg != tx_msg)
//      throw std::runtime_error("link2channel message corrupted");
//  }

  {
    auto mp = fake_medium<1>();

    auto l0 = wrapper::link2channel(wrapper::make_reliable(wrapper::medium2link(std::move(mp.first ))));
    auto l1 = wrapper::link2channel(wrapper::make_reliable(wrapper::medium2link(std::move(mp.second))));

    nu::data tx_msg = nu::serialise("Hello, world!");
    l0->send_msg(tx_msg);

    nu::data rx_msg = l1->receive_msg().wait().value();
    std::string rx_str = nu::deserialise<std::string>(rx_msg);

    if (rx_msg != tx_msg)
      throw std::runtime_error("reliable message corrupted");
  }
}
