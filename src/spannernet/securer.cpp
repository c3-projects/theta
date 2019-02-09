#include "c3/theta/spannernet/securer.hpp"

#include <c3/upsilon/hash.hpp>
#include <c3/upsilon/symmetric.hpp>
#include <c3/upsilon/kdf.hpp>
#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/identity.hpp>
#include <c3/upsilon/csprng.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::spannernet {
  class packet_header : public nu::serialisable<packet_header> {
  public:
    upsilon::safe_hash<id_hash_len> src_id;
    upsilon::safe_hash<id_hash_len> dst_id;
    upsilon::symmetric_algorithm alg;
    nu::data nonce;

  private:
    nu::data _serialise() const override {
      return nu::squash_hybrid(src_id, dst_id, alg, nonce);
    }

    C3_NU_DEFINE_DESERIALISE(packet_header, b) {
      packet_header ret;
      nu::expand_hybrid(b, ret.src_id, ret.dst_id, ret.alg, ret.nonce);
      return ret;
    }
  };

  decltype(packet_header::nonce) gen_nonce() {
    static thread_local std::uniform_int_distribution<decltype(packet_header::nonce)> dist;
    return dist(upsilon::csprng::standard);
  }

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
}

#include <c3/nu/data/clean_helpers.hpp>
