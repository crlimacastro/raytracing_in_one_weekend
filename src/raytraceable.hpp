#pragma once

#include <memory>
#include <vector>
#include <span>
#include <algorithm>

#include "common.hpp"
#include "hit_result.hpp"
#include "material.hpp"

struct material;



struct raytraceable
{
    virtual ~raytraceable() = default;
    virtual auto hit(const ray &r, const interval &t, hit_result &res) const -> bool = 0;
    virtual auto bbox() const -> aabb = 0;
};

struct translate : raytraceable
{
    std::shared_ptr<raytraceable> object;
    vec3 offset;
    aabb m_bbox;

    translate(std::shared_ptr<raytraceable> object, const vec3 &offset)
        : object(object), offset(offset)
    {
        m_bbox = object->bbox() + offset;
    }

    auto hit(const ray &r, const interval &ray_t, hit_result &res) const -> bool override
    {
        // Move the ray backwards by the offset
        ray offset_r{r.origin - offset, r.direction, r.time};

        // Determine whether an intersection exists along the offset ray (and if so, where)
        if (!object->hit(offset_r, ray_t, res))
            return false;

        // Move the intersection point forwards by the offset
        res.p += offset;

        return true;
    }

    auto bbox() const -> aabb override { return m_bbox; }
};

struct rotate_y : raytraceable
{
    std::shared_ptr<raytraceable> object;
    float sin_theta;
    float cos_theta;
    aabb m_bbox;

    rotate_y(std::shared_ptr<raytraceable> object, angle angle) : object(object)
    {
        sin_theta = std::sin(angle.radians);
        cos_theta = std::cos(angle.radians);
        m_bbox = object->bbox();

        vec3 min{infinity, infinity, infinity};
        vec3 max{-infinity, -infinity, -infinity};

        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 2; j++)
            {
                for (int k = 0; k < 2; k++)
                {
                    auto x = i * m_bbox.x.max + (1 - i) * m_bbox.x.min;
                    auto y = j * m_bbox.y.max + (1 - j) * m_bbox.y.min;
                    auto z = k * m_bbox.z.max + (1 - k) * m_bbox.z.min;

                    auto newx = cos_theta * x + sin_theta * z;
                    auto newz = -sin_theta * x + cos_theta * z;

                    vec3 tester{newx, y, newz};

                    for (int c = 0; c < 3; c++)
                    {
                        min.data[c] = std::fminf(min.data[c], tester.data[c]);
                        max.data[c] = std::fmaxf(max.data[c], tester.data[c]);
                    }
                }
            }
        }

        m_bbox = aabb::from_points(min, max);
    }

    auto hit(const ray &r, const interval &ray_t, hit_result &res) const -> bool override
    {

        // Transform the ray from world space to object space.

        auto origin = vec3{
            (cos_theta * r.origin.x) - (sin_theta * r.origin.z),
            r.origin.y,
            (sin_theta * r.origin.x) + (cos_theta * r.origin.z)};

        auto direction = vec3{
            (cos_theta * r.direction.x) - (sin_theta * r.direction.z),
            r.direction.y,
            (sin_theta * r.direction.x) + (cos_theta * r.direction.z)};

        ray rotated_r{origin, direction, r.time};

        // Determine whether an intersection exists in object space (and if so, where).

        if (!object->hit(rotated_r, ray_t, res))
            return false;

        // Transform the intersection from object space back to world space.

        res.p = vec3{
            (cos_theta * res.p.x) + (sin_theta * res.p.z),
            res.p.y,
            (-sin_theta * res.p.x) + (cos_theta * res.p.z)};

        res.normal = vec3{
            (cos_theta * res.normal.x) + (sin_theta * res.normal.z),
            res.normal.y,
            (-sin_theta * res.normal.x) + (cos_theta * res.normal.z)};

        return true;
    }

    aabb bbox() const override { return m_bbox; }
};

struct world : raytraceable
{
    std::vector<std::shared_ptr<raytraceable>> objs{};

    auto add(std::shared_ptr<raytraceable> obj) -> void
    {
        objs.emplace_back(obj);
        m_bbox = aabb::from_aabbs(m_bbox, obj->bbox());
    }

    auto hit(const ray &r, const interval &t, hit_result &res) const -> bool override
    {
        hit_result temp_res{};
        bool hit_anything = false;
        auto closest = t.max;

        for (const auto &obj : objs)
        {
            if (obj->hit(r, interval{t.min, closest}, temp_res))
            {
                hit_anything = true;
                closest = temp_res.t;
                res = temp_res;
            }
        }

        return hit_anything;
    }

