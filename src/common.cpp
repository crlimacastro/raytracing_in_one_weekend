#include "common.hpp"

const auto interval::empty = interval{infinity, -infinity};
const auto interval::universe = interval{-infinity, infinity};
const auto aabb::empty = aabb{interval::empty, interval::empty, interval::empty};
const auto aabb::universe = aabb{interval::universe, interval::universe, interval::universe};