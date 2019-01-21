#include <limits>

static_assert(std::numeric_limits<long double>::digits>=31,"Float not precise enough");
