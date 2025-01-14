#include "raytraceable.hpp"

auto world::optimize() -> void
{
    objs = std::vector<std::shared_ptr<raytraceable>>{std::make_shared<bvh_node>(bvh_node::from_world(*this))};
}