    auto bbox() const -> aabb override { return m_bbox; }

    auto optimize() -> void;

private:
    aabb m_bbox;
};

struct sphere : raytraceable
{
    ray center{};
    float radius = 1;
    std::shared_ptr<material> mat{};

    sphere() = default;

    static auto stationary(const vec3 &center, float radius, std::shared_ptr<material> mat)
    {
        return moving(center, vec3{0.f, 0.f, 0.f}, radius, mat);
    }

    static auto moving(const vec3 &center1, const vec3 &center2, float radius, std::shared_ptr<material> mat) -> sphere
    {
        auto s = sphere();
        s.center = {center1, center2 - center1};
        s.radius = radius;
        s.mat = mat;
        const auto rvec = vec3{radius, radius, radius};
        const auto box1 = aabb::from_points(s.center.at(0.f) - rvec, s.center.at(1.f) + rvec);
        const auto box2 = aabb::from_points(s.center.at(1.f) - rvec, s.center.at(1.f) + rvec);
        s.m_bbox = aabb::from_aabbs(box1, box2);
        return s;
    }

    auto hit(const ray &r, const interval &t, hit_result &res) const -> bool override
    {
        auto current_center = center.at(r.time);
        auto oc = current_center - r.origin;
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
        vec3 outward_normal = (res.p - current_center) / radius;
        res.set_face_normal(r, outward_normal);
        get_sphere_uv(outward_normal, res.u, res.v);
        res.mat = mat;

        return true;
    }

    auto bbox() const -> aabb override
    {
        return m_bbox;
    }

    static auto get_sphere_uv(const vec3 &p, float &u, float &v) -> void
    {
        auto theta = std::acosf(-p.y);
        auto phi = std::atan2f(-p.z, p.x) + pi;

        u = phi / (2 * pi);
        v = theta / pi;
    }

private:
    aabb m_bbox{};
};

struct bvh_node : raytraceable
{
    std::shared_ptr<raytraceable> left{};
    std::shared_ptr<raytraceable> right{};

    static auto from_world(world &w) -> bvh_node { return from_world_slice(w, 0, w.objs.size()); }

    static auto from_world_slice(world &w, std::size_t start, std::size_t end) -> bvh_node
    {
        auto node = bvh_node{};

        auto bbox = aabb::empty;

        for (std::size_t i = start; i < end; ++i)
        {
            bbox = aabb::from_aabbs(bbox, w.objs[i]->bbox());
        }
        node.m_bbox = bbox;

        int axis = bbox.longest_axis();

        auto comparator = (axis == 0)   ? box_x_compare
                          : (axis == 1) ? box_y_compare
                                        : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1)
        {
            node.left = node.right = w.objs[start];
        }
        else if (object_span == 2)
        {
            node.left = w.objs[start];
            node.right = w.objs[start + 1];
        }
        else
        {
            std::sort(std::begin(w.objs) + start, std::begin(w.objs) + end, comparator);

            auto mid = start + object_span / 2;
            node.left = std::make_shared<bvh_node>(bvh_node::from_world_slice(w, start, mid));
            node.right = std::make_shared<bvh_node>(bvh_node::from_world_slice(w, mid, end));
        }

        return node;
    }

    auto hit(const ray &r, const interval &t, hit_result &res) const -> bool override
    {
        if (!m_bbox.hit(r, t))
            return false;

        bool hit_left = left->hit(r, t, res);
        bool hit_right = right->hit(r, interval{t.min, hit_left ? res.t : t.max}, res);

        return hit_left || hit_right;
    }

    auto bbox() const -> aabb override { return m_bbox; }

