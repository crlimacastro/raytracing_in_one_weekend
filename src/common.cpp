#include "common.hpp"

const auto interval::empty = interval{infinity, -infinity};
const auto interval::universe = interval{-infinity, infinity};