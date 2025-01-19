#pragma once

#include <cmath>
#include <random>
#include <limits>
#include <array>

constexpr auto infinity = std::numeric_limits<float>::infinity();
constexpr auto pi = 3.141592f;
constexpr auto degrees_to_radians = pi / 180.0f;
constexpr auto radians_to_degrees = 180.0f / pi;

constexpr auto is_nan(float n) { return n != n; }

inline auto randf() -> float
{
    static std::uniform_real_distribution<float> distribution(0.f, 1.f);
    static std::mt19937 generator;
    return distribution(generator);
}
inline auto randf(float min, float max) -> float { return min + (max - min) * randf(); }

inline auto randi(int min, int max) -> int
{
    return int(randf(min, max + 1));
}

template <typename type>
constexpr auto lerp(type a, type b, float t) -> type { return a + t * (b - a); }

struct vec3;
constexpr auto operator*(const float s, const vec3 &v) -> vec3;

struct vec3
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        struct
        {
            float r;
            float g;
            float b;
        };
        float data[3];
    };

    constexpr auto
    operator-() const -> vec3
    {
        return {-x, -y, -z};
    }
    constexpr auto operator+(const vec3 &v) const -> vec3 { return {x + v.x, y + v.y, z + v.z}; }
    constexpr auto operator+=(const vec3 &v) -> vec3 & { return *this = *this + v; }
    constexpr auto operator-(const vec3 &v) const -> vec3 { return {x - v.x, y - v.y, z - v.z}; }
    constexpr auto operator-=(const vec3 &v) -> vec3 & { return *this = *this - v; }
    constexpr auto operator*(const float s) const -> vec3 { return {x * s, y * s, z * s}; }
    constexpr auto operator*=(const float s) -> vec3 & { return *this = *this * s; }
    constexpr auto operator/(const float s) const -> vec3 { return {x / s, y / s, z / s}; }
    constexpr auto operator/=(const float s) -> vec3 & { return *this = *this / s; }
    constexpr auto operator*(const vec3 &v) const -> vec3 { return {x * v.x, y * v.y, z * v.z}; }
    constexpr auto operator*=(const vec3 &v) -> vec3 & { return *this = *this * v; }
    constexpr auto operator/(const vec3 &v) -> vec3 { return {x / v.x, y / v.y, z / v.z}; }
    constexpr auto operator/=(const vec3 &v) -> vec3 & { return *this = *this / v; }
    constexpr auto magnitude_squared() const -> float { return x * x + y * y + z * z; }
    constexpr auto magnitude() const -> float { return sqrtf(magnitude_squared()); }
    constexpr auto normalized() const -> vec3 { return *this / magnitude(); }
    constexpr auto dot(const vec3 &v) const -> float { return x * v.x + y * v.y + z * v.z; }
    constexpr auto cross(const vec3 &v) const -> vec3 { return {y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x}; }

    constexpr auto near_zero() const
    {
        auto epsilon = 1e-8;
        return fabs(x) < epsilon && fabs(y) < epsilon && fabs(z) < epsilon;
    }

    constexpr auto reflect(const vec3 &normal) const -> vec3
    {
        return *this - 2 * this->dot(normal) * normal;
    }

    constexpr auto refract(const vec3 &normal, float etai_over_etat) const -> vec3
    {
        const auto uv = *this;
        const auto cos_theta = std::fmin((-uv).dot(normal), 1.f);
        const auto r_out_perp = etai_over_etat * (uv + cos_theta * normal);
        const auto r_out_parallel = -std::sqrtf(fabs(1.f - r_out_perp.magnitude_squared())) * normal;
        return r_out_perp + r_out_parallel;
    }

    static auto random() -> vec3 { return {randf(), randf(), randf()}; }
    static auto random(float min, float max) -> vec3 { return {randf(min, max), randf(min, max), randf(min, max)}; }
    static auto random_unit_vector() -> vec3
    {
        const auto phi = randf(0, pi * 2);
        const auto costheta = randf(-1, 1);
        const auto theta = std::acosf(costheta);
        const auto x = std::sinf(theta) * std::cosf(phi);
        const auto y = std::sinf(theta) * std::sinf(phi);
        const auto z = std::cosf(theta);
        return {x, y, z};
    }

    static auto random_on_hemisphere(const vec3 &normal) -> vec3
    {
        auto on_unit_sphere = random_unit_vector();
        if (on_unit_sphere.dot(normal) > 0.f)
        {
            return on_unit_sphere;
        }
        return -on_unit_sphere;
    }

    static auto random_in_unit_disk() -> vec3
    {
        while (true)
        {
            const auto p = vec3{randf(-1, 1), randf(-1, 1), 0};
            if (p.magnitude_squared() < 1)
                return p;
        }
    }

    static auto random_cosine_direction() -> vec3
    {
        const auto r1 = randf();
        const auto r2 = randf();

        const auto phi = 2 * pi * r1;
        const auto sqrt_r2 = std::sqrtf(r2);
        const auto x = std::cosf(phi) * sqrt_r2;
        const auto y = std::sinf(phi) * sqrt_r2;
        const auto z = std::sqrtf(1 - r2);

        return {x, y, z};
    }
};

