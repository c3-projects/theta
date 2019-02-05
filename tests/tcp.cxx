#include "c3/theta/channel/fake.hpp"
#include "c3/theta/channel/wrapper/tcp.hpp.old.txt"

int main() {
  auto dgram_link = c3::theta::fake_floating_link_controller<512>();

  auto alice = dgram_link.listen(0);
  auto bob = dgram_link.listen(1);

  return 0;
}
