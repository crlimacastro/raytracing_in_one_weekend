#include "common.hpp"
#include "raytraceable.hpp"

const auto interval::empty = interval{infinity, -infinity};
const auto interval::universe = interval{-infinity, infinity};
const auto aabb::empty = aabb{interval::empty, interval::empty, interval::empty};
const auto aabb::universe = aabb{interval::universe, interval::universe, interval::universe};

auto raytraceable_pdf::value(const vec3 &direction) const -> float
{
    return obj.pdf_value(origin, direction);
}

auto raytraceable_pdf::generate() const -> vec3
{
    return obj.random(origin);
}
