#include "c3/theta/channel/wrapper/reliability.hpp"
#include <c3/upsilon/csprng.hpp>

using standard_reliable_rng_t = std::mt19937_64;

namespace c3::theta::wrapper {
  reliable_id_t generate_reliable_id() {
    static thread_local std::uniform_int_distribution<reliable_id_t> dist;
    static thread_local standard_reliable_rng_t rng{dist(upsilon::csprng::standard)};

    return dist(rng);
  }
}
