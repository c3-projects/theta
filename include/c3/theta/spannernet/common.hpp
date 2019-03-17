#pragma once

#include <c3/nu/data.hpp>
#include <c3/nu/data/collections.hpp>
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
      auto weak_ids_b = nu::squash_seq<uint32_t>(weak_ids.begin(), weak_ids.end());
      auto weak_agreers_b = nu::squash_seq<uint32_t>(weak_agreers.begin(), weak_agreers.end());
      return nu::squash<uint32_t>(main_id, weak_ids_b, weak_agreers_b);
    }

    C3_NU_DEFINE_DESERIALISE(user, b) {
      user ret;
      nu::data weak_ids_b;
      nu::data weak_agreers_b;
      nu::expand<uint32_t>(b, ret.main_id, weak_ids_b, weak_agreers_b);
      ret.weak_ids = nu::expand_seq<upsilon::identity, uint32_t>(weak_ids_b);
      ret.weak_agreers = nu::expand_seq<upsilon::agreer, uint32_t>(weak_agreers_b);
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