using color = vec3;

struct onb
{
    std::array<vec3, 3> axis;

    constexpr onb(const vec3 &normal)
    {
        axis[2] = normal.normalized();
        const auto a = (std::fabsf(axis[2].x) > 0.9f) ? vec3{0, 1, 0} : vec3{1, 0, 0};
        axis[1] = axis[2].cross(a).normalized();
        axis[0] = axis[1].cross(axis[2]);
    }

    auto u() const -> const vec3 & { return axis[0]; }
    auto v() const -> const vec3 & { return axis[1]; }
    auto w() const -> const vec3 & { return axis[2]; }

    auto transform(const vec3 &v) const -> vec3 { return (v.x * axis[0]) + (v.y * axis[1]) + (v.z * axis[2]); }
};

struct pdf
{
    virtual ~pdf() {}
    virtual auto value(const vec3 &direction) const -> float = 0;
    virtual auto generate() const -> vec3 = 0;
};

struct sphere_pdf : pdf
{
    sphere_pdf() {}
    auto value(const vec3 &direction) const -> float { return 1.f / (4.f * pi); }
    auto generate() const -> vec3 { return vec3::random_unit_vector(); }
};

struct cosine_pdf : pdf
{
    onb uvw;

    cosine_pdf(const vec3 &w) : uvw{w} {}

    auto value(const vec3 &direction) const -> float override
    {
        const auto cosine_theta = direction.normalized().dot(uvw.w());
        return std::fmaxf(0.f, cosine_theta / pi);
    }

    auto generate() const -> vec3 override
    {
        return uvw.transform(vec3::random_cosine_direction());
    }
};

struct raytraceable;
struct raytraceable_pdf : pdf
{
    const raytraceable& obj;
    vec3 origin;

    raytraceable_pdf(const raytraceable& obj, const vec3& origin) : obj{obj}, origin{origin} {}

    auto value(const vec3& direction) const -> float override;
    auto generate() const -> vec3 override;
};

struct mixture_pdf : pdf
{
    std::array<std::shared_ptr<pdf>, 2> p;

    mixture_pdf(std::shared_ptr<pdf> p1, std::shared_ptr<pdf> p2) : p{p1, p2} {}

    auto value(const vec3& direction) const -> float override
    {
        return 0.5f * p[0]->value(direction) + 0.5f * p[1]->value(direction);
    }

    auto generate() const -> vec3 override
    {
        if (randf() < 0.5f)
            return p[0]->generate();
        return p[1]->generate();
    }
};

constexpr auto
linear_to_gamma(float linear) -> float
{
    if (linear > 0.f)
    {
        return std::sqrtf(linear);
    }
    return 0.f;
}

constexpr auto operator*(const float s, const vec3 &v) -> vec3 { return {v.x * s, v.y * s, v.z * s}; }

struct ray
{
    vec3 origin{};
    vec3 direction{};
    float time{};

    vec3 at(float t) const { return origin + direction * t; }
};

struct angle
{
    float radians{};

    constexpr auto degrees() const -> float { return radians * radians_to_degrees; }

