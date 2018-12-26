#pragma once

#include <cstdint>

#include <stdexcept>


#include <vector>
namespace c3::theta {
  std::vector<int> x;

  class bit_ref {
  private:
    uint8_t* _ptr;
    size_t _offset;

  public:
    operator bool() const { return *_ptr & (1 << _offset); }
    bit_ref& operator=(bool b) {
      *_ptr = (*_ptr & ~(1 << _offset)) | (b << _offset);
      return *this;
    }

  public:
    static bit_ref from_base_offset(uint8_t* base, size_t offset);
  };

  class bit_const_ref {
  private:
    const uint8_t* _ptr;
    size_t _offset;

  public:
    operator bool() const { return *_ptr & (1 << _offset); }

  public:
    static bit_const_ref from_base_offset(const uint8_t* base, size_t offset);
  };

  class bit_iterator {
  public:
    using value_type = bit_ref;
    using difference_type = ssize_t;
    using reference = bit_ref&;
    using iterator_category = std::random_access_iterator_tag;

  private:
    uint8_t* _base;
    size_t _offset;

  public:
    inline value_type operator*() { return bit_ref::from_base_offset(_base, _offset); }
    inline bit_iterator& operator++() { _offset++; return *this; }
    inline bit_iterator& operator--() { _offset--; return *this; }

  public:
    bit_iterator(decltype(_base) base, decltype(_offset) offset) :
      _base{base}, _offset{offset} {}
  };

  class bit_const_iterator {
  public:
    using value_type = bit_const_ref;
    using difference_type = ssize_t;
    using reference = bit_const_ref&;
    using iterator_category = std::random_access_iterator_tag;

  private:
    const uint8_t* _base;
    size_t _offset;

  public:
    inline value_type operator*() { return bit_const_ref::from_base_offset(_base, _offset); }
    inline bit_const_iterator& operator++() { _offset++; return *this; }
    inline bit_const_iterator& operator--() { _offset--; return *this; }

  public:
    bit_const_iterator(decltype(_base) base, decltype(_offset) offset) :
      _base{base}, _offset{offset} {}
  };

  class bits {
  private:
    std::vector<uint8_t> _data;
    size_t _final_nbits;

  public:
    inline uint8_t* data() { return _data.data(); }
    inline const uint8_t* data() const { return _data.data(); }
    inline size_t size() const { return _data.size() + _final_nbits; }

//    uint8_t* _ptr;
//    size_t _len_bits;
//    size_t _alloc_len;

  public:
    bit_iterator begin() { return { data(), 0 }; }
    bit_const_iterator cbegin() const { return { data(), 0 }; }
    bit_const_iterator begin() const { return cbegin(); }

    bit_iterator end() { return { data(), size() }; }
    bit_const_iterator cend() const { return { data(), size() }; }
    bit_const_iterator end() const { return cend(); }

  public:

  public:
    bits() : _ptr{nullptr}, _len_bits{0} {}
    bits(size_t len) : _ptr{new uint8_t[len]}, _len_bits{len} {}
    ~bits() { delete[] _ptr; }
  };
}
