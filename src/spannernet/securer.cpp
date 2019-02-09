#include "c3/theta/spannernet/common.hpp"

#include <c3/upsilon/hash.hpp>
#include <c3/upsilon/symmetric.hpp>
#include <c3/upsilon/kdf.hpp>
#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/identity.hpp>
#include <c3/upsilon/csprng.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::spannernet {
  /*
  void open_securer::_pump_body() {
    nu::mutexed<std::shared_ptr<nu::cancellable<nu::data>>> current_cancellable = nullptr;
    std::thread t([&](){
      while (!_stop_pump) {
        std::shared_ptr<nu::cancellable<nu::data>> recv_c;
        {
          auto handle = current_cancellable.get_handle();
          recv_c = std::make_shared<nu::cancellable<nu::data>>(base->receive());
          *handle = recv_c;
        }
        recv_c->take_on_complete([this](auto b) {
          message m = nu::deserialise<message>(b);
          auto id = m.supposed_src_id;
          pb.post(id, std::move(m));
        });
        **current_cancellable = nullptr;
        recv_c->wait_final();
      }
    });

    _stop_pump.wait_for_open();
    auto handle = current_cancellable.get_handle();
    if (*handle)
      (*handle)->finalise();
    t.join();
  }

  open_securer::open_securer(std::unique_ptr<channel>&& _base,
                             upsilon::agreer local_id,
                             upsilon::symmetric_algorithm enc_alg) :
      base{std::move(_base)} {

  }
  */
}

#include <c3/nu/data/clean_helpers.hpp>
