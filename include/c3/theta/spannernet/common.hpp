#pragma once

#include <c3/nu/data.hpp>
#include <c3/nu/structs.hpp>
#include <c3/upsilon/agreement.hpp>
#include <c3/upsilon/symmetric.hpp>
#include <c3/upsilon/identity.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta::spannernet {
  class user : public nu::serialisable<user> {
  public:
    using ref = upsilon::safe_hash<16>;

  public:
    upsilon::identity main_id;
    std::vector<upsilon::identity> weak_ids;
    std::vector<upsilon::agreer> weak_agreers;

  public:
    nu::data _serialise() const override {
      return nu::squash<uint16_t>(main_id, weak_ids, weak_agreers);
    }

    C3_NU_DEFINE_DESERIALISE(user, b) {
      user ret;
      nu::expand<uint16_t>(b, ret.main_id, ret.weak_ids, ret.weak_agreers);
      return ret;
    }
  };

  class spanner : public nu::serialisable<spanner> {
  public:
    /// Should be ephemeral
    upsilon::remote_agreer src_agreer;
    /// Should be published, and acts as verification of receiver
    c3::upsilon::safe_hash<16> dst_agreer;
    upsilon::symmetric_algorithm enc_alg;
    nu::data message_enc;

  private:
    nu::data _serialise() const override {
      return nu::squash<uint16_t>(src_agreer, dst_agreer, enc_alg, message_enc);
    }

    C3_NU_DEFINE_DESERIALISE(spanner, b) {
      spanner ret;
      nu::expand<uint16_t>(b, ret.src_agreer, ret.dst_agreer, ret.enc_alg, ret.message_enc);
      return ret;
    }
  };

  class message : public nu::serialisable<message> {
  public:
    /// Acts as verification of sender
    upsilon::identity sender;
    nu::data signature;
    nu::data payload;

  private:
    nu::data _serialise() const override {
      return nu::squash<uint16_t>(sender, signature, payload);
    }

    C3_NU_DEFINE_DESERIALISE(message, b) {
      message ret;
      nu::expand<uint16_t>(b, ret.sender, ret.signature, ret.payload);
      return ret;
    }
  };
}

#include <c3/nu/data/clean_helpers.hpp>
