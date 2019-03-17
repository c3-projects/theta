#pragma once

#include <c3/upsilon/hash.hpp>
#include <c3/nu/data.hpp>
#include <c3/nu/data/collections.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta {
  template<size_t HashLen>
  class address_like : public nu::static_serialisable<address_like<HashLen>> {
  public:
    static constexpr size_t hash_len = 128 / 8;

  public:
    // We have to include this, so that a single weak hash algorithm doesn't screw everyone over
    // with generated collisions
    upsilon::hash_algorithm hash_alg;
    upsilon::hash<hash_len> value;

  private:
    static constexpr size_t _serialised_size_value = nu::total_serialised_size<decltype(hash_alg),
                                                                                    decltype(value)>();
  public:
    address_like(decltype(hash_alg) hash_alg, decltype(value) value) :
      hash_alg{std::move(hash_alg)},
      value{std::move(value)} {}

  private:
    void _serialise_static(nu::data_ref b) const override {
      nu::squash_static_unsafe(b, hash_alg, value);
    }

    C3_NU_DEFINE_STATIC_DESERIALISE(address_like<HashLen>, _serialised_size_value, b) {
      decltype(hash_alg) hash_alg;
      decltype(value) value;

      expand_static(b, hash_alg, value);

      return { std::move(hash_alg), std::move(value) };
    }
  };

  template<size_t HashLen>
  bool operator<(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    if (x.hash_alg != other.hash_alg)
      return x.hash_alg < other.hash_alg;
    else
      return x.value < other.value;
  }
  template<size_t HashLen>
  bool operator>(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    if (x.hash_alg != other.hash_alg)
      return x.hash_alg > other.hash_alg;
    else
      return x.value > other.value;
  }
  template<size_t HashLen>
  bool operator<=(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    if (x.hash_alg != other.hash_alg)
      return x.hash_alg <= other.hash_alg;
    else
      return x.value <= other.value;
  }
  template<size_t HashLen>
  bool operator>=(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    if (x.hash_alg != other.hash_alg)
      return x.hash_alg >= other.hash_alg;
    else
      return x.value >= other.value;
  }
  template<size_t HashLen>
  bool operator==(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    return x.hash_alg == other.hash_alg && x.value == other.value;
  }
  template<size_t HashLen>
  bool operator!=(const address_like<HashLen>& x, const address_like<HashLen>& other) {
    return x.hash_alg != other.hash_alg && x.value != other.value;
  }

  /// A temporary address, created by a long term name
  using address = address_like<128 / 8>;
  /// A long-term address
  using actor = address_like<256 / 8>;
}

#include <c3/nu/data/clean_helpers.hpp>
