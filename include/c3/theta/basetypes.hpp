#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <array>

#include <c3/nu/data.hpp>

#include <c3/nu/data/helpers.hpp>

namespace c3::theta {
  class date : nu::serialisable<date> {
  private:
    uint64_t milliseconds;

  public:
    constexpr date(uint64_t milliseconds) noexcept : milliseconds{milliseconds} {}

    C3_NU_DEFER_SERIALISATION_VAR(date, milliseconds);

  public:
    bool operator> (const date& other) const { return milliseconds >  other.milliseconds; }
    bool operator< (const date& other) const { return milliseconds <  other.milliseconds; }
    bool operator>=(const date& other) const { return milliseconds >= other.milliseconds; }
    bool operator<=(const date& other) const { return milliseconds <= other.milliseconds; }
    bool operator==(const date& other) const { return milliseconds == other.milliseconds; }
    bool operator!=(const date& other) const { return milliseconds != other.milliseconds; }
  };
}

#include <c3/nu/data/helpers.hpp>
