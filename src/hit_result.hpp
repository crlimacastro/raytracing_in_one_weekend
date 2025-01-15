#pragma once

#include <memory>

#include "common.hpp"


struct material;

struct hit_result
{
    vec3 p{};
    vec3 normal{};
    std::shared_ptr<material> mat;
    float t{};
    float u{};
    float v{};
    bool front_face{};

    void set_face_normal(const ray &r, const vec3 &outward_normal)
    {
        front_face = r.direction.dot(outward_normal) < 0.f;
        normal = front_face ? outward_normal : -outward_normal;
    }
};