    constexpr static auto from_radians(float radians) -> angle { return {radians}; }
    constexpr static auto from_degrees(float degrees) -> angle { return {degrees * degrees_to_radians}; }

    constexpr auto operator+(const angle &a) const -> angle { return {radians + a.radians}; }
    constexpr auto operator-(const angle &a) const -> angle { return {radians - a.radians}; }
    constexpr auto operator*(const float s) const -> angle { return {radians * s}; }
    constexpr auto operator/(const float s) const -> angle { return {radians / s}; }
};

struct interval
{
    float min = -infinity;
    float max = infinity;

    static constexpr auto from_intervals(const interval &a, const interval &b) -> interval
    {
        return {std::min(a.min, b.min), std::max(a.max, b.max)};
    }

    constexpr auto size() const -> float { return max - min; }
    constexpr auto contains(float n) const -> bool { return min <= n && n <= max; }
    constexpr auto surrounds(float n) const -> bool { return min < n && n < max; }
    constexpr auto clamp(float n) const -> float { return std::max(min, std::min(max, n)); }
    constexpr auto expand(float delta) const -> interval
    {
        auto padding = delta / 2.f;
        return {min - padding, max + padding};
    }

    constexpr auto operator+(float displacement) const -> interval
    {
        return interval{min + displacement, max + displacement};
    }

    static const interval empty;
    static const interval universe;
};

constexpr auto operator+(float displacement, const interval &ival) -> interval
{
    return ival + displacement;
}

struct aabb
{
    interval x, y, z;

    aabb() = default;

    aabb(interval x, interval y, interval z)
        : x(x), y(y), z(z)
    {
        pad_to_minimums();
    }

    static auto from_points(const vec3 &a, const vec3 &b) -> aabb
    {
        return aabb(
            (a.x <= b.x) ? interval{a.x, b.x} : interval{b.x, a.x},
            (a.y <= b.y) ? interval{a.y, b.y} : interval{b.y, a.y},
            (a.z <= b.z) ? interval{a.z, b.z} : interval{b.z, a.z});
    }

    static auto from_aabbs(const aabb &a, const aabb &b) -> aabb
    {
        return aabb(
            interval::from_intervals(a.x, b.x),
            interval::from_intervals(a.y, b.y),
            interval::from_intervals(a.z, b.z));
    }

    constexpr auto axis_interval(int n) const -> interval
    {
        if (n == 1)
            return y;
        if (n == 2)
            return z;
        return x;
    }

    constexpr auto hit(const ray &r, interval ray_t) const -> bool
    {
        const vec3 &ray_orig = r.origin;
        const vec3 &ray_dir = r.direction;

        for (int axis = 0; axis < 3; axis++)
        {
            const interval &ax = axis_interval(axis);
            const float adinv = 1.0 / ray_dir.data[axis];

            auto t0 = (ax.min - ray_orig.data[axis]) * adinv;
            auto t1 = (ax.max - ray_orig.data[axis]) * adinv;

            if (t0 < t1)
            {
                if (t0 > ray_t.min)
                    ray_t.min = t0;
                if (t1 < ray_t.max)
                    ray_t.max = t1;
            }
            else
            {
                if (t1 > ray_t.min)
                    ray_t.min = t1;
                if (t0 < ray_t.max)
                    ray_t.max = t0;
            }

            if (ray_t.max <= ray_t.min)
                return false;
        }
        return true;
    }

    constexpr auto longest_axis() const -> int
    {
        if (x.size() > y.size())
        {
            return x.size() > z.size() ? 0 : 2;
        }

        return y.size() > z.size() ? 1 : 2;
    }

    constexpr auto operator+(const vec3 &offset) const -> aabb
    {
        return aabb(x + offset.x, y + offset.y, z + offset.z);
    }

    static const aabb empty, universe;

private:
    void pad_to_minimums()
    {
        float delta = 0.0001;
        if (x.size() < delta)
            x = x.expand(delta);
        if (y.size() < delta)
            y = y.expand(delta);
        if (z.size() < delta)
            z = z.expand(delta);
    }
};

constexpr auto operator+(const vec3 &offset, const aabb &bbox) -> aabb
{
    return bbox + offset;
}