private:
    aabb m_bbox{};

    static bool box_compare(
        const std::shared_ptr<raytraceable> a, const std::shared_ptr<raytraceable> b, int axis_index)
    {
        auto a_axis_interval = a->bbox().axis_interval(axis_index);
        auto b_axis_interval = b->bbox().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare(const std::shared_ptr<raytraceable> a, const std::shared_ptr<raytraceable> b)
    {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare(const std::shared_ptr<raytraceable> a, const std::shared_ptr<raytraceable> b)
    {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare(const std::shared_ptr<raytraceable> a, const std::shared_ptr<raytraceable> b)
    {
        return box_compare(a, b, 2);
    }
};

struct quad : raytraceable
{
    vec3 q;
    vec3 u, v;
    vec3 w;
    std::shared_ptr<material> mat;
    aabb m_bbox;
    vec3 normal;
    float d;

    quad(const vec3 &q, const vec3 &u, const vec3 &v, std::shared_ptr<material> mat)
        : q(q), u(u), v(v), mat(mat)
    {
        auto n = u.cross(v);
        normal = n.normalized();
        d = normal.dot(q);
        w = n / n.dot(n);
        set_bounding_box();
    }

    virtual void set_bounding_box()
    {
        auto bbox_diagonal1 = aabb::from_points(q, q + u + v);
        auto bbox_diagonal2 = aabb::from_points(q + u, q + v);
        m_bbox = aabb::from_aabbs(bbox_diagonal1, bbox_diagonal2);
    }

    aabb bbox() const override { return m_bbox; }

    auto hit(const ray &r, const interval &ray_t, hit_result &res) const -> bool override
    {
        auto denom = normal.dot(r.direction);

        if (std::fabs(denom) < 1e-8)
            return false;

        auto t = (d - normal.dot(r.origin)) / denom;
        if (!ray_t.contains(t))
            return false;

        auto intersection = r.at(t);
        vec3 planar_hitpt_vector = intersection - q;
        auto alpha = w.dot(planar_hitpt_vector.cross(v));
        auto beta = w.dot(u.cross(planar_hitpt_vector));

        if (!is_interior(alpha, beta, res))
            return false;

        res.t = t;
        res.p = intersection;
        res.mat = mat;
        res.set_face_normal(r, normal);

        return true;
    }

    virtual bool is_interior(float a, float b, hit_result &res) const
    {
        interval unit_interval = interval{0, 1};

        if (!unit_interval.contains(a) || !unit_interval.contains(b))
            return false;

        res.u = a;
        res.v = b;
        return true;
    }
};

inline std::shared_ptr<world> box(const vec3 &a, const vec3 &b, std::shared_ptr<material> mat)
{
    auto sides = std::make_shared<world>();

    auto min = vec3{std::fminf(a.x, b.x), std::fminf(a.y, b.y), std::fminf(a.z, b.z)};
    auto max = vec3{std::fmaxf(a.x, b.x), std::fmaxf(a.y, b.y), std::fmaxf(a.z, b.z)};

    auto dx = vec3{max.x - min.x, 0, 0};
    auto dy = vec3{0, max.y - min.y, 0};
    auto dz = vec3{0, 0, max.z - min.z};

    sides->add(std::make_shared<quad>(vec3{min.x, min.y, max.z}, dx, dy, mat));
    sides->add(std::make_shared<quad>(vec3{max.x, min.y, max.z}, -dz, dy, mat));
    sides->add(std::make_shared<quad>(vec3{max.x, min.y, min.z}, -dx, dy, mat));
    sides->add(std::make_shared<quad>(vec3{min.x, min.y, min.z}, dz, dy, mat));
    sides->add(std::make_shared<quad>(vec3{min.x, max.y, max.z}, dx, -dz, mat));
    sides->add(std::make_shared<quad>(vec3{min.x, min.y, min.z}, dx, dz, mat));

    return sides;
}

struct constant_medium : raytraceable {
    constant_medium(std::shared_ptr<raytraceable> boundary, float density, std::shared_ptr<texture> tex)
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(std::make_shared<isotropic>(tex))
    {}

    constant_medium(std::shared_ptr<raytraceable> boundary, float density, const color& albedo)
      : boundary(boundary), neg_inv_density(-1/density),
        phase_function(std::make_shared<isotropic>(albedo))
    {}

    auto hit(const ray& r, const interval& ray_t, hit_result& rec) const -> bool override {
        hit_result rec1, rec2;

        if (!boundary->hit(r, interval::universe, rec1))
            return false;

        if (!boundary->hit(r, interval(rec1.t+0.0001, infinity), rec2))
            return false;

        if (rec1.t < ray_t.min) rec1.t = ray_t.min;
        if (rec2.t > ray_t.max) rec2.t = ray_t.max;

        if (rec1.t >= rec2.t)
            return false;

        if (rec1.t < 0)
            rec1.t = 0;

        auto ray_length = r.direction.magnitude();
        auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
        auto hit_distance = neg_inv_density * std::logf(randf());

        if (hit_distance > distance_inside_boundary)
            return false;

        rec.t = rec1.t + hit_distance / ray_length;
        rec.p = r.at(rec.t);

        rec.normal = vec3{1,0,0};  // arbitrary
        rec.front_face = true;     // also arbitrary
        rec.mat = phase_function;

        return true;
    }

    auto bbox() const -> aabb override { return boundary->bbox(); }

  private:
    std::shared_ptr<raytraceable> boundary;
    float neg_inv_density;
    std::shared_ptr<material> phase_function;
};
