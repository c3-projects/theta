#pragma once

#include "c3/theta/channel/base.hpp"

namespace c3::theta::wrapper {
  class hamming : public universal_wrapper {
  public:
    size_t message_bits;
    size_t error_bits;

  public:
    void translate_transmit(medium&, upsilon::data_const_ref) override;
    void translate_receive(medium&, upsilon::data_ref) override;

    void translate_transmit_msg(channel&, upsilon::data_const_ref) override;
    upsilon::data translate_receive_msg(channel&) override;

  public:
    hamming(size_t message_bits, size_t error_bits) :
      message_bits{message_bits}, error_bits{error_bits} {}
  };

  //std::unique_ptr<channel> reed_solomon_bch(wrappee_channel, size_t message_bits, size_t error_bits);
}
