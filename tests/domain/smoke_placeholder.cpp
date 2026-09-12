#include <cassert>

namespace mru::domain {
int domain_placeholder();
}

int main() {
  assert(mru::domain::domain_placeholder() == 0);
  return 0;
}
