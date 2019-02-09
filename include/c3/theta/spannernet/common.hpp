#pragma once

#include <c3/nu/data.hpp>
#include <c3/nu/structs.hpp>
#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/symmetric.hpp>
#include <c3/upsilon/identity.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::spannernet {
  class spanner : public nu::serialisable<spanner> {
  public:
    /// Should be ephemeral
    upsilon::remote_agreer src_agreer;
    /// Should be published, and acts as verification of receiver
    c3::upsilon::safe_hash<16> dst_agreer;
    upsilon::symmetric_algorithm enc_alg;
    nu::data message_enc;

  public:
    nu::data _serialise() const override {
      return nu::squash_dynamic<uint16_t>(src_agreer, dst_agreer, enc_alg);
    }

    C3_NU_DEFINE_DESERIALISE(spanner, b) {
      spanner ret;
      nu::expand_dynamic(b, ret.src_agreer, ret.dst_agreer, ret.enc_alg)
    }
  };

  class message : public nu::serialisable<message> {
    /// Acts as verification of sender
    upsilon::identity sender;
    nu::data signature;
    nu::data message;
  };
}

#include <c3/nu/data/clean_helpers.hpp>
