#pragma once

#include <cmath>
#include <random>
#include <limits>

constexpr auto infinity = std::numeric_limits<float>::infinity();
constexpr auto pi = 3.141592f;
constexpr auto degrees_to_radians = pi / 180.0f;
constexpr auto radians_to_degrees = 180.0f / pi;

inline auto randf() -> float
{
    static std::uniform_real_distribution<float> distribution(0.f, 1.f);
    static std::mt19937 generator;
    return distribution(generator);
}
inline auto randf(float min, float max) -> float { return min + (max - min) * randf(); }

template <typename type>
constexpr auto lerp(type a, type b, float t) -> type { return a + t * (b - a); }

struct vec3;
constexpr auto operator*(const float s, const vec3 &v) -> vec3;

struct vec3
{
    float x{};
    float y{};
    float z{};

    constexpr auto operator-() const -> vec3 { return {-x, -y, -z}; }
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
};

using color = vec3;

constexpr auto linear_to_gamma(float linear) -> float
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

    constexpr auto size() const -> float { return max - min; }
    constexpr auto contains(float n) const -> bool { return min <= n && n <= max; }
    constexpr auto surrounds(float n) const -> bool { return min < n && n < max; }
    constexpr auto clamp(float n) const -> float { return std::max(min, std::min(max, n)); }

    static const interval empty;
    static const interval universe;
};
