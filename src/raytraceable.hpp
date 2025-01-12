#pragma once

#include <memory>
#include <vector>
#include <span>

#include "common.hpp"

struct material;

struct hit_result
{
    vec3 p{};
    vec3 normal{};
    std::shared_ptr<material> mat;
    float t{};
    bool front_face{};

    void set_face_normal(const ray &r, const vec3 &outward_normal)
    {
        front_face = r.direction.dot(outward_normal) < 0.f;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

struct raytraceable
{
    virtual ~raytraceable() = default;
    virtual bool hit(const ray &r, const interval &t, hit_result &res) const = 0;
};

using world = std::vector<std::unique_ptr<raytraceable>>;
using world_view = std::span<std::unique_ptr<raytraceable>>;

auto hit_anything(world_view objs, const ray &r, const interval &t, hit_result &res) -> bool
{
    hit_result temp_res{};
    bool hit_anything = false;
    auto closest = t.max;

    for (const auto &obj : objs)
    {
        if (obj->hit(r, interval(t.min, closest), temp_res))
        {
            hit_anything = true;
            closest = temp_res.t;
            res = temp_res;
        }
    }

    return hit_anything;
}

struct sphere : raytraceable
{
    vec3 center{};
    float radius = 1;
    std::shared_ptr<material> mat{};

    sphere() = default;
    sphere(vec3 center, float radius, std::shared_ptr<material> mat) : center(center), radius(radius), mat(mat) {}

    bool hit(const ray &r, const interval &t, hit_result &res) const override
    {
        auto oc = center - r.origin;
        auto a = r.direction.magnitude_squared();
        auto h = r.direction.dot(oc);
        auto c = oc.magnitude_squared() - radius * radius;
        auto discriminant = h * h - a * c;

        if (discriminant < 0)
            return false;

        auto sqrtd = std::sqrtf(discriminant);

        auto root = (h - sqrtd) / a;
        if (!t.surrounds(root))
        {
            root = (h + sqrtd) / a;
            if (!t.surrounds(root))
                return false;
        }

        res.t = root;
        res.p = r.at(res.t);
        vec3 outward_normal = (res.p - center) / radius;
        res.set_face_normal(r, outward_normal);
        res.mat = mat;

        return true;
    }
};